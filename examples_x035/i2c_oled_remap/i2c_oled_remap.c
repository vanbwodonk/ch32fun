/*
 * Example for using I2C with 128x32 graphic OLED
 * 03-29-2023 E. Brombaugh
 */

// what type of OLED - uncomment just one
// #define SSD1306_64X32
// #define SSD1306_72X40
// #define SSD1306_128X32
#define SSD1306_128X64

#include "ch32fun.h"
#include <stdio.h>
#include "ssd1306_i2c.h"
#include "ssd1306.h"

#include "bomb.h"
#include "../usbcdc_libs/usbcdc_internal.h"

#define I2C_TIMEOUT         5000
#define I2C1_REMAP_VAL      (AFIO_PCFR1_I2C1_REMAP_0 | AFIO_PCFR1_I2C1_REMAP_1)

static void debug(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

// static void debug_buf(char *buf, int len)
// {
//     CDC_write_buf(buf, len);
// }

int main()
{
	// 48MHz internal clock
	SystemInit();

  CDC_init();
  Delay_Ms(100);
  while (!CDC_connected());
  Delay_Ms(100);

	debug("\r\r\n\ni2c_oled example\n\r");

	// init i2c and oled
	Delay_Ms( 100 );	// give OLED some more time

  // remap i2c
  // Must disable SDI on PC18/PC19 first — same as Arduino's GPIO_PinRemapConfig(SWJ_Disable)
  AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;
  // Then set I2C1 remap to PC18(SDA)/PC19(SCL)
  AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | I2C1_REMAP_VAL;

  // Configure PC18/PC19 for I2C (AF_OD 50MHz = 0xF per nibble)
  // Must be before ssd1306_i2c_init() enables I2C1
  GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 8)) | (0xF << 8);
  GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 12)) | (0xF << 12);

	debug("initializing i2c oled...");
	if(!ssd1306_i2c_init())
	{
		ssd1306_init();
		debug("done.\n\r");
		
		debug("Looping on test modes...");
		while(1)
		{
			for(uint8_t mode=0;mode<(SSD1306_H>32?9:8);mode++)
			{
				// clear buffer for next mode
				ssd1306_setbuf(0);

				switch(mode)
				{
					case 0:
						debug("buffer fill with binary\n\r");
						for(int i=0;i<sizeof(ssd1306_buffer);i++)
							ssd1306_buffer[i] = i;
						break;
					
					case 1:
						debug("pixel plots\n\r");
						for(int i=0;i<SSD1306_W;i++)
						{
							ssd1306_drawPixel(i, i/(SSD1306_W/SSD1306_H), 1);
							ssd1306_drawPixel(i, SSD1306_H-1-(i/(SSD1306_W/SSD1306_H)), 1);
						}
						break;
					
					case 2:
						{
							debug("Line plots\n\r");
							uint8_t y= 0;
							for(uint8_t x=0;x<SSD1306_W;x+=16)
							{
								ssd1306_drawLine(x, 0, SSD1306_W, y, 1);
								ssd1306_drawLine(SSD1306_W-x, SSD1306_H, 0, SSD1306_H-y, 1);
								y+= SSD1306_H/8;
							}
						}
						break;
						
					case 3:
						debug("Circles empty and filled\n\r");
						for(uint8_t x=0;x<SSD1306_W;x+=16)
							if(x<64)
								ssd1306_drawCircle(x, SSD1306_H/2, 15, 1);
							else
								ssd1306_fillCircle(x, SSD1306_H/2, 15, 1);
						break;
					case 4:
						debug("Image\n\r");
						ssd1306_drawImage(0, 0, bomb_i_stripped, 32, 32, 0);
						break;
					case 5:
						debug("Unscaled Text\n\r");
						ssd1306_drawstr(0,0, "This is a test", 1);
						ssd1306_drawstr(0,8, "of the emergency", 1);
						ssd1306_drawstr(0,16,"broadcasting", 1);
						ssd1306_drawstr(0,24,"system.",1);
						if(SSD1306_H>32)
						{
							ssd1306_drawstr(0,32, "Lorem ipsum", 1);
							ssd1306_drawstr(0,40, "dolor sit amet,", 1);
							ssd1306_drawstr(0,48,"consectetur", 1);
							ssd1306_drawstr(0,56,"adipiscing",1);
						}
						ssd1306_xorrect(SSD1306_W/2, 0, SSD1306_W/2, SSD1306_W);
						break;
						
					case 6:
						debug("Scaled Text 1, 2\n\r");
						ssd1306_drawstr_sz(0,0, "sz 8x8", 1, fontsize_8x8);
						ssd1306_drawstr_sz(0,16, "16x16", 1, fontsize_16x16);
						break;
					
					case 7:
						debug("Scaled Text 4\n\r");
						ssd1306_drawstr_sz(0,0, "32x32", 1, fontsize_32x32);
						break;
					
					
					case 8:
						debug("Scaled Text 8\n\r");
						ssd1306_drawstr_sz(0,0, "64", 1, fontsize_64x64);
						break;

					default:
						break;
				}
				ssd1306_refresh();
			
				Delay_Ms(2000);
			}
		}
	}
	else
		debug("failed.\n\r");
	
	debug("Stuck here forever...\n\r");
	while(1);
}