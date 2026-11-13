/**
 * @file    lcd_ltdc.h
 * @brief   LCD / LTDC driver (vendor drawing code) ported to CubeMX + HAL.
 *
 * - Renamed from ltdc.h because CubeMX generates its own ltdc.c / ltdc.h.
 * - Pins, LTDC peripheral clock and the HAL handle `hltdc` come from CubeMX (ltdc.c).
 * - Panel timings, PLL3 pixel clock and the layer are set at runtime from the panel table
 *   in lcd_ltdc.c (panel is auto-detected from the R7/G7/B7 strap pins, like the vendor code).
 * - Framebuffers live at fixed SDRAM addresses, so no linker-script section is needed.
 */
#ifndef __LCD_LTDC_H
#define __LCD_LTDC_H

#include "main.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

    /* Panel is fixed: 4.3\" 800x480 (ID 0x4384). Timings are in lcd_ltdc.c. */

    /* ------------------------------------------------------------------ device descriptor */
    typedef struct
    {
        uint32_t pwidth;     /* physical panel width    */
        uint32_t pheight;    /* physical panel height                            */
        uint16_t hsw, vsw;   /* sync widths                                      */
        uint16_t hbp, vbp;   /* back porches                                     */
        uint16_t hfp, vfp;   /* front porches                                    */
        uint8_t activelayer; /* buffer currently shown (0/1)                     */
        uint8_t dir;         /* 0 = portrait, 1 = landscape                      */
        uint16_t width;      /* logical width  (depends on dir)                  */
        uint16_t height;     /* logical height (depends on dir)                  */
        uint32_t pixsize;    /* bytes per pixel                                  */
        uint8_t pixformat;   /* LTDC_PIXFORMAT_xxx                               */
    } _ltdc_dev;

    extern _ltdc_dev lcdltdc;
    extern uint32_t *g_ltdc_framebuf[2];
    extern uint8_t g_draw_layer;

/* ------------------------------------------------------------------ pixel formats (same values as HAL) */
#define LTDC_PIXFORMAT_ARGB8888 0x00
#define LTDC_PIXFORMAT_RGB888   0x01
#define LTDC_PIXFORMAT_RGB565   0x02
#define LTDC_PIXFORMAT_ARGB1555 0x03
#define LTDC_PIXFORMAT_ARGB4444 0x04
#define LTDC_PIXFORMAT_L8       0x05
#define LTDC_PIXFORMAT_AL44     0x06
#define LTDC_PIXFORMAT_AL88     0x07

#define LTDC_PIXFORMAT      LTDC_PIXFORMAT_RGB565
#define LTDC_BACKLAYERCOLOR 0x00000000UL

/* ------------------------------------------------------------------ framebuffers in SDRAM */
#define LTDC_FRAME_BUF_ADDR 0xC0000000UL

#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888
#define LTDC_BYTES_PER_PIXEL 4UL
#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
#define LTDC_BYTES_PER_PIXEL 3UL
#else
#define LTDC_BYTES_PER_PIXEL 2UL
#endif

/* sized for the largest supported panel (1280x800); two buffers for lcd_flush() double buffering */
#define LTDC_FRAME_BUF_SIZE  (1280UL * 800UL * LTDC_BYTES_PER_PIXEL)
#define LTDC_FRAME_BUF0_ADDR (LTDC_FRAME_BUF_ADDR)
#define LTDC_FRAME_BUF1_ADDR (LTDC_FRAME_BUF_ADDR + LTDC_FRAME_BUF_SIZE)

/* ------------------------------------------------------------------ panel control pins (PD11 = RST, PD12 = BL) */
#define LTDC_RST_GPIO_PORT GPIOD
#define LTDC_RST_GPIO_PIN  GPIO_PIN_11
#define LTDC_BL_GPIO_PORT  GPIOD
#define LTDC_BL_GPIO_PIN   GPIO_PIN_12
#define LTDC_BL(x)         HAL_GPIO_WritePin(LTDC_BL_GPIO_PORT, LTDC_BL_GPIO_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define LTDC_RST(x)        HAL_GPIO_WritePin(LTDC_RST_GPIO_PORT, LTDC_RST_GPIO_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/* ------------------------------------------------------------------ compat for the old delay.h calls in lcd_ex */
#ifndef delay_ms
#define delay_ms(x) HAL_Delay(x)
#endif
#ifndef delay_us
#define delay_us(x) HAL_Delay((((uint32_t)(x)) + 999U) / 1000U)
#endif

    void ltdc_switch(uint8_t sw);
    void ltdc_layer_switch(uint8_t layerx, uint8_t sw);
    void ltdc_select_layer(uint8_t layerx);
    void ltdc_display_dir(uint8_t dir);
    void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color);
    uint32_t ltdc_read_point(uint16_t x, uint16_t y);
    void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
    void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
    void ltdc_clear(uint32_t color);
    uint8_t ltdc_clk_set(uint32_t pll3n, uint32_t pll3m, uint32_t pll3r);
    uint16_t ltdc_panelid_read(void);
    bool ltdc_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_LTDC_H */