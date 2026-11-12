#include "Display.h"

Display::Display()
{
    // Constructor implementation
}

Display disp;

void Display::disp_init()
{
    // Initialization code for the display
}
void Display::disp_run()
{
    if (HAL_GetTick() - tick < 1000) // Check if 1 second has passed
        return;
    tick = HAL_GetTick();                  // Update the tick to current time
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // Toggle the GPIO pin
    debug_poll();
}

void run()
{
    disp.disp_init();
    while (1)
    {
        disp.disp_run();
        if (g_enter_bootloader)
            request_bootloader();
    }
}
