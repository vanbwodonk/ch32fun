#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

#define TIM2_CCMR1_CC1S_INPUT_TI1  ((uint16_t)0x0001)
#define TIM2_CCMR1_CC2S_INPUT_TI2  ((uint16_t)0x0100)

static void msg(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

int main()
{
    int16_t prev_pos = 0;
    char buf[64];

    SystemInit();
    funGpioInitAll();

    // Enable TIM2 clock
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    // PA0 (TIM2_CH1) and PA1 (TIM2_CH2) as input pull-up with filter
    funPinMode(PA0, GPIO_CFGLR_IN_PUPD);
    funPinMode(PA1, GPIO_CFGLR_IN_PUPD);

    // Disable TIM2 while configuring
    TIM2->CTLR1 = 0;

    // CC1 = input, TI1 mapped to CH1
    TIM2->CHCTLR1 = (TIM2->CHCTLR1 & ~TIM_CC1S) | TIM2_CCMR1_CC1S_INPUT_TI1;
    // CC2 = input, TI2 mapped to CH2
    TIM2->CHCTLR1 = (TIM2->CHCTLR1 & ~TIM_CC2S) | TIM2_CCMR1_CC2S_INPUT_TI2;

    // Input capture filter IC1F/IC2F = 0b1111: fSAMPLING=fDTS/32, N=8
    // At 48MHz: 48MHz/32=1.5MHz, 8 samples = ~5.3µs debounce
    // If still bouncy, add external 10k pull-ups or use IC1PSC prescaler
    TIM2->CHCTLR1 |= (0xF << 4) | (0xF << 12);

    // Encoder mode 3: count on both TI1 and TI2 edges
    TIM2->SMCFGR = (TIM2->SMCFGR & ~TIM_SMS) | (3 << 0);

    // ARR max for full 16-bit range
    TIM2->ATRLR = 0xFFFF;
    // Center position for signed reading
    TIM2->CNT = 0x8000;
    TIM2->PSC = 0;

    // Enable counter
    TIM2->CTLR1 |= TIM_CEN;

    CDC_init();
    Delay_Ms(100);
    while (!CDC_connected());
    Delay_Ms(100);

    msg("TIM2 encoder mode test\r\n");
    msg("PA0(TIM2_CH1) PA1(TIM2_CH2)\r\n");
    sprintf(buf, "ARR=%u PSC=%u\r\n", TIM2->ATRLR, TIM2->PSC);
    CDC_write_buf(buf, (int)strlen(buf));

    while (1)
    {
        int16_t pos = (int16_t)(TIM2->CNT - 0x8000);
        int16_t diff = pos - prev_pos;
        if (diff)
        {
            prev_pos = pos;
            sprintf(buf, "pos=%6d diff=%4d\r\n", pos, diff);
            CDC_write_buf(buf, (int)strlen(buf));
        }
        Delay_Ms(50);
    }
}
