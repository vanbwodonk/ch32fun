#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

static const char msg[] = "Hello World\r\n";

int main()
{
    SystemInit();
    funGpioInitAll();
    CDC_init();

    while (1)
    {
        Delay_Ms(1000);
        CDC_write_buf(msg, sizeof(msg) - 1);
    }
}