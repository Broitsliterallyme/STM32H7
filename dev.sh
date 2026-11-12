#!/bin/bash
set -e

APP_ID="0483:5740"     # your firmware (CDC virtual COM port)
DFU_ID="0483:df11"     # ST system bootloader
BAUD="${BAUD:-115200}"

CLEAN=false
BUILD=false
FLASH=false
MONITOR=false

for arg in "$@"; do
    case "$arg" in
        clean)   CLEAN=true ;;
        compile) BUILD=true ;;
        flash)   FLASH=true ;;
        monitor) MONITOR=true ;;
        all)     CLEAN=true; BUILD=true; FLASH=true; MONITOR=true ;;
        *)
            echo "Unknown command: $arg"
            exit 1
            ;;
    esac
done

# Find the /dev/ttyACM* that belongs to the device with APP_ID
find_port() {
    local vid="${APP_ID%%:*}" pid="${APP_ID##*:}" dev
    for dev in /dev/ttyACM*; do
        [ -e "$dev" ] || continue
        if udevadm info -q property -n "$dev" 2>/dev/null \
            | grep -qx "ID_VENDOR_ID=$vid" &&
           udevadm info -q property -n "$dev" 2>/dev/null \
            | grep -qx "ID_MODEL_ID=$pid"; then
            echo "$dev"
            return 0
        fi
    done
    return 1
}

port_present() { find_port >/dev/null; }
dfu_present()  { lsusb -d "$DFU_ID" | grep -q .; }

wait_for() {   # wait_for <description> <timeout_s> <command...>
    local desc="$1" timeout="$2"; shift 2
    for ((i = 0; i < timeout * 10; i++)); do
        "$@" && return 0
        sleep 0.1
    done
    echo "Timed out waiting for $desc"
    return 1
}

if $CLEAN; then
    rm -rf build
fi

if $BUILD; then
    mkdir -p build
    cd build
    cmake ..
    make -j
    cd ..
fi

if $FLASH; then
    if [ ! -f build/H7.bin ] || [ build/H7.elf -nt build/H7.bin ]; then
        arm-none-eabi-objcopy -O binary build/H7.elf build/H7.bin
    fi

    if ! dfu_present; then
        PORT="$(find_port)" || {
            echo "Neither $APP_ID (app) nor $DFU_ID (DFU) found. Plug the board in."
            exit 1
        }
        echo "Triggering bootloader via 1200 baud touch on $PORT..."
        stty -F "$PORT" 1200 || true
        wait_for "DFU device $DFU_ID" 5 dfu_present
    fi

    # "Error during download get_status" after :leave is expected (board resets and drops off the bus)
    if ! out=$(dfu-util -d "$DFU_ID" -a 0 -s 0x08000000:leave -D build/H7.bin 2>&1); then
        echo "$out"
        if echo "$out" | grep -q "File downloaded successfully"; then
            echo "Flash OK (ignoring post-leave status error)."
        else
            echo "Flash FAILED."
            exit 1
        fi
    else
        echo "$out"
    fi
fi

if $MONITOR; then
    echo "Waiting for $APP_ID..."
    wait_for "serial port" 10 port_present
    sleep 0.3
    exec tio -b "$BAUD" "$(find_port)"
fi