/**
 ****************************************************************************************************
 * @file        lcd.h  (OPTIMIZED)
 * @version     V2.0
 * @brief       LCD display application header — optimized for STM32H743 LTDC + DMA2D
 *
 ****************************************************************************************************
 */

#ifndef __LCD_H
#define __LCD_H

#include "stdlib.h"
#include <stdbool.h>
#include "main.h"
#include "ltdc.h"
#include "lcd_ltdc.h" /* defines LTDC_PIXFORMAT + lcdltdc: MUST come before the #if blocks below */

#ifndef LTDC_PIXFORMAT
#error "LTDC_PIXFORMAT is not defined - lcd.h would silently pick the wrong pixel format"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Core LCD device descriptor                                                                     */
typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t id;
    uint8_t dir; /* 0=portrait, 1=landscape */
    uint16_t wramcmd;
    uint16_t setxcmd;
    uint16_t setycmd;
} _lcd_dev;

extern _lcd_dev lcddev;
extern uint8_t g_spi_mode;
extern uint32_t g_point_color;
extern uint32_t g_back_color;

/* GPIO macros                                                                                    */
#define LCD_BL(x) LTDC_BL(x)
#define LCD_RST(x) LTDC_RST(x)

/* Scan direction constants                                                                       */
#define L2R_U2D 0
#define L2R_D2U 1
#define R2L_U2D 2
#define R2L_D2U 3
#define U2D_L2R 4
#define U2D_R2L 5
#define D2U_L2R 6
#define D2U_R2L 7
#define DFT_SCAN_DIR L2R_U2D

/* Color definitions — selected by pixel format                                                   */

#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB565

/* ---- Standard colors ---- */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define CYAN 0x07FF

/* ---- Extended colors ---- */
#define BROWN 0xBC40
#define BRRED 0xFC07
#define GRAY 0x8430
#define DARKBLUE 0x01CF
#define LIGHTBLUE 0x7D7C
#define GRAYBLUE 0x5458
#define LIGHTGREEN 0x841F
#define LGRAY 0xC618
#define LGRAYBLUE 0xA651
#define LBBLUE 0x2B12

/* ---- NEW extra colors (RGB565) ---- */
#define ORANGE 0xFD20    /* Pure orange                     */
#define PINK 0xF81F      /* Hot pink (same hue as magenta)  */
#define VIOLET 0x781F    /* Purple-violet                   */
#define LIME 0x07E0      /* Bright lime (= GREEN)           */
#define TEAL 0x0410      /* Teal / dark cyan                */
#define MAROON 0x7800    /* Dark red                        */
#define NAVY 0x000F      /* Dark navy blue                  */
#define OLIVE 0x7BE0     /* Olive / dark yellow             */
#define SILVER 0xC618    /* Silver (same as LGRAY)          */
#define GOLD 0xFEA0      /* Gold                            */
#define INDIGO 0x400F    /* Indigo                          */
#define CORAL 0xFBEA     /* Coral salmon                    */
#define SALMON 0xFC0E    /* Salmon red                      */
#define SKYBLUE 0x867D   /* Sky blue                        */
#define DARKGREEN 0x03E0 /* Dark green                      */
#define DARKGRAY 0x4208  /* Dark gray                       */
#define LIGHTGRAY 0xD69A /* Light gray                      */
#define CHOCOLATE 0xD343 /* Chocolate brown                 */
#define TURQUOISE 0x471A /* Turquoise                       */
#define LAVENDER 0xE73F  /* Lavender                        */

#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888

#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define MAGENTA 0xFF00FF
#define YELLOW 0xFFFF00
#define CYAN 0x00FFFF
#define BROWN 0xB88800
#define BRRED 0xF88038
#define GRAY 0x808480
#define DARKBLUE 0x003878
#define LIGHTBLUE 0x78ACE0
#define GRAYBLUE 0x5088C0
#define LIGHTGREEN 0x8080F8
#define LGRAY 0xC0C0C0
#define LGRAYBLUE 0xA0C888
#define LBBLUE 0x286090
/* NEW */
#define ORANGE 0xFF8000
#define PINK 0xFF69B4
#define VIOLET 0x8B00FF
#define LIME 0x32CD32
#define TEAL 0x008080
#define MAROON 0x800000
#define NAVY 0x000080
#define OLIVE 0x808000
#define SILVER 0xC0C0C0
#define GOLD 0xFFD700
#define INDIGO 0x4B0082
#define CORAL 0xFF7F50
#define SALMON 0xFA8072
#define SKYBLUE 0x87CEEB
#define DARKGREEN 0x006400
#define DARKGRAY 0x404040
#define LIGHTGRAY 0xD3D3D3
#define CHOCOLATE 0xD2691E
#define TURQUOISE 0x40E0D0
#define LAVENDER 0xE6E6FA

#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888

#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define MAGENTA 0xFFFF00FF
#define YELLOW 0xFFFFFF00
#define CYAN 0xFF00FFFF
#define BROWN 0xFFB88800
#define BRRED 0xFFF88038
#define GRAY 0xFF808480
#define DARKBLUE 0xFF003878
#define LIGHTBLUE 0xFF78ACE0
#define GRAYBLUE 0xFF5088C0
#define LIGHTGREEN 0xFF8080F8
#define LGRAY 0xFFC0C0C0
#define LGRAYBLUE 0xFFA0C888
#define LBBLUE 0xFF286090
/* NEW */
#define ORANGE 0xFFFF8000
#define PINK 0xFFFF69B4
#define VIOLET 0xFF8B00FF
#define LIME 0xFF32CD32
#define TEAL 0xFF008080
#define MAROON 0xFF800000
#define NAVY 0xFF000080
#define OLIVE 0xFF808000
#define SILVER 0xFFC0C0C0
#define GOLD 0xFFFFD700
#define INDIGO 0xFF4B0082
#define CORAL 0xFFFF7F50
#define SALMON 0xFFFA8072
#define SKYBLUE 0xFF87CEEB
#define DARKGREEN 0xFF006400
#define DARKGRAY 0xFF404040
#define LIGHTGRAY 0xFFD3D3D3
#define CHOCOLATE 0xFFD2691E
#define TURQUOISE 0xFF40E0D0
#define LAVENDER 0xFFE6E6FA

#endif /* LTDC_PIXFORMAT */

/* g_ltdc_framebuf is defined in ltdc.c — declare it here so lcd.h macros can use it            */
extern uint32_t *g_ltdc_framebuf[2];

/* FAST inline pixel write — bypasses all function-call overhead on LTDC path                    */
/* Usage: LCD_DRAW_POINT_FAST(x, y, color);                                                      */
/* WARNING: NO bounds check — caller must ensure x < lcddev.width, y < lcddev.height             */
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888
#define LCD_DRAW_POINT_FAST(x, y, color)                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        if (lcdltdc.dir)                                                                                     \
            *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] +                                   \
                          lcdltdc.pixsize * (lcdltdc.pwidth * (y) + (x))) = (color);                         \
        else                                                                                                 \
            *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] +                                   \
                          lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - (x) - 1) + (y))) = (color); \
    } while (0)
#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
#define LCD_DRAW_POINT_FAST(x, y, color)                                                                                                                        \
    do                                                                                                                                                          \
    {                                                                                                                                                           \
        uint32_t _off_ = lcdltdc.dir ? lcdltdc.pixsize * (lcdltdc.pwidth * (y) + (x)) : lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - (x) - 1) + (y)); \
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] + _off_) = (uint16_t)(color);                                                              \
        *(uint8_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] + _off_ + 2) = (uint8_t)((color) >> 16);                                                    \
    } while (0)
#else /* RGB565 */
extern uint8_t g_draw_layer;
#define LCD_DRAW_POINT_FAST(x, y, color)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (lcdltdc.dir)                                                                                               \
            *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] +                                                    \
                          lcdltdc.pixsize * (lcdltdc.pwidth * (y) + (x))) = (uint16_t)(color);                         \
        else                                                                                                           \
            *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_draw_layer] +                                                    \
                          lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - (x) - 1) + (y))) = (uint16_t)(color); \
    } while (0)
#endif

/* RGB565 helper macro                                                                            */
/** Build an RGB565 value at compile-time from 8-bit R,G,B components */
#define RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

/** Build an RGB888 / ARGB8888 value at compile-time */
#define RGB888(r, g, b) (uint32_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
#define ARGB8888(a, r, g, b) (uint32_t)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Function prototypes — original API (unchanged signatures)                                      */
void lcd_wr_data(volatile uint16_t data);
void lcd_wr_regno(volatile uint16_t regno);
void lcd_write_reg(uint16_t regno, uint16_t data);
void lcd_spi_speed_low(void);
void lcd_spi_speed_high(void);

bool lcd_init(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_scan_dir(uint8_t dir);
void lcd_display_dir(uint8_t dir);
void lcd_write_ram_prepare(void);
void lcd_set_cursor(uint16_t x, uint16_t y);

uint32_t lcd_rgb565torgb888(uint16_t rgb565);
uint32_t lcd_read_point(uint16_t x, uint16_t y);
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);

void lcd_clear(uint32_t color);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_flush(void);
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint32_t color);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color);

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint32_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint32_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint32_t color);
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color);

/* NEW fast/extra draw functions                                                                  */

/**
 * @brief  Draw a vertical line — uses DMA2D single ltdc_fill() call, O(1) setup
 * @param  x     X coordinate
 * @param  y     Top Y coordinate
 * @param  len   Length in pixels
 * @param  color Pixel color
 */
void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);

/**
 * @brief  Draw a solid filled rectangle — single DMA2D ltdc_fill() call, fastest possible
 * @param  x1,y1  Top-left corner
 * @param  x2,y2  Bottom-right corner
 * @param  color  Fill color
 */
void lcd_draw_rect_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);

/**
 * @brief  Fast line draw with dedicated horizontal/vertical fast path
 *         Falls back to Bresenham for diagonal lines.
 *         Horizontal and vertical lines are a single DMA2D call (~microseconds).
 */
void lcd_draw_line_fast(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);

/**
 * @brief  Draw a triangle outline using three lcd_draw_line_fast() calls
 */
void lcd_draw_triangle(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       uint32_t color);

/**
 * @brief  Draw a filled triangle using scanline algorithm + DMA2D per-row fill
 *         Much faster than plotting individual pixels.
 */
void lcd_draw_triangle_filled(uint16_t x0, uint16_t y0,
                              uint16_t x1, uint16_t y1,
                              uint16_t x2, uint16_t y2,
                              uint32_t color);

/**
 * @brief  DMA2D memory-to-memory blit: copy a rectangular region of the framebuffer
 *         to another position. Both regions must be within screen bounds.
 *         src and dst must not overlap.
 * @param  sx,sy   Source top-left
 * @param  dx,dy   Destination top-left
 * @param  w,h     Width and height of the region
 */
void lcd_copy_region(uint16_t sx, uint16_t sy,
                     uint16_t dx, uint16_t dy,
                     uint16_t w, uint16_t h);

/**
 * @brief  lcd_draw_circle_filled — more intuitive alias for lcd_fill_circle
 */
static inline void lcd_draw_circle_filled(uint16_t x, uint16_t y, uint16_t r, uint32_t color)
{
    lcd_fill_circle(x, y, r, color);
}

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */