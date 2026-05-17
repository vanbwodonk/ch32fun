#include "../usbcdc_libs/usbcdc_internal.h"
#include "ch32fun.h"

int main(void)
{
    SystemInit();
    funGpioInitAll();

    CDC_init();

    GPIOB->CFGHR &= ~(0xF << 16);
    GPIOB->CFGHR |=  (0x01 << 16);

    while (!CDC_connected());

    CDC_write_buf("Send 'b' or set 1200 baud + close port to enter bootloader\r\n", 61);

    while (1)
    {
        GPIOB->BSHR = (1 << 12);
        Delay_Ms(250);
        GPIOB->BSHR = (1 << (16+12));
        Delay_Ms(250);

        if (CDC_available() && CDC_read() == 'b')
        {
            /* Manual trigger — pull D+ low then enter bootloader */
            RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
            GPIOC->CFGXR &= ~(0xF << 4);
            GPIOC->CFGXR |=  (0x01 << 4);
            GPIOC->BSXR = (1 << 17);
            for (volatile int i = 0; i < 4000000; i++);
            GPIOC->CFGXR &= ~(0xF << 4);
            GPIOC->CFGXR |=  (0x04 << 4);
            BOOT_now();
            while (1);
        }
    }
}
