#include "Display.h"

Display::Display()
    : tick(0)
{
}

Display disp;

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

    lcd_clear(WHITE);
}

void Display::disp_run()
{

    if (HAL_GetTick() - tick < 1)
        return;
    tick = HAL_GetTick();
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
    if (tp_scan())
        lcd_fill_circle(tp_dev.x[0], tp_dev.y[0], 4, RED);


    lcd_flush();
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
