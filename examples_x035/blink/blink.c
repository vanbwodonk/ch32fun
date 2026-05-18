#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

#define PIN_LED PB12

// Extended pins PC18/PC19 use CFGXR (4 bits per pin: CNF[1:0] | MODE[1:0])
// 0x3 = Out_PP 50MHz, CFGXR nibble positions: PC18@[11:8], PC19@[15:12]
#define PC18_CFG_VAL  (0x3 << 8)
#define PC19_CFG_VAL  (0x3 << 12)
#define PC18_CFG_MASK (0xF << 8)
#define PC19_CFG_MASK (0xF << 12)

static void msg(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

int main()
{
    SystemInit();
    funGpioInitAll();

    // Disable SDI on PC18(SWDIO)/PC19(SWCLK) — same method as Arduino core
    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;

    // Configure PC18, PC19 as push-pull output 50MHz via CFGXR
    GPIOC->CFGXR = (GPIOC->CFGXR & ~(PC18_CFG_MASK | PC19_CFG_MASK)) | PC18_CFG_VAL | PC19_CFG_VAL;

    funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);

    CDC_init();
    while (!CDC_connected());

    msg("CH32X035 PC18/PC19 toggling via AFIO->PCFR1 + CFGXR/BSXR\r\n");

    while (1)
    {
        funDigitalWrite(PIN_LED, FUN_HIGH);
        GPIOC->BSXR = GPIO_BSXR_BS18 | GPIO_BSXR_BS19;
        Delay_Ms(250);

        funDigitalWrite(PIN_LED, FUN_LOW);
        GPIOC->BSXR = GPIO_BSXR_BR18 | GPIO_BSXR_BR19;
        Delay_Ms(250);
    }
}
