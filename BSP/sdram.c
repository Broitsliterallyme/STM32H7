
#include "main.h"
#include "fmc.h"        /* declares: extern SDRAM_HandleTypeDef hsdram1; */
#include "sdram.h"


#define SDRAM_HANDLE    (&hsdram1)
#define SDRAM_TIMEOUT   0xFFFFU

/**
 * @brief       Send a command to the SDRAM
 * @param       bankx   : 0 = command target is BANK5 SDRAM (HAL bank 1)
 *                        1 = command target is BANK6 SDRAM (HAL bank 2)
 * @param       cmd     : 0 normal mode / 1 clock enable / 2 precharge all / 3 auto-refresh
 *                        4 load mode register / 5 self-refresh / 6 power-down
 * @param       refresh : number of auto-refresh cycles (only used when cmd = 3)
 * @param       regval  : mode register value (only used when cmd = 4)
 * @retval      0 = OK, 1 = failed
 */
uint8_t sdram_send_cmd(uint8_t bankx, uint8_t cmd, uint8_t refresh, uint16_t regval)
{
    FMC_SDRAM_CommandTypeDef command;

    command.CommandMode            = (uint32_t)cmd;
    command.CommandTarget          = (bankx == 0) ? FMC_SDRAM_CMD_TARGET_BANK1
                                                  : FMC_SDRAM_CMD_TARGET_BANK2;
    command.AutoRefreshNumber      = (refresh == 0) ? 1U : (uint32_t)refresh;
    command.ModeRegisterDefinition = (uint32_t)regval;

    return (HAL_SDRAM_SendCommand(SDRAM_HANDLE, &command, SDRAM_TIMEOUT) == HAL_OK) ? 0 : 1;
}

/**
 * @brief       Initialize the SDRAM chip (JEDEC power-up sequence)
 * @note        MX_FMC_Init() must already have been called.
 * @param       none
 * @retval      none
 */
uint8_t sdram_init(void)
{
    uint8_t err = 0;
    uint16_t mregval = (2 << 4) | (1 << 9);  /* burst 1, sequential, CAS 2, standard, single write */

    err |= sdram_send_cmd(0, 1, 0, 0);
    HAL_Delay(1);
    err |= sdram_send_cmd(0, 2, 0, 0);
    err |= sdram_send_cmd(0, 3, 8, 0);
    err |= sdram_send_cmd(0, 4, 0, mregval);
    err |= (HAL_SDRAM_ProgramRefreshRate(SDRAM_HANDLE, 839) != HAL_OK);  /* recalculate! */
    return err;
}

/**
 * @brief       Write n bytes into SDRAM starting at (BANK5_SDRAM_ADDR + writeaddr)
 * @param       pbuf        : source buffer
 * @param       writeaddr   : offset inside SDRAM
 * @param       n           : number of bytes
 * @retval      none
 */
void fmc_sdram_write_buffer(uint8_t *pbuf, uint32_t writeaddr, uint32_t n)
{
    for (; n != 0; n--)
    {
        *(volatile uint8_t *)(BANK5_SDRAM_ADDR + writeaddr) = *pbuf;
        writeaddr++;
        pbuf++;
    }
}

/**
 * @brief       Read n bytes from SDRAM starting at (BANK5_SDRAM_ADDR + readaddr)
 * @param       pbuf        : destination buffer
 * @param       readaddr    : offset inside SDRAM
 * @param       n           : number of bytes
 * @retval      none
 */
void fmc_sdram_read_buffer(uint8_t *pbuf, uint32_t readaddr, uint32_t n)
{
    for (; n != 0; n--)
    {
        *pbuf++ = *(volatile uint8_t *)(BANK5_SDRAM_ADDR + readaddr);
        readaddr++;
    }
}