#include "touch.h"
#include "lcd.h" /* lcddev.dir / lcddev.width */

_tp_dev tp_dev;

/* ---- pins ---- */
#define SCL(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define RST(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA_IN HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13)

/* ---- ~2 us delay from the DWT cycle counter (HAL_Delay is 1 ms, far too slow) ---- */
static void dly(void)
{
    uint32_t t = DWT->CYCCNT, c = (SystemCoreClock / 1000000U) * 2U;
    while ((DWT->CYCCNT - t) < c)
        ;
}

/* ---- bit-banged I2C ---- */
static void i2c_start(void)
{
    SDA(1);
    SCL(1);
    dly();
    SDA(0);
    dly();
    SCL(0);
}

static void i2c_stop(void)
{
    SDA(0);
    dly();
    SCL(1);
    dly();
    SDA(1);
    dly();
}

static uint8_t i2c_tx(uint8_t d) /* returns 1 if no ACK */
{
    for (uint8_t i = 0; i < 8; i++, d <<= 1)
    {
        SDA(d & 0x80);
        dly();
        SCL(1);
        dly();
        SCL(0);
    }
    SDA(1);
    dly();
    SCL(1);
    dly();
    uint8_t nack = SDA_IN;
    SCL(0);
    dly();
    return nack;
}

static uint8_t i2c_rx(uint8_t ack)
{
    uint8_t r = 0;
    SDA(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        SCL(1);
        dly();
        r = (r << 1) | SDA_IN;
        SCL(0);
        dly();
    }
    SDA(!ack);
    dly();
    SCL(1);
    dly();
    SCL(0);
    SDA(1);
    return r;
}

/* ---- GT9xxx register access (16-bit register address) ---- */
static uint8_t gt_rw(uint16_t reg, uint8_t *b, uint8_t n, uint8_t rd)
{
    uint8_t e;
    i2c_start();
    e = i2c_tx(0x28);
    e |= i2c_tx(reg >> 8);
    e |= i2c_tx(reg & 0xFF);
    if (rd)
    {
        i2c_start();
        e |= i2c_tx(0x29);
        while (n--)
            *b++ = i2c_rx(n != 0);
    }
    else
        while (n--)
            e |= i2c_tx(*b++);
    i2c_stop();
    return e;
}

/* ---- public API ---- */
uint8_t tp_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Speed = GPIO_SPEED_FREQ_MEDIUM;
    g.Pull = GPIO_PULLUP;

    g.Mode = GPIO_MODE_OUTPUT_PP; /* SCL, RST */
    g.Pin = GPIO_PIN_12 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOB, &g);

    g.Mode = GPIO_MODE_OUTPUT_OD; /* SDA: open-drain so it can be read back */
    g.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &g);

    g.Mode = GPIO_MODE_INPUT; /* INT high during reset -> I2C address 0x28/0x29 */
    g.Pin = GPIO_PIN_7;
    HAL_GPIO_Init(GPIOH, &g);

    i2c_stop();
    RST(0);
    HAL_Delay(30);
    RST(1);
    HAL_Delay(30);
    g.Pull = GPIO_NOPULL; /* release INT */
    HAL_GPIO_Init(GPIOH, &g);
    HAL_Delay(100);

    uint8_t v = 0x02; /* soft reset, then normal mode */
    gt_rw(0x8040, &v, 1, 0);
    HAL_Delay(10);
    v = 0x00;
    return gt_rw(0x8040, &v, 1, 0);
}

uint8_t tp_scan(void)
{
    uint8_t s, z = 0, b[8 * TP_MAX];

    gt_rw(0x814E, &s, 1, 1);
    if (!(s & 0x80))
        return tp_dev.n; /* no new frame: keep previous state */

    uint8_t n = s & 0x0F;
    if (n > TP_MAX)
        n = 0;
    if (n)
        gt_rw(0x8150, b, 8 * n, 1);
    gt_rw(0x814E, &z, 1, 0); /* clear the data-ready flag */

    for (uint8_t i = 0; i < n; i++)
    {
        uint16_t px = b[8 * i] | (b[8 * i + 1] << 8);     /* panel-native x (0..799) */
        uint16_t py = b[8 * i + 2] | (b[8 * i + 3] << 8); /* panel-native y (0..479) */
        if (lcddev.dir) /* landscape */
        {
            tp_dev.x[i] = px;
            tp_dev.y[i] = py;
        }
        else /* portrait: same mapping as ltdc pix_addr() */
        {
            tp_dev.x[i] = lcddev.width - 1 - py;
            tp_dev.y[i] = px;
        }
    }
    return tp_dev.n = n;
}