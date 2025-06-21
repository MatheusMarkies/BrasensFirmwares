/*
 * KX122.c
 *
 *  Created on: Jun 7, 2024
 *      Author: Matheus Markies
 */
#include "KX122.h"

static HAL_StatusTypeDef KX122_WriteRegister(I2C_HandleTypeDef *hi2c,
		uint8_t reg, uint8_t value);
static HAL_StatusTypeDef KX122_ReadRegister(I2C_HandleTypeDef *hi2c,
		uint8_t reg, uint8_t *data);

HAL_StatusTypeDef KX122_Init(I2C_HandleTypeDef *hi2c) {
    if (KX122_WriteRegister(hi2c, 0x18, 0x00) != HAL_OK) return HAL_ERROR;
    if (KX122_WriteRegister(hi2c, 0x18, 0x80) != HAL_OK) return HAL_ERROR;
    if (KX122_WriteRegister(hi2c, 0x19, 0x00) != HAL_OK) return HAL_ERROR;
    if (KX122_WriteRegister(hi2c, 0x1A, 0x00) != HAL_OK) return HAL_ERROR;
    if (KX122_WriteRegister(hi2c, KX122_ODR_SPEED, 0x0F) != HAL_OK) return HAL_ERROR;
    return HAL_OK;
}

Vibration KX122_ReadAccelData(I2C_HandleTypeDef *hi2c) {
    uint8_t data[6];
    Vibration vib = {0};

    if (KX122_ReadRegister(hi2c, KX122_XOUT_L, &data[0]) != HAL_OK) return vib;
    if (KX122_ReadRegister(hi2c, KX122_XOUT_H, &data[1]) != HAL_OK) return vib;
    if (KX122_ReadRegister(hi2c, KX122_YOUT_L, &data[2]) != HAL_OK) return vib;
    if (KX122_ReadRegister(hi2c, KX122_YOUT_H, &data[3]) != HAL_OK) return vib;
    if (KX122_ReadRegister(hi2c, KX122_ZOUT_L, &data[4]) != HAL_OK) return vib;
    if (KX122_ReadRegister(hi2c, KX122_ZOUT_H, &data[5]) != HAL_OK) return vib;

    int16_t raw_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t raw_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t raw_z = (int16_t)((data[5] << 8) | data[4]);

    // Converting raw data to Gs (assuming default sensitivity)
    double sensitivity = 0.000061; // 61 μg/LSB for ±2g range
    vib.x = raw_x * sensitivity;
    vib.y = raw_y * sensitivity;
    vib.z = raw_z * sensitivity;

    return vib;
}

static HAL_StatusTypeDef KX122_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value) {
    uint8_t data[2] = { reg, value };
    return HAL_I2C_Master_Transmit(hi2c, KX122_I2C_ADDRESS << 1, data, 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef KX122_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data) {
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, KX122_I2C_ADDRESS << 1, &reg, 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        status = HAL_I2C_Master_Receive(hi2c, KX122_I2C_ADDRESS << 1, data, 1, HAL_MAX_DELAY);
    }
    return status;
}
