#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "ch32fun.h"

// INA219 I2C address (7-bit)
#define INA_ADDR        0x40

// Register addresses
#define INA_REG_CONFIG  0x00
#define INA_REG_SHUNT   0x01
#define INA_REG_VOLTAGE 0x02
#define INA_REG_POWER   0x03
#define INA_REG_CURRENT 0x04
#define INA_REG_CALIB   0x05

// Configuration: 32V range, ±320mA range with 0.1Ω shunt
// BRNG=1 (32V), PG=11 (±320mV), BADC=1111 (128 samples), SADC=1111 (128 samples), mode=111 (continuous shunt+bus)
#define INA_CONFIG      0b0010011111111111
#define INA_CALIB       4096

void INA_init(I2C_TypeDef* I2Cx);
void INA_write(I2C_TypeDef* I2Cx, uint8_t reg, uint16_t value);
uint16_t INA_read(I2C_TypeDef* I2Cx, uint8_t reg);
uint16_t INA_readVoltage(I2C_TypeDef* I2Cx);
int16_t INA_readCurrent(I2C_TypeDef* I2Cx);

#ifdef __cplusplus
}
#endif
