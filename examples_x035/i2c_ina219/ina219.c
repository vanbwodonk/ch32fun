#include "ina219.h"

// Declarations from lib_i2c.h (included by main .c file, not here to avoid duplicate definitions)
u8 i2c_sendBytes(I2C_TypeDef* I2Cx, u8 i2cAddress, u8* buffer, u8 len);
u8 i2c_readRegTx_buffer(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 *tx_buf, u8 tx_len, u8 *rx_buf, u8 rx_len);

void INA_write(I2C_TypeDef* I2Cx, uint8_t reg, uint16_t value) {
  uint8_t buf[3] = { reg, value >> 8, value & 0xFF };
  i2c_sendBytes(I2Cx, INA_ADDR, buf, 3);
}

uint16_t INA_read(I2C_TypeDef* I2Cx, uint8_t reg) {
  uint8_t rx[2];
  i2c_readRegTx_buffer(I2Cx, INA_ADDR, &reg, 1, rx, 2);
  return ((uint16_t)rx[0] << 8) | rx[1];
}

void INA_init(I2C_TypeDef* I2Cx) {
  INA_write(I2Cx, INA_REG_CONFIG, INA_CONFIG);
  INA_write(I2Cx, INA_REG_CALIB, INA_CALIB);
}

uint16_t INA_readVoltage(I2C_TypeDef* I2Cx) {
  return (INA_read(I2Cx, INA_REG_VOLTAGE) >> 1) & 0xFFFC;
}

int16_t INA_readCurrent(I2C_TypeDef* I2Cx) {
  return (int16_t)INA_read(I2Cx, INA_REG_CURRENT);
}
