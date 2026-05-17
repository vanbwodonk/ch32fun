/*
 * I2C OLED - using WuxiProject helper library's exact init sequence
 * ported to direct register access.
 */
#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

#define SSD1306_I2C_ADDR    0x3c
#define I2C_TIMEOUT         100000

static void debug(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

/* --- ported from WuxiProject I2C_BusInit() for REMAP_3 --- */
static void i2c_init(void)
{
    /* Enable clocks */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    /* Remap I2C1 to PC18(SDA)/PC19(SCL) — same as GPIO_PartialRemap3_I2C1 */
    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | (AFIO_PCFR1_I2C1_REMAP_0 | AFIO_PCFR1_I2C1_REMAP_1);

    /* PC18/PC19 as AF_PP 50MHz — same as GPIO_Init with GPIO_Mode_AF_PP */
    GPIOC->CFGXR &= ~(0xF << 8);
    GPIOC->CFGXR |=  (0xB << 8);
    GPIOC->CFGXR &= ~(0xF << 12);
    GPIOC->CFGXR |=  (0xB << 12);

    /* I2C init (same as I2C_Init) */
    I2C1->CTLR1 &= ~I2C_CTLR1_PE;

    uint16_t freq = (FUNCONF_SYSTEM_CORE_CLOCK / 1000000) & I2C_CTLR2_FREQ;
    I2C1->CTLR2 = (I2C1->CTLR2 & ~I2C_CTLR2_FREQ) | freq;

    I2C1->CKCFGR = (FUNCONF_SYSTEM_CORE_CLOCK / (2 * 100000)) & I2C_CKCFGR_CCR;

    I2C1->OADDR1 = 0x4001;  /* 7-bit mode, own addr 1 (unused in master) */

    I2C1->CTLR1 |= I2C_CTLR1_PE;
    I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

/* --- event check + write (same logic as helper library) --- */
static uint8_t chk_evt(uint32_t mask)
{
    uint32_t status = I2C1->STAR1 | (I2C1->STAR2 << 16);
    return (status & mask) == mask;
}

static uint8_t i2c_write(uint8_t addr, const uint8_t *data, int sz)
{
    int32_t to;

    /* Wait for bus free */
    to = I2C_TIMEOUT;
    while ((I2C1->STAR2 & I2C_STAR2_BUSY) && to--);
    if (to < 0) { debug("ERR: BUSY\r\n"); return 1; }

    I2C1->CTLR1 |= I2C_CTLR1_START;

    to = I2C_TIMEOUT;
    while (!chk_evt(0x00030001) && to--);
    if (to < 0) { debug("ERR: START\r\n"); return 2; }

    I2C1->DATAR = addr << 1;

    to = I2C_TIMEOUT;
    while (!chk_evt(0x00070082) && to--);
    if (to < 0) { debug("ERR: ADDR NACK\r\n"); return 3; }

    while (sz--) {
        to = I2C_TIMEOUT;
        while (!(I2C1->STAR1 & I2C_STAR1_TXE) && to--);
        if (to < 0) { debug("ERR: TXE\r\n"); return 4; }
        I2C1->DATAR = *data++;
    }

    to = I2C_TIMEOUT;
    while (!chk_evt(0x00070084) && to--);
    if (to < 0) { debug("ERR: BTF\r\n"); return 5; }

    I2C1->CTLR1 |= I2C_CTLR1_STOP;
    return 0;
}

int main()
{
    SystemInit();
    funGpioInitAll();

    CDC_init();
    while (!CDC_connected());

    debug("I2C1 remap PC18=SDA PC19=SCL (WuxiProject method)\r\n");

    i2c_init();
    debug("I2C init done\r\n");

    /* Send display-off command to OLED */
    uint8_t cmd[] = { 0xAE };
    uint8_t ret = i2c_write(SSD1306_I2C_ADDR, cmd, 1);

    char buf[64];
    int n = sprintf(buf, "i2c_write returned %d\r\n", ret);
    CDC_write_buf(buf, n);

    /* PB12 heartbeat */
    GPIOB->CFGHR &= ~(0xF << 16);
    GPIOB->CFGHR |=  (0x01 << 16);

    while (1)
    {
        GPIOB->BSHR = (1 << 12);
        Delay_Ms(500);
        GPIOB->BSHR = (1 << (16+12));
        Delay_Ms(500);
    }
}
