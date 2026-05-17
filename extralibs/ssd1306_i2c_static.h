/*
 * Single-File-Header for SSD1306 I2C using extralibs/static_i2c.h bit-bang
 * Provides the transport layer (ssd1306_pkt_send, ssd1306_rst, ssd1306_i2c_init)
 * using the generic macro-based I2C from static_i2c.h
 */

#ifndef _SSD1306_I2C_STATIC_H
#define _SSD1306_I2C_STATIC_H

#include <stdint.h>
#include "ch32fun.h"

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3c
#endif

#ifndef SSD1306_I2C_SDA
#define SSD1306_I2C_SDA PC1
#endif

#ifndef SSD1306_I2C_SCL
#define SSD1306_I2C_SCL PC2
#endif

#ifndef SSD1306_RST_PIN
#define SSD1306_RST_PIN PC3
#endif

#ifndef I2CSPEEDBASE
#define I2CSPEEDBASE 1
#endif

#ifndef I2CDELAY_FUNC
#define I2CDELAY_FUNC(x) Delay_Us(x)
#endif

/*
 * Macros required by extralibs/static_i2c.h
 * Open-drain emulation: HIGH = input with pull-up, LOW = output push-pull driving 0
 */
#define DELAY1 I2CDELAY_FUNC(1 * I2CSPEEDBASE);
#define DELAY2 I2CDELAY_FUNC(2 * I2CSPEEDBASE);

#define DSCL_IHIGH  { funPinMode( SSD1306_I2C_SCL, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( SSD1306_I2C_SCL, 1 ); }
#define DSDA_IHIGH  { funPinMode( SSD1306_I2C_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( SSD1306_I2C_SDA, 1 ); }
#define DSDA_INPUT  { funPinMode( SSD1306_I2C_SDA, GPIO_CFGLR_IN_PUPD ); funDigitalWrite( SSD1306_I2C_SDA, 1 ); }
#define DSCL_OUTPUT { funDigitalWrite( SSD1306_I2C_SCL, 0 ); funPinMode( SSD1306_I2C_SCL, GPIO_CFGLR_OUT_2Mhz_PP ); }
#define DSDA_OUTPUT { funDigitalWrite( SSD1306_I2C_SDA, 0 ); funPinMode( SSD1306_I2C_SDA, GPIO_CFGLR_OUT_2Mhz_PP ); }
#define READ_DSDA    funDigitalRead( SSD1306_I2C_SDA )

#define I2CNEEDGETBYTE 0
#define I2CPREFIX ssd1306_i2c

#include "static_i2c.h"

/*
 * Initialize I2C GPIO pins (both lines high/impedance = idle bus state)
 */
uint8_t ssd1306_i2c_init(void)
{
	funGpioInitAll();
	ssd1306_i2cConfigI2C();
	return 0;
}

/*
 * Send a packet to the SSD1306 over I2C
 * cmd=1 sends command control byte, cmd=0 sends data control byte
 */
uint8_t ssd1306_pkt_send(const uint8_t *data, int sz, uint8_t cmd)
{
	ssd1306_i2cSendStart();
	if( ssd1306_i2cSendByte( SSD1306_I2C_ADDR << 1 ) )
		return 1;
	if( ssd1306_i2cSendByte( cmd ? 0x00 : 0x40 ) )
		return 1;
	for( int i = 0; i < sz; i++ )
	{
		if( ssd1306_i2cSendByte( data[i] ) )
			return 1;
	}
	ssd1306_i2cSendStop();
	return 0;
}

/*
 * Hardware reset pulse for the SSD1306
 */
void ssd1306_rst(void)
{
	funPinMode( SSD1306_RST_PIN, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( SSD1306_RST_PIN, 0 );
	Delay_Ms(10);
	funDigitalWrite( SSD1306_RST_PIN, 1 );
	Delay_Us(10);
}

#endif
