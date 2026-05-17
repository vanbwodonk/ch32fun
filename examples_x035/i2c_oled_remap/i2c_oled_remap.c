/*
 * I2C OLED example for CH32X035F8U6 (UFQFPN20)
 *
 * Uses hardware I2C1 remapped to PC18(SDA) / PC19(SCL)
 * since the default PA10/PA11 are not available on UFQFPN20.
 *
 * WARNING: PC18/PC19 are the SWDIO/SWCLK debug pins.
 * After I2C remap is active, the SWD debug interface is
 * disconnected and you cannot program/debug until a POR.
 */

#include <stdio.h>
#include <string.h>
#include "ch32fun.h"
#include "../usbcdc_libs/usbcdc_internal.h"

/* I2C address */
#define SSD1306_I2C_ADDR    0x3c
#define I2C_TIMEOUT         100000

/* I2C1 Remap 3: PC18=SDA, PC19=SCL  (bits[4:2] = 011) */
#define I2C1_REMAP_VAL      (AFIO_PCFR1_I2C1_REMAP_0 | AFIO_PCFR1_I2C1_REMAP_1)

/* I2C event status masks (STAR1 | STAR2<<16) */
#define EVT_MASTER_MODE_SELECT          ((uint32_t)0x00030001)
#define EVT_MASTER_TRANSMITTER_SELECTED ((uint32_t)0x00070082)
#define EVT_MASTER_BYTE_TRANSMITTED     ((uint32_t)0x00070084)

static void debug(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

static uint8_t chk_evt(uint32_t mask)
{
    uint32_t status = I2C1->STAR1 | (I2C1->STAR2 << 16);
    return (status & mask) == mask;
}

static uint8_t i2c_write(uint8_t addr, const uint8_t *data, int sz)
{
    int32_t to;

    to = I2C_TIMEOUT;
    while ((I2C1->STAR2 & I2C_STAR2_BUSY) && to--);
    if (to < 0) { debug("TIMEOUT: BUSY\r\n"); return 1; }

    I2C1->CTLR1 |= I2C_CTLR1_START;

    to = I2C_TIMEOUT;
    while (!chk_evt(EVT_MASTER_MODE_SELECT) && to--);
    if (to < 0) { debug("TIMEOUT: START\r\n"); return 2; }

    I2C1->DATAR = addr << 1;

    to = I2C_TIMEOUT;
    while (!chk_evt(EVT_MASTER_TRANSMITTER_SELECTED) && to--);
    if (to < 0) { debug("TIMEOUT: ADDR NACK\r\n"); return 3; }

    while (sz--) {
        to = I2C_TIMEOUT;
        while (!(I2C1->STAR1 & I2C_STAR1_TXE) && to--);
        if (to < 0) { debug("TIMEOUT: TXE\r\n"); return 4; }
        I2C1->DATAR = *data++;
    }

    to = I2C_TIMEOUT;
    while (!chk_evt(EVT_MASTER_BYTE_TRANSMITTED) && to--);
    if (to < 0) { debug("TIMEOUT: BTF\r\n"); return 5; }

    I2C1->CTLR1 |= I2C_CTLR1_STOP;
    return 0;
}

static void i2c_init(void)
{
    uint16_t tmp;

    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | I2C1_REMAP_VAL;

    /* PC18(SDA), PC19(SCL): 50MHz AF_PP (same as working HAL ref) */
    GPIOC->CFGXR &= ~(0xF << 8);
    GPIOC->CFGXR |=  (0xB << 8);
    GPIOC->CFGXR &= ~(0xF << 12);
    GPIOC->CFGXR |=  (0xB << 12);

    tmp = I2C1->CTLR2 & ~I2C_CTLR2_FREQ;
    tmp |= (FUNCONF_SYSTEM_CORE_CLOCK / 1000000) & I2C_CTLR2_FREQ;
    I2C1->CTLR2 = tmp;

    I2C1->CKCFGR = (FUNCONF_SYSTEM_CORE_CLOCK / (2 * 100000)) & I2C_CKCFGR_CCR;

    I2C1->CTLR1 |= I2C_CTLR1_PE | I2C_CTLR1_ACK;
}

int main()
{
    SystemInit();
    funGpioInitAll();

    CDC_init();
    while (!CDC_connected());

    debug("I2C1 remap test: PC18=SDA PC19=SCL\r\n");

    i2c_init();
    debug("I2C init done\r\n");

    /* Try to address the OLED */
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