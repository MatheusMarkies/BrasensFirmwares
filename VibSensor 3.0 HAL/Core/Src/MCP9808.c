/*
 * MCP9808.c
 *
 *  Created on: Jun 9, 2024
 *      Author: Matheus Markies
 */

#include "MCP9808.h"

static HAL_StatusTypeDef MCP9808_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t len);
static HAL_StatusTypeDef MCP9808_WriteRegister_16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value);
static HAL_StatusTypeDef MCP9808_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t len);
static HAL_StatusTypeDef MCP9808_ReadRegister_16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value);

HAL_StatusTypeDef MCP9808_Init(I2C_HandleTypeDef *hi2c) {
    uint16_t manuf_id, device_id;

    if (MCP9808_ReadRegister_16(hi2c, MCP9808_REG_MANUF_ID, &manuf_id) != HAL_OK) {
        return HAL_ERROR;
    }
    if (manuf_id != 0x0054) {
        return HAL_ERROR;
    }

    if (MCP9808_ReadRegister_16(hi2c, MCP9808_REG_DEVICE_ID, &device_id) != HAL_OK) {
        return HAL_ERROR;
    }
    if (device_id != 0x0400) {
        return HAL_ERROR;
    }

    MCP9808_WriteRegister_16(hi2c, MCP9808_REG_CONFIG, 0x0000);

    return HAL_OK;
}

HAL_StatusTypeDef MCP9808_Shutdown(I2C_HandleTypeDef *hi2c) {
    uint16_t conf_register;
    uint16_t conf_shutdown;

    if (MCP9808_ReadRegister_16(hi2c, MCP9808_REG_CONFIG, &conf_register) != HAL_OK) {
        return HAL_ERROR;
    }

    conf_shutdown = conf_register | MCP9808_REG_CONFIG_SHUTDOWN;
    return MCP9808_WriteRegister_16(hi2c, MCP9808_REG_CONFIG, conf_shutdown);
}

HAL_StatusTypeDef MCP9808_Wake(I2C_HandleTypeDef *hi2c) {
    uint16_t conf_register;
    uint16_t conf_shutdown;

    if (MCP9808_ReadRegister_16(hi2c, MCP9808_REG_CONFIG, &conf_register) != HAL_OK) {
        return HAL_ERROR;
    }

    conf_shutdown = conf_register & ~MCP9808_REG_CONFIG_SHUTDOWN;
    return MCP9808_WriteRegister_16(hi2c, MCP9808_REG_CONFIG, conf_shutdown);
}

uint8_t MCP9808_GetResolution(I2C_HandleTypeDef *hi2c) {
  uint8_t data;
  MCP9808_ReadRegister(hi2c, MCP9808_REG_RESOLUTION, &data, 2);
  return data;
}

HAL_StatusTypeDef MCP9808_SetResolution(I2C_HandleTypeDef *hi2c, uint8_t value) {
    return MCP9808_WriteRegister(hi2c, MCP9808_REG_RESOLUTION, &value, 1);
}

HAL_StatusTypeDef MCP9808_ReadTemperature(I2C_HandleTypeDef *hi2c, double *temperature) {
    uint8_t temp_data[2];
    if (MCP9808_ReadRegister(hi2c, MCP9808_REG_TEMP, temp_data, 2) != HAL_OK) {
        return HAL_ERROR;
    }

    uint16_t raw_temp = (temp_data[0] << 8) | temp_data[1];
    raw_temp &= 0x0FFF;  // Máscara para os 12 bits de dados

    double temp = raw_temp & 0x0FFF;
    temp /= 16.0;

    if (raw_temp & 0x1000) {
        temp -= 256.0;
    }

    *temperature = temp;
    return HAL_OK;
}

static HAL_StatusTypeDef MCP9808_WriteRegister_16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
    return HAL_I2C_Master_Transmit(hi2c, MCP9808_I2C_ADDRESS << 1, data, 3, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef MCP9808_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t len) {
    uint8_t buffer[3];
    buffer[0] = reg;
    for (uint8_t i = 0; i < len; i++) {
        buffer[i + 1] = data[i];
    }
    return HAL_I2C_Master_Transmit(hi2c, MCP9808_I2C_ADDRESS << 1, buffer, len + 1, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef MCP9808_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t len) {
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, MCP9808_I2C_ADDRESS << 1, &reg, 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        status = HAL_I2C_Master_Receive(hi2c, MCP9808_I2C_ADDRESS << 1, data, len, HAL_MAX_DELAY);
    }
    return status;
}

static HAL_StatusTypeDef MCP9808_ReadRegister_16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, MCP9808_I2C_ADDRESS << 1, &reg, 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        status = HAL_I2C_Master_Receive(hi2c, MCP9808_I2C_ADDRESS << 1, data, 2, HAL_MAX_DELAY);
        if (status == HAL_OK) {
            *value = (data[0] << 8) | data[1];
        }
    }
    return status;
}
