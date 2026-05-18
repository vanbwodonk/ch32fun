#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

/*
 * Quadrature encoder via EXTI on PA0/PA1
 *
 * Decodes using a 4-state lookup table:
 *   enc_table[prev_state<<2 | cur_state] = step
 *
 * EXTI on both edges → ISR fires 4x per full cycle (same resolution as TIM encoder mode 3).
 * Pros: any 2 GPIO pins, software filter possible.
 * Cons: CPU overhead per edge (~1-2µs at 48MHz).
 */

// Encoder lookup table: [prev][cur] → step
// prev/cur = (PA1 << 1) | PA0, 2-bit gray code
static const int8_t enc_table[16] = {
     0,  1, -1,  0,   // prev=00
    -1,  0,  0,  1,   // prev=01
     1,  0,  0, -1,   // prev=10
     0, -1,  1,  0    // prev=11
};

static volatile int32_t encoder_pos = 0;
static volatile uint32_t last_enc = 0;

// Override weak EXTI7_0 handler from ch32fun
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void)
{
    uint32_t cur = GPIOA->INDR & 3;         // PA0=bit0, PA1=bit1
    uint32_t idx = (last_enc << 2) | cur;
    encoder_pos += enc_table[idx & 0xF];
    last_enc = cur;
    EXTI->INTFR = 0xFF;                     // clear lines 0-7
}

static void msg(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

int main()
{
    char buf[64];
    int32_t prev_pos = 0;

    SystemInit();
    funGpioInitAll();

    // PA0/PA1 as input pull-up
    funPinMode(PA0, GPIO_CFGLR_IN_PUPD);
    funPinMode(PA1, GPIO_CFGLR_IN_PUPD);

    // PA0 = EXTI0, PA1 = EXTI1 — default maps to PORTA, no AFIO config needed
    // Rising + falling edge trigger on both lines
    EXTI->RTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;
    EXTI->FTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;
    // Enable interrupt
    EXTI->INTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;
    // Set NVIC priority and enable
    NVIC_SetPriority(EXTI7_0_IRQn, 0);
    NVIC_EnableIRQ(EXTI7_0_IRQn);

    // Capture initial state
    last_enc = GPIOA->INDR & 3;

    CDC_init();
    Delay_Ms(100);
    while (!CDC_connected());
    Delay_Ms(100);

    msg("EXTI encoder test on PA0/PA1\r\n");
    msg("Uses lookup-table decoder in EXTI7_0 ISR\r\n");

    while (1)
    {
        int32_t pos;

        __disable_irq();
        pos = encoder_pos;
        __enable_irq();

        if (pos != prev_pos)
        {
            prev_pos = pos;
            sprintf(buf, "pos=%6ld\r\n", (long)pos);
            CDC_write_buf(buf, (int)strlen(buf));
        }

        Delay_Ms(50);
    }
}
