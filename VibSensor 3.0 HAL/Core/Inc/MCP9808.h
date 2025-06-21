/*
 * MCP9808.h
 *
 *  Created on: Jun 7, 2024
 *      Author: Matheus Markies
 */

#ifndef INC_MCP9808_H_
#define INC_MCP9808_H_

#include "stm32l0xx.h"

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "gpio.h"
#include <math.h>

#define MCP9808_I2C_ADDRESS 0x18 ///< I2C address
#define MCP9808_REG_CONFIG 0x01      ///< MCP9808 config register

#define MCP9808_REG_TUPPER 0x02
#define MCP9808_REG_TLOWER 0x03
#define MCP9808_REG_TCRIT 0x04
#define MCP9808_REG_TEMP 0x05
#define MCP9808_REG_MANUF_ID 0x06
#define MCP9808_REG_DEVICE_ID 0x07
#define MCP9808_REG_RESOLUTION 0x08

#define MCP9808_REG_CONFIG_SHUTDOWN 0x0100   ///< shutdown config
#define MCP9808_REG_CONFIG_CRITLOCKED 0x0080 ///< critical trip lock
#define MCP9808_REG_CONFIG_WINLOCKED 0x0040  ///< alarm window lock
#define MCP9808_REG_CONFIG_INTCLR 0x0020     ///< interrupt clear
#define MCP9808_REG_CONFIG_ALERTSTAT 0x0010  ///< alert output status
#define MCP9808_REG_CONFIG_ALERTCTRL 0x0008  ///< alert output control
#define MCP9808_REG_CONFIG_ALERTSEL 0x0004   ///< alert output select
#define MCP9808_REG_CONFIG_ALERTPOL 0x0002   ///< alert output polarity
#define MCP9808_REG_CONFIG_ALERTMODE 0x0001  ///< alert output mode

HAL_StatusTypeDef MCP9808_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP9808_Shutdown(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP9808_Wake(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP9808_ReadTemperature(I2C_HandleTypeDef *hi2c, double *temperature);
HAL_StatusTypeDef MCP9808_WriteConfig(I2C_HandleTypeDef *hi2c, uint16_t config);
HAL_StatusTypeDef MCP9808_ReadConfig(I2C_HandleTypeDef *hi2c, uint16_t *config);

uint8_t MCP9808_GetResolution(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP9808_SetResolution(I2C_HandleTypeDef *hi2c, uint8_t value);

#endif /* INC_MCP9808_H_ */
