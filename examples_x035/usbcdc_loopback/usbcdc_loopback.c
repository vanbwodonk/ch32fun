#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

int main()
{
    SystemInit();
    funGpioInitAll();

    CDC_init();

    while (1)
    {
        if (CDC_available())
        {
            char c = CDC_read();
            CDC_write(c);
        }
    }
}
