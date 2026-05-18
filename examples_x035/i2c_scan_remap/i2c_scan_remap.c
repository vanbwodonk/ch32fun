/*
 * I2C address scanner on PC18(SDA)/PC19(SCL) via I2C1 remap
 * Reports results over USB CDC serial.
 */
#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

#define I2C_TIMEOUT         5000
#define I2C1_REMAP_VAL      (AFIO_PCFR1_I2C1_REMAP_0 | AFIO_PCFR1_I2C1_REMAP_1)

static void debug(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

static void debug_buf(char *buf, int len)
{
    CDC_write_buf(buf, len);
}

static uint8_t chk_evt(uint32_t mask)
{
    uint32_t status = I2C1->STAR1 | (I2C1->STAR2 << 16);
    return (status & mask) == mask;
}

/* Try addressing a device, returns 0 on ACK */
static uint8_t i2c_probe(uint8_t addr)
{
    int32_t to;

    to = I2C_TIMEOUT;
    while ((I2C1->STAR2 & I2C_STAR2_BUSY) && to--);
    if (to < 0) return 1;

    I2C1->CTLR1 |= I2C_CTLR1_START;

    to = I2C_TIMEOUT;
    while (!chk_evt(0x00030001) && to--);
    if (to < 0) { I2C1->CTLR1 |= I2C_CTLR1_STOP; return 2; }

    I2C1->DATAR = addr << 1;

    to = I2C_TIMEOUT;
    while (!chk_evt(0x00070082) && to--);
    if (to < 0) { I2C1->CTLR1 |= I2C_CTLR1_STOP; return 3; }

    /* Got ACK, send STOP */
    I2C1->CTLR1 |= I2C_CTLR1_STOP;
    return 0;
}

static void i2c_init(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    // Must disable SDI on PC18/PC19 first — same as Arduino's GPIO_PinRemapConfig(SWJ_Disable)
    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;
    // Then set I2C1 remap to PC18(SDA)/PC19(SCL)
    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | I2C1_REMAP_VAL;

    GPIOC->CFGXR &= ~(0xF << 8);
    GPIOC->CFGXR |=  (0xB << 8);
    GPIOC->CFGXR &= ~(0xF << 12);
    GPIOC->CFGXR |=  (0xB << 12);

    uint16_t tmp = I2C1->CTLR2 & ~I2C_CTLR2_FREQ;
    tmp |= (FUNCONF_SYSTEM_CORE_CLOCK / 1000000) & I2C_CTLR2_FREQ;
    I2C1->CTLR2 = tmp;

    I2C1->CKCFGR = (FUNCONF_SYSTEM_CORE_CLOCK / (2 * 100000)) & I2C_CKCFGR_CCR;
    I2C1->OADDR1 = 0x4001;
    I2C1->CTLR1 |= I2C_CTLR1_PE | I2C_CTLR1_ACK;
}

int main()
{
    SystemInit();
    funGpioInitAll();
    CDC_init();
    Delay_Ms(100);
    while (!CDC_connected());
    Delay_Ms(100);

    debug("I2C scan PC18=SDA PC19=SCL\r\n");

    i2c_init();

    GPIOB->CFGHR &= ~(0xF << 16);
    GPIOB->CFGHR |=  (0x01 << 16);

    int found = 0;
    for (uint8_t addr = 1; addr < 128; addr++)
    {
        uint8_t ret = i2c_probe(addr);
        if (ret == 0)
        {
            char buf[32];
            int n = sprintf(buf, "  Found 0x%02X (7-bit)\r\n", addr);
            debug_buf(buf, n);
            found++;
        }

        if (addr % 16 == 0)
        {
            GPIOB->BSHR = (1 << 12);
            Delay_Ms(50);
            GPIOB->BSHR = (1 << (16+12));
        }
    }

    char res[64];
    int n = sprintf(res, "Scan complete. %d device(s) found.\r\n", found);
    debug_buf(res, n);

    if (found == 0)
        debug("No I2C devices detected. I2C1 remap may not work.\r\n");

    while (1)
    {
        GPIOB->BSHR = (1 << 12);
        Delay_Ms(500);
        GPIOB->BSHR = (1 << (16+12));
        Delay_Ms(500);
    }
}
