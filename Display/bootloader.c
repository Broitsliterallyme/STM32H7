#include "main.h"
#include "bootloader.h"

#define SYSTEM_BOOTLOADER_ADDR  0x1FF09800UL
#define BOOT_MAGIC              0xB007B007UL

static void bkp_access_enable(void)
{
    RCC->APB4ENR |= RCC_APB4ENR_RTCAPBEN;   /* clock for RTC register access */
    (void)RCC->APB4ENR;                      /* dummy read: clock-enable delay */
    HAL_PWR_EnableBkUpAccess();              /* sets PWR_CR1.DBP */
}

/* Call from your main loop when g_enter_bootloader is set */
void request_bootloader(void)
{
    bkp_access_enable();
    RTC->BKP0R = BOOT_MAGIC;
    __DSB();
    NVIC_SystemReset();
}

/* Call as the very first thing in main(), before HAL_Init() */
void bootloader_check(void)
{
    bkp_access_enable();

    if (RTC->BKP0R != BOOT_MAGIC)
        return;

    RTC->BKP0R = 0;                          /* clear so the next reset boots normally */

    uint32_t sp = *(volatile uint32_t *)SYSTEM_BOOTLOADER_ADDR;
    uint32_t pc = *(volatile uint32_t *)(SYSTEM_BOOTLOADER_ADDR + 4);

    __set_MSP(sp);
    ((void (*)(void))pc)();
    while (1);
}