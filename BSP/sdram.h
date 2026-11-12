#ifndef __SDRAM_H
#define __SDRAM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FMC SDRAM bank 1 (SDNE0) base address. Use 0xD0000000UL if the chip is on SDNE1. */
#define BANK5_SDRAM_ADDR    0xC0000000UL

/* Size of sdram chip (32 MB in this case) */
#define SDRAM_SIZE_BYTES    (32UL * 1024UL * 1024UL)

uint8_t sdram_send_cmd(uint8_t bankx, uint8_t cmd, uint8_t refresh, uint16_t regval);
uint8_t sdram_init(void);   /* returns 0 = OK, nonzero = a command failed */

void fmc_sdram_write_buffer(uint8_t *pbuf, uint32_t writeaddr, uint32_t n);
void fmc_sdram_read_buffer(uint8_t *pbuf, uint32_t readaddr, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* __SDRAM_H */