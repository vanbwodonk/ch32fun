#include "ch32fun.h"
#include "usbcdc_internal.h"
#include "lib_i2c.h"
#include "ina219.h"

#define I2C_PORT      I2C1
#define I2C_SPEED     100000UL
#define INA_ADDR      0x40

#define I2C_TIMEOUT         5000
#define I2C1_REMAP_VAL      (AFIO_PCFR1_I2C1_REMAP_0 | AFIO_PCFR1_I2C1_REMAP_1)


static void debug(const char *s)
{
    CDC_write_buf(s, (int)strlen(s));
}

int main(void) {

  SystemInit();

  CDC_init();
  Delay_Ms(100);
  while (!CDC_connected());
  Delay_Ms(100);

  debug("\r\r\n\nINA219 test\n\r");

  // remap i2c
  // Must disable SDI on PC18/PC19 first — same as Arduino's GPIO_PinRemapConfig(SWJ_Disable)
  AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;
  // Then set I2C1 remap to PC18(SDA)/PC19(SCL)
  AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | I2C1_REMAP_VAL;

  // Configure PC18/PC19 for I2C (AF_OD 50MHz = 0xF per nibble)
  // Must be before ssd1306_i2c_init() enables I2C1
  GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 8)) | (0xF << 8);
  GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 12)) | (0xF << 12);
  
  // Initialize bit-bang I2C
  i2c_init(I2C_PORT, FUNCONF_SYSTEM_CORE_CLOCK, I2C_SPEED);
  
  // Initialize INA219
  INA_init(I2C_PORT);
  
  // Continuous loop: read and print INA219 data
  while (1) {
    // Read voltage and current
    uint16_t voltage = INA_readVoltage(I2C_PORT);
    int16_t current = INA_readCurrent(I2C_PORT);
    uint16_t power = INA_read(I2C_PORT, INA_REG_POWER);
    
    // Convert to human-readable values
    float voltage_v = (float)voltage / 10.0f;  // LSB = 10mV
    float current_a = (float)current / 320.0f;  // LSB = 320µA
    float power_w = (float)power / 128.0f;  // LSB = 128mW
    
    // Print via USB CDC
    char buf[64];
    mini_snprintf(buf, sizeof(buf), 
        "V=%6.2fV  I=%+5.2fA  P=%6.2fW\r\n",
        voltage_v, current_a, power_w);
    CDC_write_buf(buf, strlen(buf));
    
    // Delay 250ms between updates
    Delay_Ms(250);
  }
}
