/**
 * @file    lcd_ltdc.c
 * @brief   LTDC driver: vendor drawing code on top of a CubeMX/HAL initialisation.
 *
 * Init sequence (see ltdc_init):
 *   1. set the PLL3R pixel clock (33.3 MHz)
 *   2. HAL_LTDC_Init()  -> CubeMX HAL_LTDC_MspInit() configures pins + clock
 *   3. HAL_LTDC_ConfigLayer() with a framebuffer in SDRAM
 *   4. self-test (SDRAM read-back, LTDC + layer enabled)
 *   5. panel reset pulse, backlight on, clear
 */
#include "main.h"
#include "ltdc.h" /* CubeMX generated: hltdc, MX_LTDC_Init */
#include "lcd.h"  /* lcddev */
#include "lcd_ltdc.h"

uint8_t g_draw_layer = 1;
uint32_t *g_ltdc_framebuf[2];
_ltdc_dev lcdltdc;

/* ------------------------------------------------------------------ the panel: 4.3" 800x480 (ID 0x4384) */
#define PANEL_ID  0x4384
#define PANEL_W   800
#define PANEL_H   480
#define PANEL_HSW 48
#define PANEL_HBP 88
#define PANEL_HFP 40
#define PANEL_VSW 3
#define PANEL_VBP 32
#define PANEL_VFP 13
/* pixel clock = 25 MHz / 25 * 300 / 9 = 33.3 MHz (PLL3R) */
#define PANEL_PLL3N 300
#define PANEL_PLL3M 25
#define PANEL_PLL3R 9

/* ------------------------------------------------------------------ simple switches */
void ltdc_switch(uint8_t sw)
{
    if (sw)
        LTDC->GCR |= LTDC_GCR_LTDCEN;
    else
        LTDC->GCR &= ~LTDC_GCR_LTDCEN;
}

void ltdc_layer_switch(uint8_t layerx, uint8_t sw)
{
    LTDC_Layer_TypeDef *l = (layerx == 0) ? LTDC_Layer1 : LTDC_Layer2;

    if (sw)
        l->CR |= LTDC_LxCR_LEN;
    else
        l->CR &= ~LTDC_LxCR_LEN;

    LTDC->SRCR |= LTDC_SRCR_IMR; /* immediate reload */
}

void ltdc_select_layer(uint8_t layerx) { g_draw_layer = layerx; }

void ltdc_display_dir(uint8_t dir)
{
    lcdltdc.dir = dir;

    if (dir == 0)
    {
        lcdltdc.width = lcdltdc.pheight;
        lcdltdc.height = lcdltdc.pwidth;
    } /* portrait  */
    else
    {
        lcdltdc.width = lcdltdc.pwidth;
        lcdltdc.height = lcdltdc.pheight;
    } /* landscape */
}

/* ------------------------------------------------------------------ pixel access */
static inline uint32_t pix_addr(uint16_t x, uint16_t y)
{
    uint32_t base = (uint32_t)g_ltdc_framebuf[g_draw_layer];

    if (lcdltdc.dir)
        return base + lcdltdc.pixsize * (lcdltdc.pwidth * y + x);
    return base + lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y);
}

void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    uint32_t a = pix_addr(x, y);

#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888
    *(volatile uint32_t *)a = color;
#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    *(volatile uint8_t *)(a + 0) = (uint8_t)color;
    *(volatile uint8_t *)(a + 1) = (uint8_t)(color >> 8);
    *(volatile uint8_t *)(a + 2) = (uint8_t)(color >> 16);
#else
    *(volatile uint16_t *)a = (uint16_t)color;
#endif
}

uint32_t ltdc_read_point(uint16_t x, uint16_t y)
{
    uint32_t a = pix_addr(x, y);

#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888
    return *(volatile uint32_t *)a;
#elif LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    return ((uint32_t)*(volatile uint8_t *)(a + 0)) | ((uint32_t)*(volatile uint8_t *)(a + 1) << 8) |
           ((uint32_t)*(volatile uint8_t *)(a + 2) << 16);
#else
    return *(volatile uint16_t *)a;
#endif
}

/* ------------------------------------------------------------------ DMA2D fills */
static void dma2d_wait(void)
{
    uint32_t timeout = 0;

    while ((DMA2D->ISR & DMA2D_ISR_TCIF) == 0)
    {
        if (++timeout > 0x1FFFFFUL)
            break;
    }
    DMA2D->IFCR = DMA2D_IFCR_CTCIF;
}

void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint32_t psx, psy, pex, pey; /* panel-native coordinates */

    if (lcdltdc.dir)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else
    {
        if (ex >= lcdltdc.pheight)
            ex = lcdltdc.pheight - 1;
        if (sx >= lcdltdc.pheight)
            sx = lcdltdc.pheight - 1;

        psx = sy;
        psy = lcdltdc.pheight - ex - 1;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1;
    }

    uint32_t offline = lcdltdc.pwidth - (pex - psx + 1);
    uint32_t addr = (uint32_t)g_ltdc_framebuf[g_draw_layer] + lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx);

    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->CR = (3UL << 16); /* register-to-memory */
    DMA2D->OPFCCR = LTDC_PIXFORMAT;
    DMA2D->OOR = offline;
    DMA2D->OMAR = addr;
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16);
    DMA2D->OCOLR = color;
    DMA2D->CR |= DMA2D_CR_START;

    dma2d_wait();
}

void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint32_t psx, psy, pex, pey;

    if (lcdltdc.dir)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else
    {
        psx = sy;
        psy = lcdltdc.pheight - ex - 1;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1;
    }

    uint32_t offline = lcdltdc.pwidth - (pex - psx + 1);
    uint32_t addr = (uint32_t)g_ltdc_framebuf[g_draw_layer] + lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx);

    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->CR = 0; /* memory-to-memory */
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;
    DMA2D->FGOR = 0;
    DMA2D->OOR = offline;
    DMA2D->FGMAR = (uint32_t)color;
    DMA2D->OMAR = addr;
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16);
    DMA2D->CR |= DMA2D_CR_START;

    dma2d_wait();
}

void ltdc_clear(uint32_t color) { ltdc_fill(0, 0, lcdltdc.width - 1, lcdltdc.height - 1, color); }

/* ------------------------------------------------------------------ pixel clock (PLL3R) */
/* Fdclk = HSE / pll3m * pll3n / pll3r   (HSE = 25 MHz, pll3m = 25 -> 1 MHz VCO input) */
uint8_t ltdc_clk_set(uint32_t pll3n, uint32_t pll3m, uint32_t pll3r)
{
    RCC_PeriphCLKInitTypeDef c = {0};

    c.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    c.PLL3.PLL3M = pll3m;
    c.PLL3.PLL3N = pll3n;
    c.PLL3.PLL3P = 2;
    c.PLL3.PLL3Q = 2;
    c.PLL3.PLL3R = pll3r;
    c.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0; /* 1..2 MHz input */
    c.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE; /* 192..836 MHz VCO */
    c.PLL3.PLL3FRACN = 0;

    return (HAL_RCCEx_PeriphCLKConfig(&c) == HAL_OK) ? 0 : 1;
}

/* ------------------------------------------------------------------ panel ID (fixed) */
uint16_t ltdc_panelid_read(void) { return PANEL_ID; }

/* ------------------------------------------------------------------ self-test */
/* SDRAM write/read-back + LTDC/layer enabled. Runs BEFORE the first clear so no junk is left on screen. */
static bool ltdc_selftest(void)
{
    static const uint16_t pat[4] = {0xA5A5, 0x5A5A, 0xFFFF, 0x0000};
    volatile uint16_t *fb = (volatile uint16_t *)LTDC_FRAME_BUF0_ADDR;

    for (int i = 0; i < 4; i++)
        fb[i] = pat[i];
    __DSB();
    for (int i = 0; i < 4; i++)
        if (fb[i] != pat[i])
            return false;

    if (!(LTDC->GCR & LTDC_GCR_LTDCEN))
        return false; /* controller running */
    if (!(LTDC_Layer1->CR & LTDC_LxCR_LEN))
        return false; /* layer enabled */
    return true;
}

/* ------------------------------------------------------------------ init */
bool ltdc_init(void)
{
    lcdltdc.pwidth = PANEL_W;
    lcdltdc.pheight = PANEL_H;
    lcdltdc.hsw = PANEL_HSW;
    lcdltdc.hbp = PANEL_HBP;
    lcdltdc.hfp = PANEL_HFP;
    lcdltdc.vsw = PANEL_VSW;
    lcdltdc.vbp = PANEL_VBP;
    lcdltdc.vfp = PANEL_VFP;
    lcdltdc.pixsize = LTDC_BYTES_PER_PIXEL;
    lcdltdc.pixformat = LTDC_PIXFORMAT;

    /* reset + backlight pins (idle: panel not in reset, backlight off) */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    LTDC_RST(1);
    LTDC_BL(0);
    GPIO_InitTypeDef g = {0};
    g.Pin = LTDC_RST_GPIO_PIN | LTDC_BL_GPIO_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &g);

    /* pixel clock */
    if (ltdc_clk_set(PANEL_PLL3N, PANEL_PLL3M, PANEL_PLL3R) != 0)
        return false;

    /* if MX_LTDC_Init() already ran (not suppressed in CubeMX), restart cleanly */
    hltdc.Instance = LTDC;
    if (hltdc.State != HAL_LTDC_STATE_RESET)
        HAL_LTDC_DeInit(&hltdc);

    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc.Init.HorizontalSync = PANEL_HSW - 1;
    hltdc.Init.VerticalSync = PANEL_VSW - 1;
    hltdc.Init.AccumulatedHBP = PANEL_HSW + PANEL_HBP - 1;
    hltdc.Init.AccumulatedVBP = PANEL_VSW + PANEL_VBP - 1;
    hltdc.Init.AccumulatedActiveW = PANEL_HSW + PANEL_HBP + PANEL_W - 1;
    hltdc.Init.AccumulatedActiveH = PANEL_VSW + PANEL_VBP + PANEL_H - 1;
    hltdc.Init.TotalWidth = PANEL_HSW + PANEL_HBP + PANEL_W + PANEL_HFP - 1;
    hltdc.Init.TotalHeigh = PANEL_VSW + PANEL_VBP + PANEL_H + PANEL_VFP - 1;
    hltdc.Init.Backcolor.Blue = (LTDC_BACKLAYERCOLOR) & 0xFF;
    hltdc.Init.Backcolor.Green = (LTDC_BACKLAYERCOLOR >> 8) & 0xFF;
    hltdc.Init.Backcolor.Red = (LTDC_BACKLAYERCOLOR >> 16) & 0xFF;

    if (HAL_LTDC_Init(&hltdc) != HAL_OK) /* -> CubeMX HAL_LTDC_MspInit: pins + clock */
        return false;

    /* framebuffers */
    g_ltdc_framebuf[0] = (uint32_t *)LTDC_FRAME_BUF0_ADDR;
    g_ltdc_framebuf[1] = (uint32_t *)LTDC_FRAME_BUF1_ADDR;

    /* layer 1 (index 0), shows buffer 0 */
    LTDC_LayerCfgTypeDef l = {0};
    l.WindowX0 = 0;
    l.WindowX1 = PANEL_W;
    l.WindowY0 = 0;
    l.WindowY1 = PANEL_H;
    l.PixelFormat = LTDC_PIXFORMAT; /* same numeric values as LTDC_PIXEL_FORMAT_xxx */
    l.Alpha = 255;
    l.Alpha0 = 0;
    l.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    l.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    l.FBStartAdress = LTDC_FRAME_BUF0_ADDR;
    l.ImageWidth = PANEL_W;
    l.ImageHeight = PANEL_H;
    l.Backcolor.Blue = l.Backcolor.Green = l.Backcolor.Red = 0;

    if (HAL_LTDC_ConfigLayer(&hltdc, &l, LTDC_LAYER_1) != HAL_OK)
        return false;

    /* verify SDRAM + LTDC state before touching the screen */
    if (!ltdc_selftest())
        return false;

    lcddev.width = lcdltdc.pwidth;
    lcddev.height = lcdltdc.pheight;
    lcdltdc.activelayer = 0;
    ltdc_display_dir(1); /* so the first clear uses valid width/height; lcd_init() sets the final dir */
    ltdc_select_layer(0);

    /* panel reset pulse */
    LTDC_RST(1);
    HAL_Delay(10);
    LTDC_RST(0);
    HAL_Delay(50);
    LTDC_RST(1);
    HAL_Delay(200);

    LTDC_BL(1);
    ltdc_clear(0xFFFFFFFFUL);
    return true;
}