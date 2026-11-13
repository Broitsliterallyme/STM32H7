/**
 * @file    touch.h
 * @brief   GT9xxx capacitive touch (bit-banged I2C, HAL GPIO only).
 *          Pins: SCL=PB12  SDA=PB13  RST=PB14  INT=PH7
 */
#ifndef __TOUCH_H
#define __TOUCH_H

#include "main.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TP_MAX 5 /* max simultaneous points read */

    typedef struct
    {
        uint16_t x[TP_MAX]; /* panel coordinates, 800x480 landscape */
        uint16_t y[TP_MAX];
        uint8_t n; /* number of fingers down (0 = no touch) */
    } _tp_dev;

    extern _tp_dev tp_dev;

    uint8_t tp_init(void); /* 0 = OK, 1 = chip did not ACK */
    uint8_t tp_scan(void); /* returns finger count; updates tp_dev */

#ifdef __cplusplus
}
#endif

#endif