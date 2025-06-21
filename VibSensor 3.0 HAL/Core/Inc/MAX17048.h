/*
 * MAX17048.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */

#ifndef MAX17048_H
#define MAX17048_H

#include "stm32l0xx_hal.h"

// I2C address of the MAX17048
#define MAX17048_I2C_ADDRESS (0x6C << 1)

// Register addresses
#define MAX17048_REG_VCELL       0x02
#define MAX17048_REG_SOC         0x04
#define MAX17048_REG_MODE        0x06
#define MAX17048_REG_VERSION     0x08
#define MAX17048_REG_HIBRT       0x0A
#define MAX17048_REG_CONFIG      0x0C
#define MAX17048_REG_VALRT       0x14
#define MAX17048_REG_CRATE       0x16
#define MAX17048_REG_VRESET_ID   0x18
#define MAX17048_REG_STATUS      0x1A
#define MAX17048_REG_TABLE       0x40

// Function prototypes
HAL_StatusTypeDef MAX17048_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MAX17048_ReadVoltage(I2C_HandleTypeDef *hi2c, uint16_t *voltage);
HAL_StatusTypeDef MAX17048_ReadSOC(I2C_HandleTypeDef *hi2c, uint16_t *soc);
HAL_StatusTypeDef MAX17048_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value);
HAL_StatusTypeDef MAX17048_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value);

#endif // MAX17048_H

