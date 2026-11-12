/**
 * @file    sdram_test.c
 * @brief   SDRAM tests: data bus, address bus, byte lanes, unaligned access, full march test.
 *
 * Return value of every test: 0 = pass, otherwise one of the SDRAM_ERR_* codes.
 * On failure, sdram_test_fail_addr holds the failing absolute address.
 *
 * Add to sdram_test.h:
 *     extern volatile uint32_t sdram_test_fail_addr;
 *     #define SDRAM_ERR_DATABUS   1
 *     #define SDRAM_ERR_ADDRBUS   2
 *     #define SDRAM_ERR_BYTELANE  3
 *     #define SDRAM_ERR_UNALIGNED 4
 *     #define SDRAM_ERR_FULL      5
 *
 * IMPORTANT (F7/H7): either mark the SDRAM region non-cacheable with the MPU, or keep the
 * cache maintenance below. Without one of the two, the tests read back from D-cache and
 * pass even when the SDRAM is broken.
 */
#include <string.h>
#include "main.h"
#include "sdram.h"
#include "common/debug.h"
#include "sdram_test.h"

#ifndef SDRAM_WORDS
#define SDRAM_WORDS   (SDRAM_SIZE_BYTES / 4U)
#endif
#define PROBE_STEP_WORDS   (64UL * 1024UL / 4UL)   /* one probe every 64 KB */
#define BLOCK_WORDS        256U                    /* 1 KB blocks */

#define CHUNK_WORDS   256U     /* cache-maintenance granularity of the full test */

volatile uint32_t sdram_test_fail_addr;

/* ------------------------------------------------------------------ cache helpers */

static inline void cache_sync(const volatile void *addr, uint32_t bytes)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)(uintptr_t)addr, (int32_t)bytes);
#else
    (void)addr; (void)bytes;
#endif
    __DSB();
}

/* write then push to SDRAM and drop the cache line */
static inline void wr32(volatile uint32_t *p, uint32_t v) { *p = v; cache_sync(p, 4); }
static inline void wr8 (volatile uint8_t  *p, uint8_t  v) { *p = v; cache_sync(p, 1); }
/* drop any cached copy first, so the read really goes to the SDRAM */
static inline uint32_t rd32(volatile uint32_t *p) { cache_sync(p, 4); return *p; }

static inline uint32_t mix(uint32_t x)          /* bijective hash: unique data per address */
{
    x ^= x >> 16; x *= 0x7FEB352DU;
    x ^= x >> 15; x *= 0x846CA68BU;
    x ^= x >> 16;
    return x;
}

#define FAIL(code, addr)  do { sdram_test_fail_addr = (uint32_t)(uintptr_t)(addr); return (code); } while (0)

/* ------------------------------------------------------------------ data bus */

static uint32_t test_data_bus(void)
{
    volatile uint32_t *a = (volatile uint32_t *)BANK5_SDRAM_ADDR;
    volatile uint32_t *b = a + 1;

    for (uint32_t bit = 0; bit < 32; bit++) {
        uint32_t v = 1UL << bit;

        wr32(a, v);                 /* walking one  */
        wr32(b, ~v);                /* different address in between: defeats bus-hold readback */
        if (rd32(a) != v)  FAIL(SDRAM_ERR_DATABUS, a);
        if (rd32(b) != ~v) FAIL(SDRAM_ERR_DATABUS, b);

        wr32(a, ~v);                /* walking zero */
        wr32(b, v);
        if (rd32(a) != ~v) FAIL(SDRAM_ERR_DATABUS, a);
        if (rd32(b) != v)  FAIL(SDRAM_ERR_DATABUS, b);
    }
    return 0;
}

/* ------------------------------------------------------------------ address bus */

static uint32_t test_address_bus(void)
{
    volatile uint32_t *p = (volatile uint32_t *)BANK5_SDRAM_ADDR;
    const uint32_t pat = 0xAAAAAAAAU, anti = 0x55555555U;

    for (uint32_t off = 1; off < SDRAM_WORDS; off <<= 1) wr32(&p[off], pat);

    /* stuck-high / shorted address line: writing the base must not disturb any offset */
    wr32(&p[0], anti);
    for (uint32_t off = 1; off < SDRAM_WORDS; off <<= 1)
        if (rd32(&p[off]) != pat) FAIL(SDRAM_ERR_ADDRBUS, &p[off]);

    /* stuck-low / shorted: write the antipattern at each offset, nothing else may change */
    wr32(&p[0], pat);
    for (uint32_t t = 1; t < SDRAM_WORDS; t <<= 1) {
        wr32(&p[t], anti);
        if (rd32(&p[0]) != pat) FAIL(SDRAM_ERR_ADDRBUS, &p[0]);
        for (uint32_t off = 1; off < SDRAM_WORDS; off <<= 1)
            if (off != t && rd32(&p[off]) != pat) FAIL(SDRAM_ERR_ADDRBUS, &p[off]);
        wr32(&p[t], pat);
    }
    return 0;
}

/* ------------------------------------------------------------------ byte lanes (DQM / NBL) */

static uint32_t test_byte_lanes(void)
{
    volatile uint32_t *w32 = (volatile uint32_t *)BANK5_SDRAM_ADDR;
    const uint32_t idx[] = { 2, 3, SDRAM_WORDS / 2, SDRAM_WORDS - 1 };

    for (uint32_t k = 0; k < sizeof(idx) / sizeof(idx[0]); k++) {
        volatile uint32_t *w = &w32[idx[k]];
        volatile uint8_t  *b = (volatile uint8_t *)w;

        /* single byte writes into a zeroed word: neighbours must stay 0 */
        wr32(w, 0);
        uint32_t expect = 0;
        for (uint32_t lane = 0; lane < 4; lane++) {
            uint8_t v = (uint8_t)(0xA1 + lane * 0x11);
            wr8(&b[lane], v);
            expect |= (uint32_t)v << (8 * lane);
            if (rd32(w) != expect) FAIL(SDRAM_ERR_BYTELANE, w);
        }

        /* single byte writes into an all-ones word: neighbours must stay 0xFF */
        wr32(w, 0xFFFFFFFFU);
        expect = 0xFFFFFFFFU;
        for (uint32_t lane = 0; lane < 4; lane++) {
            wr8(&b[lane], 0x00);
            expect &= ~(0xFFUL << (8 * lane));
            if (rd32(w) != expect) FAIL(SDRAM_ERR_BYTELANE, w);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ unaligned buffer access */

uint32_t sdram_byte_test(void)
{
    enum { GUARD = 16, LEN = 61, TOTAL = GUARD + LEN + GUARD };
    const uint32_t base = 0x1000;
    uint8_t guard[TOTAL], w[LEN], r[TOTAL];

    for (uint32_t off = 0; off < 8; off++) {
        memset(guard, 0x5A, sizeof guard);
        for (uint32_t i = 0; i < LEN; i++) w[i] = (uint8_t)(i * 7 + 3 + off * 29); /* differs per pass */

        fmc_sdram_write_buffer(guard, base, TOTAL);                 /* guard fill */
        fmc_sdram_write_buffer(w, base + GUARD + off, LEN);         /* payload at odd / unaligned offset */
        cache_sync((void *)(BANK5_SDRAM_ADDR + base), TOTAL);
        fmc_sdram_read_buffer(r, base, TOTAL);

        if (memcmp(&r[GUARD + off], w, LEN) != 0)
            FAIL(SDRAM_ERR_UNALIGNED, BANK5_SDRAM_ADDR + base + GUARD + off);
        for (uint32_t i = 0; i < TOTAL; i++) {
            if (i >= GUARD + off && i < GUARD + off + LEN) continue;
            if (r[i] != 0x5A) FAIL(SDRAM_ERR_UNALIGNED, BANK5_SDRAM_ADDR + base + i);  /* neighbour clobbered */
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ quick test (seconds-free) */

uint32_t sdram_quick_test(void)
{
    uint32_t e;
    if ((e = test_data_bus()))    return e;
    if ((e = test_address_bus())) return e;
    if ((e = test_byte_lanes()))  return e;
    return 0;
}

/* ------------------------------------------------------------------ full march-style test */
/*  1) up:   w  mix(i)
 *  2) up:   r  mix(i), w ~mix(i)
 *  3) down: r ~mix(i), w  mix(i)
 *  4) up:   r  mix(i)
 *  Unique data per address catches aliasing; the two directions catch coupling faults. */

uint32_t sdram_full_test(void)
{
    volatile uint32_t *p = (volatile uint32_t *)BANK5_SDRAM_ADDR;
    const uint32_t n = SDRAM_WORDS;

    for (uint32_t c = 0; c < n; c += CHUNK_WORDS) {                         /* 1 */
        uint32_t m = (n - c < CHUNK_WORDS) ? n - c : CHUNK_WORDS;
        for (uint32_t i = c; i < c + m; i++) p[i] = mix(i);
        cache_sync(&p[c], m * 4);
    }

    for (uint32_t c = 0; c < n; c += CHUNK_WORDS) {                         /* 2 */
        uint32_t m = (n - c < CHUNK_WORDS) ? n - c : CHUNK_WORDS;
        for (uint32_t i = c; i < c + m; i++) {
            if (p[i] != mix(i)) FAIL(SDRAM_ERR_FULL, &p[i]);
            p[i] = ~mix(i);
        }
        cache_sync(&p[c], m * 4);
    }

    for (uint32_t end = n; end > 0; ) {                                     /* 3 */
        uint32_t m = (end < CHUNK_WORDS) ? end : CHUNK_WORDS;
        uint32_t c = end - m;
        for (uint32_t i = c + m; i-- > c; ) {
            if (p[i] != ~mix(i)) FAIL(SDRAM_ERR_FULL, &p[i]);
            p[i] = mix(i);
        }
        cache_sync(&p[c], m * 4);
        end = c;
    }

    for (uint32_t c = 0; c < n; c += CHUNK_WORDS) {                         /* 4 */
        uint32_t m = (n - c < CHUNK_WORDS) ? n - c : CHUNK_WORDS;
        cache_sync(&p[c], m * 4);
        for (uint32_t i = c; i < c + m; i++)
            if (p[i] != mix(i)) FAIL(SDRAM_ERR_FULL, &p[i]);
    }
    return 0;
}


static uint32_t light_mix(uint32_t x)
{
    x ^= x >> 16; x *= 0x7FEB352DU;
    x ^= x >> 15; x *= 0x846CA68BU;
    x ^= x >> 16;
    return x;
}

uint32_t sdram_light_test(void)
{
    volatile uint32_t *p = (volatile uint32_t *)BANK5_SDRAM_ADDR;
    const uint32_t n = SDRAM_SIZE_BYTES / 4U;
    uint32_t errs = 0;

    /* 1) address-in-address, every 64 KB plus the last word */
    for (uint32_t i = 0; i < n; i += PROBE_STEP_WORDS)
        p[i] = (uint32_t)(uintptr_t)&p[i];
    p[n - 1] = (uint32_t)(uintptr_t)&p[n - 1];

    for (uint32_t i = 0; i < n; i += PROBE_STEP_WORDS) {
        uint32_t exp = (uint32_t)(uintptr_t)&p[i], got = p[i];
        if (got != exp) {
            DBG_INFO("SDRAM addr FAIL @%08lX exp %08lX got %08lX\r\n",
                     (unsigned long)exp, (unsigned long)exp, (unsigned long)got);
            errs++;
        }
    }
    if (p[n - 1] != (uint32_t)(uintptr_t)&p[n - 1]) {
        DBG_INFO("SDRAM addr FAIL @last word\r\n");
        errs++;
    }
    DBG_INFO("SDRAM addr test: %s\r\n", errs ? "FAIL" : "ok");

    /* 2) mixed width: word write -> byte read -> byte write -> word read */
    const uint32_t blocks[] = { 0, n / 4, n / 2, n - BLOCK_WORDS };
    uint32_t e2 = 0;
    for (uint32_t b = 0; b < 4; b++) {
        volatile uint32_t *w = &p[blocks[b]];
        volatile uint8_t  *c = (volatile uint8_t *)w;

        for (uint32_t i = 0; i < BLOCK_WORDS; i++) w[i] = light_mix(blocks[b] + i);

        for (uint32_t i = 0; i < BLOCK_WORDS; i++) {          /* byte reads vs word write */
            uint32_t v = light_mix(blocks[b] + i);
            for (uint32_t k = 0; k < 4; k++)
                if (c[i * 4 + k] != (uint8_t)(v >> (8 * k))) e2++;
        }
        for (uint32_t i = 0; i < BLOCK_WORDS * 4; i++)        /* byte writes, inverted */
            c[i] = (uint8_t)~c[i];
        for (uint32_t i = 0; i < BLOCK_WORDS; i++)            /* word read vs byte writes */
            if (w[i] != ~light_mix(blocks[b] + i)) e2++;
    }
    DBG_INFO("SDRAM byte/word test: %s (%lu errors)\r\n", e2 ? "FAIL" : "ok", (unsigned long)e2);
    errs += e2;

    /* 3) retention across several refresh periods (64 ms each) */
    const uint32_t RET_WORDS = 4096;
    volatile uint32_t *r = &p[n / 8];
    for (uint32_t i = 0; i < RET_WORDS; i++) r[i] = light_mix(0xBEEF0000U + i);
    HAL_Delay(300);
    uint32_t e3 = 0;
    for (uint32_t i = 0; i < RET_WORDS; i++)
        if (r[i] != light_mix(0xBEEF0000U + i)) e3++;
    DBG_INFO("SDRAM retention test: %s (%lu errors)\r\n", e3 ? "FAIL" : "ok", (unsigned long)e3);
    errs += e3;

    DBG_INFO("SDRAM light test: %s\r\n", errs ? "FAILED" : "PASSED");
    return errs;
}