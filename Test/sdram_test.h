#ifndef SDRAM_TEST_H
#define SDRAM_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t sdram_test_fail_addr;

#define SDRAM_ERR_DATABUS    1
#define SDRAM_ERR_ADDRBUS    2
#define SDRAM_ERR_BYTELANE   3
#define SDRAM_ERR_UNALIGNED  4
#define SDRAM_ERR_FULL       5

uint32_t sdram_byte_test(void);
uint32_t sdram_quick_test(void);
uint32_t sdram_full_test(void);
uint32_t sdram_light_test(void);

#ifdef __cplusplus
}
#endif

#endif