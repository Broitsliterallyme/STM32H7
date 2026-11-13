#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
#include <stdint.h>
#include <stdio.h>
#include "common/debug.h"
#include "common/processor_type.h"
#include "main.h"
#include "gpio.h"
#include "lcd.h"
#include "lcd_ltdc.h"
#include "touch.h"

#include "sdram.h"
#include "sdram_test.h"

#define LED_ON(port, pin)  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define LED_OFF(port, pin) HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)

#define LED_ERR_ON()  LED_ON(LED_ERR_GPIO_Port, LED_ERR_Pin)
#define LED_ERR_OFF() LED_OFF(LED_ERR_GPIO_Port, LED_ERR_Pin)
#define LED_OK_ON()   LED_ON(LED_RUN_GPIO_Port, LED_RUN_Pin)
#define LED_OK_OFF()  LED_OFF(LED_RUN_GPIO_Port, LED_RUN_Pin)

extern "C" volatile uint8_t g_enter_bootloader;
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
#ifdef __cplusplus
}
#endif

#endif /* __APP_H */