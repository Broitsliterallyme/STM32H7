#include "Display.h"
#include <math.h>


static constexpr uint32_t UI_PERIOD_MS = 10; // touch + UI rate

// ---- page 0: controls -------------------------------------------------------
static Label       p0_title(20, 10, 760, 36, "CONTROLS   (swipe left / right)");
static Button      p0_go(20, 60, 160, 60, "GO");
static ToggleButton p0_led(200, 60, 160, 60, "LED OFF", "LED ON");
static Checkbox    p0_check(400, 74, 32, "Enable");
static Slider      p0_slider(20, 160, 500, 50, 0, 100, 50);
static Label       p0_slider_val(540, 160, 240, 50, "Value: 50");
static ProgressBar p0_bar(20, 240, 500, 30);
static InfoTile    p0_taps(20, 300, 220, 100, "GO taps", "0");
// ---- page 1: data -----------------------------------------------------------
static Label       p1_title(20, 10, 500, 36, "DATA");
static Graph       p1_graph(20, 50, 500, 300);
static FpsCounter  p1_fps(700, 10);
static StatusChip  p1_chip_touch(540, 60, 120, 40, "TOUCH");
static StatusChip  p1_chip_sdram(670, 60, 120, 40, "SDRAM");
static StatusChip  p1_chip_link(540, 110, 120, 40, "LINK");
static InfoTile    p1_uptime(540, 200, 240, 100, "Uptime s", "0");

// ---- page 2: log ------------------------------------------------------------
static Label       p2_title(20, 10, 760, 36, "LOG   (drag to scroll)");
static LogView     p2_log(20, 50, 760, 350);
static Button      p2_add(20, 415, 200, 55, "Add line");

static Page p0, p1, p2;
static PageManager pages;

// ---- callbacks --------------------------------------------------------------
static int go_taps = 0;

static void go_released(Button *)
{
    go_taps++;
    char b[16];
    snprintf(b, sizeof(b), "%d", go_taps);
    p0_taps.set_value(b);
    p2_log.push_line("GO button tapped", 0);
}

static void led_changed(ToggleButton *, bool on)
{
    if (on)
        LED_OK_ON();
    else
        LED_OK_OFF();
    p2_log.push_line(on ? "LED turned ON" : "LED turned OFF", 1);
}

static void check_changed(Checkbox *, bool checked)
{
    p2_log.push_line(checked ? "Enable: checked" : "Enable: cleared", 0);
}

// Called on EVERY finger movement while dragging = live.
static void slider_changed(Slider *, int32_t v)
{
    char b[24];
    snprintf(b, sizeof(b), "Value: %d", (int)v);
    p0_slider_val.set_text(b);
    p0_bar.set_value((uint8_t)v);
}

static void add_line(Button *)
{
    static uint8_t n = 0;
    char b[40];
    snprintf(b, sizeof(b), "Test line %u", (unsigned)n);
    p2_log.push_line(b, n % 3); // cycles INFO / WARN / ERR colours
    n++;
}

static void page_changed(int index)
{
    char b[32];
    snprintf(b, sizeof(b), "Page -> %d", index);
    p2_log.push_line(b, 0);
}

// ---- Display ----------------------------------------------------------------
Display::Display()
    : tick(0), graph_tick(0), uptime_tick(0)
{
}

Display disp;

void Display::build_ui()
{
    p0_go.on_release = go_released;
    p0_led.on_change = led_changed;
    p0_check.on_change = check_changed;
    p0_slider.on_change = slider_changed;
    p0_bar.set_value(50);

    p1_chip_touch.set_healthy(true);
    p1_chip_sdram.set_healthy(true);
    p1_chip_link.set_healthy(false);

    p2_add.on_release = add_line;
    p2_log.push_line("Boot OK", 0);
    p2_log.push_line("Swipe on empty space to change page", 0);
    p2_log.push_line("Sliders and this log never change the page", 1);

    p0.add(&p0_title);
    p0.add(&p0_go);
    p0.add(&p0_led);
    p0.add(&p0_check);
    p0.add(&p0_slider);
    p0.add(&p0_slider_val);
    p0.add(&p0_bar);
    p0.add(&p0_taps);

    p1.add(&p1_title);
    p1.add(&p1_graph);
    p1.add(&p1_fps);
    p1.add(&p1_chip_touch);
    p1.add(&p1_chip_sdram);
    p1.add(&p1_chip_link);
    p1.add(&p1_uptime);

    p2.add(&p2_title);
    p2.add(&p2_log);
    p2.add(&p2_add);

    pages.add(&p0);
    pages.add(&p1);
    pages.add(&p2);
    pages.bg_color = WHITE;
    pages.on_page_changed = page_changed;
    pages.begin(); // shows page 0
}

void Display::disp_init()
{
    if (sdram_init() != 0 || !lcd_init() || tp_init() != 0)
    {
        while (1)
        {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            HAL_Delay(40);
        }
    }

    build_ui();
}

// tp_dev -> RawTouch (the only place the two meet)
RawTouch Display::read_touch()
{
    RawTouch r{};
    r.n = tp_scan();
    for (uint8_t i = 0; i < 2 && i < r.n; i++)
        r.p[i] = {(int16_t)tp_dev.x[i], (int16_t)tp_dev.y[i]};
    return r;
}

void Display::disp_run()
{
    uint32_t now = HAL_GetTick();
    if (now - tick < UI_PERIOD_MS)
        return;
    tick = now;
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);

    if (now - graph_tick >= 50)
    {
        graph_tick = now;
        p1_graph.push((int32_t)(sinf(now * 0.002f) * 100.0f));
    }
    if (now - uptime_tick >= 1000)
    {
        uptime_tick = now;
        char b[16];
        snprintf(b, sizeof(b), "%u", (unsigned)(now / 1000));
        p1_uptime.set_value(b);
    }

    pages.update(read_touch());
    if (pages.draw())
        lcd_flush(); // only copy the framebuffer when something changed

    debug_poll();
}

void run()
{
    disp.disp_init();
    while (1)
    {
        disp.disp_run();
        if (g_enter_bootloader)
            bootloader_request();
    }
}