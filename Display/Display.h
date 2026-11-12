#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
#include <stdint.h>
#include <stdio.h>
#include "common/debug.h"
#include "common/processor_type.h"

class Display
{
public:
    // Constructor
    Display();
    void disp_init();
    void disp_run();
    uint32_t tick;

private:
};

extern "C"
{
#endif
    void run();
    void request_bootloader(void);
    extern volatile uint8_t g_enter_bootloader;
#ifdef __cplusplus
}
#endif

#endif /* __APP_H */
