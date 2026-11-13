/**
 ****************************************************************************************************
 * @file        lcd.c  (OPTIMIZED V2.0, LTDC only)
 * @brief       LCD display driver - STM32H743 LTDC + DMA2D
 ****************************************************************************************************
 */

#include "stdlib.h"
#include "string.h"
#include "lcd.h"
#include "lcdfont.h"
#include "lcd_ltdc.h"

uint32_t g_point_color = RED;
uint32_t g_back_color = 0xFFFFFFFF;
uint8_t g_spi_mode = 0; /* kept only because lcd.h declares it; always 0 */
_lcd_dev lcddev;
extern uint8_t g_draw_layer;

/* ------------------------------------------------------------------ legacy API kept as no-ops
 * (declared in lcd.h, so other files may still call them; there is no SPI panel) */
void lcd_wr_data(volatile uint16_t data) { (void)data; }
void lcd_wr_regno(volatile uint16_t regno) { (void)regno; }
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    (void)regno;
    (void)data;
}
void lcd_write_ram_prepare(void) {}
void lcd_spi_speed_low(void) {}
void lcd_spi_speed_high(void) {}
void lcd_set_cursor(uint16_t x, uint16_t y)
{
    (void)x;
    (void)y;
}
void lcd_scan_dir(uint8_t dir) { (void)dir; }
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    (void)sx;
    (void)sy;
    (void)width;
    (void)height;
}

/* ------------------------------------------------------------------ basic */
uint32_t lcd_rgb565torgb888(uint16_t rgb565)
{
    uint32_t r = (rgb565 & 0xF800) >> 8;
    uint32_t g = (rgb565 & 0x07E0) >> 3;
    uint32_t b = (rgb565 & 0x001F) << 3;
    return (r << 16) | (g << 8) | b;
}

uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    if (x >= lcddev.width || y >= lcddev.height)
        return 0;
    return ltdc_read_point(x, y);
}

void lcd_display_on(void) { ltdc_switch(1); }
void lcd_display_off(void) { ltdc_switch(0); }

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color) { LCD_DRAW_POINT_FAST(x, y, color); }

void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;
    ltdc_display_dir(dir);
    lcddev.width = lcdltdc.width;
    lcddev.height = lcdltdc.height;
}

/* ------------------------------------------------------------------ init */
/**
 * @brief  Initialise the LTDC panel.
 * @retval true  = LTDC/layer configured and SDRAM framebuffer verified
 *         false = a HAL call failed or the framebuffer read-back failed
 * @note   sdram_init() must have run before this.
 */
bool lcd_init(void)
{
    lcddev.id = ltdc_panelid_read();

    if (!ltdc_init())
        return false;

    lcd_display_dir(1); /* 800x480 landscape */
    LCD_BL(1);
    lcd_clear(WHITE);
    return true;
}

/* ------------------------------------------------------------------ fills */
void lcd_clear(uint32_t color) { ltdc_clear(color); }

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    ltdc_fill(sx, sy, ex, ey, color); /* single DMA2D register-fill */
}

void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    if (sx >= lcddev.width || sy >= lcddev.height)
        return;
    if (ex >= lcddev.width)
        ex = lcddev.width - 1;
    if (ey >= lcddev.height)
        ey = lcddev.height - 1;
    if (ex < sx || ey < sy)
        return;

    ltdc_color_fill(sx, sy, ex, ey, color); /* single DMA2D M2M blit */
}

/* ------------------------------------------------------------------ lines / shapes */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color)
{
    if (len == 0 || x >= lcddev.width || y >= lcddev.height)
        return;
    uint16_t ex = x + len - 1;
    if (ex >= lcddev.width)
        ex = lcddev.width - 1;
    lcd_fill(x, y, ex, y, color);
}

void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t len, uint32_t color)
{
    if (len == 0 || x >= lcddev.width || y >= lcddev.height)
        return;
    uint16_t ey = y + len - 1;
    if (ey >= lcddev.height)
        ey = lcddev.height - 1;
    lcd_fill(x, y, x, ey, color);
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    /* Fast path: horizontal */
    if (y1 == y2)
    {
        if (x1 > x2)
        {
            uint16_t t = x1;
            x1 = x2;
            x2 = t;
        }
        lcd_fill(x1, y1, x2, y1, color);
        return;
    }
    /* Fast path: vertical */
    if (x1 == x2)
    {
        if (y1 > y2)
        {
            uint16_t t = y1;
            y1 = y2;
            y2 = t;
        }
        lcd_fill(x1, y1, x1, y2, color);
        return;
    }

    /* General: Bresenham */
    int xerr = 0, yerr = 0;
    int delta_x = x2 - x1, delta_y = y2 - y1;
    int incx = (delta_x > 0) ? 1 : (delta_x < 0) ? -1 : 0;
    int incy = (delta_y > 0) ? 1 : (delta_y < 0) ? -1 : 0;

    if (delta_x < 0)
        delta_x = -delta_x;
    if (delta_y < 0)
        delta_y = -delta_y;

    int distance = (delta_x > delta_y) ? delta_x : delta_y;
    int row = x1, col = y1;

    for (int t = 0; t <= distance + 1; t++)
    {
        lcd_draw_point((uint16_t)row, (uint16_t)col, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

void lcd_draw_line_fast(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_draw_line(x1, y1, x2, y2, color);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_fill(x1, y1, x2, y1, color); /* top    */
    lcd_fill(x1, y2, x2, y2, color); /* bottom */
    lcd_fill(x1, y1, x1, y2, color); /* left   */
    lcd_fill(x2, y1, x2, y2, color); /* right  */
}

void lcd_draw_rect_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_fill(x1, y1, x2, y2, color);
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint32_t color)
{
    int a = 0, b = r;
    int di = 3 - (r << 1);

    while (a <= b)
    {
        int pts[8][2] = {{x0 + a, y0 - b}, {x0 + b, y0 - a}, {x0 + b, y0 + a}, {x0 + a, y0 + b},
                         {x0 - a, y0 + b}, {x0 - b, y0 + a}, {x0 - a, y0 - b}, {x0 - b, y0 - a}};

        for (int i = 0; i < 8; i++)
        {
            int px = pts[i][0], py = pts[i][1];
            if (px >= 0 && py >= 0 && px < lcddev.width && py < lcddev.height)
                lcd_draw_point((uint16_t)px, (uint16_t)py, color);
        }
        a++;
        if (di < 0)
            di += 4 * a + 6;
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint32_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    /* centre horizontal span */
    lcd_fill(x - r, y, x + r, y, color);

    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            if (xr > imax)
            {
                lcd_fill(x - i + 1, y + xr, x + i - 1, y + xr, color);
                lcd_fill(x - i + 1, y - xr, x + i - 1, y - xr, color);
            }
            xr--;
        }
        lcd_fill(x - xr, y + i, x + xr, y + i, color);
        lcd_fill(x - xr, y - i, x + xr, y - i, color);
    }
}

void lcd_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_draw_line_fast(x0, y0, x1, y1, color);
    lcd_draw_line_fast(x1, y1, x2, y2, color);
    lcd_draw_line_fast(x2, y2, x0, y0, color);
}

/* name matches the prototype in lcd.h */
void lcd_draw_triangle_filled(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                              uint32_t color)
{
    /* sort vertices by y: a (top) <= b <= c (bottom) */
    int ax = x0, ay = y0, bx = x1, by = y1, cx = x2, cy = y2;
    int tmp;

    if (ay > by)
    {
        tmp = ax;
        ax = bx;
        bx = tmp;
        tmp = ay;
        ay = by;
        by = tmp;
    }
    if (ay > cy)
    {
        tmp = ax;
        ax = cx;
        cx = tmp;
        tmp = ay;
        ay = cy;
        cy = tmp;
    }
    if (by > cy)
    {
        tmp = bx;
        bx = cx;
        cx = tmp;
        tmp = by;
        by = cy;
        cy = tmp;
    }

    int total_h = cy - ay;
    if (total_h == 0)
        return;

    /* upper half: ay -> by */
    int seg1_h = by - ay;
    for (int y = ay; y <= by; y++)
    {
        if (y < 0 || y >= (int)lcddev.height)
            continue;
        int t1 = (seg1_h > 0) ? (y - ay) * (bx - ax) / seg1_h + ax : ax;
        int t2 = (y - ay) * (cx - ax) / total_h + ax;
        if (t1 > t2)
        {
            tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 < 0)
            t1 = 0;
        if (t2 >= (int)lcddev.width)
            t2 = lcddev.width - 1;
        if (t1 <= t2)
            lcd_fill((uint16_t)t1, (uint16_t)y, (uint16_t)t2, (uint16_t)y, color);
    }

    /* lower half: by -> cy */
    int seg2_h = cy - by;
    for (int y = by; y <= cy; y++)
    {
        if (y < 0 || y >= (int)lcddev.height)
            continue;
        int t1 = (seg2_h > 0) ? (y - by) * (cx - bx) / seg2_h + bx : bx;
        int t2 = (y - ay) * (cx - ax) / total_h + ax;
        if (t1 > t2)
        {
            tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 < 0)
            t1 = 0;
        if (t2 >= (int)lcddev.width)
            t2 = lcddev.width - 1;
        if (t1 <= t2)
            lcd_fill((uint16_t)t1, (uint16_t)y, (uint16_t)t2, (uint16_t)y, color);
    }
}

/* old name, kept so existing calls still compile */
void lcd_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    lcd_draw_triangle_filled(x0, y0, x1, y1, x2, y2, color);
}

/* ------------------------------------------------------------------ DMA2D region copy */
void lcd_copy_region(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t w, uint16_t h)
{
    if (w == 0 || h == 0)
        return;

    uint32_t pixsz = lcdltdc.pixsize;
    uint32_t stride = lcdltdc.pwidth; /* physical panel width in pixels */
    uint32_t base = (uint32_t)g_ltdc_framebuf[lcdltdc.activelayer];

    uint32_t src_addr, dst_addr;
    uint16_t src_offline, dst_offline;

    if (lcdltdc.dir) /* landscape: direct mapping */
    {
        src_addr = base + pixsz * (stride * sy + sx);
        dst_addr = base + pixsz * (stride * dy + dx);
        src_offline = stride - w;
        dst_offline = stride - w;
    }
    else /* portrait: swap x/y, mirror */
    {
        src_addr = base + pixsz * (stride * (lcdltdc.pheight - sx - w) + sy);
        dst_addr = base + pixsz * (stride * (lcdltdc.pheight - dx - w) + dy);
        src_offline = stride - h;
        dst_offline = stride - h;
        uint16_t tmp = w;
        w = h;
        h = tmp; /* swap for the rotated framebuffer */
    }

    uint32_t timeout = 0;

    RCC->AHB3ENR |= 1 << 4; /* DMA2D clock */
    DMA2D->CR &= ~(1 << 0); /* stop any running transfer */
    DMA2D->CR = 0 << 16;    /* M2M, no PFC */
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;
    DMA2D->FGOR = src_offline;
    DMA2D->FGMAR = src_addr;
    DMA2D->OPFCCR = LTDC_PIXFORMAT;
    DMA2D->OOR = dst_offline;
    DMA2D->OMAR = dst_addr;
    DMA2D->NLR = (uint32_t)h | ((uint32_t)w << 16);
    DMA2D->CR |= 1 << 0; /* start */

    while ((DMA2D->ISR & (1 << 1)) == 0)
    {
        if (++timeout > 0x1FFFFF)
            break;
    }
    DMA2D->IFCR |= 1 << 1; /* clear transfer-complete flag */
}

/* ------------------------------------------------------------------ text */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint32_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    chr = chr - ' ';

    switch (size)
    {
    case 12:
        pfont = (uint8_t *)asc2_1206[(uint8_t)chr];
        break;
    case 16:
        pfont = (uint8_t *)asc2_1608[(uint8_t)chr];
        break;
    case 24:
        pfont = (uint8_t *)asc2_2412[(uint8_t)chr];
        break;
    case 32:
        pfont = (uint8_t *)asc2_3216[(uint8_t)chr];
        break;
    default:
        return;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                lcd_draw_point(x, y, color);
            else if (mode == 0)
                lcd_draw_point(x, y, g_back_color);

            temp <<= 1;
            y++;

            if (y >= lcddev.height)
                return;

            if ((y - y0) == size)
            {
                y = y0;
                x++;
                if (x >= lcddev.width)
                    return;
                break;
            }
        }
    }
}

static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;

        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2) * t, y, ' ', size, 0, color);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, 0, color);
    }
}

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint32_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;

        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                if (mode & 0X80)
                    lcd_show_char(x + (size / 2) * t, y, '0', size, mode & 0X01, color);
                else
                    lcd_show_char(x + (size / 2) * t, y, ' ', size, mode & 0X01, color);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, mode & 0X01, color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint32_t color)
{
    uint16_t x0 = x; /* was uint8_t: truncated for x > 255 on an 800px panel */

    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
            break;

        lcd_show_char(x, y, *p, size, 1, color);
        x += size / 2;
        p++;
    }
}

/* ------------------------------------------------------------------ double buffering */
void lcd_flush(void)
{
    while (!(LTDC->CDSR & (1 << 0)))
        ;

    LTDC_Layer1->CFBAR = (uint32_t)g_ltdc_framebuf[g_draw_layer];
    LTDC->SRCR = 1 << 0;

    lcdltdc.activelayer = g_draw_layer;
    g_draw_layer ^= 1;

    /* DMA2D M2M copy: new front -> new back */
    RCC->AHB3ENR |= 1 << 4;
    DMA2D->CR &= ~(1 << 0);
    DMA2D->CR = 0 << 16; /* M2M */
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;
    DMA2D->FGOR = 0;
    DMA2D->OOR = 0;
    DMA2D->FGMAR = (uint32_t)g_ltdc_framebuf[lcdltdc.activelayer];
    DMA2D->OMAR = (uint32_t)g_ltdc_framebuf[g_draw_layer];
    DMA2D->NLR = lcdltdc.pheight | (lcdltdc.pwidth << 16);
    DMA2D->CR |= 1 << 0;

    uint32_t timeout = 0;
    while ((DMA2D->ISR & (1 << 1)) == 0)
        if (++timeout > 0x1FFFFF)
            break;
    DMA2D->IFCR |= 1 << 1;
}