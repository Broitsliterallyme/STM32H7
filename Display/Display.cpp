#include "Display.h"
#include "main.h"

Display::Display()
    : tick(0)
{
    // Constructor implementation
}

Display disp;

void Display::disp_init()
{

    if (sdram_init() != 0)
    {
        while (1)
        {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            HAL_Delay(100);
        }
    }

}

void Display::disp_run()
{

    if (HAL_GetTick() - tick < 100)
        return;
    tick = HAL_GetTick();
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);

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
