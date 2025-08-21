/*
 * MAX17048.c
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */


#include "MAX17048.h"

HAL_StatusTypeDef MAX17048_Init(I2C_HandleTypeDef *hi2c) {
    uint16_t config = 0x971C;
    return MAX17048_WriteRegister(hi2c, MAX17048_REG_CONFIG, config);
}

HAL_StatusTypeDef MAX17048_ReadVoltage(I2C_HandleTypeDef *hi2c, uint16_t *voltage) {
    return MAX17048_ReadRegister(hi2c, MAX17048_REG_VCELL, voltage);
}

HAL_StatusTypeDef MAX17048_ReadSOC(I2C_HandleTypeDef *hi2c, uint16_t *soc) {
    return MAX17048_ReadRegister(hi2c, MAX17048_REG_SOC, soc);
}

HAL_StatusTypeDef MAX17048_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;

    return HAL_I2C_Master_Transmit(hi2c, MAX17048_I2C_ADDRESS, data, 3, 200);
}

HAL_StatusTypeDef MAX17048_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(hi2c, MAX17048_I2C_ADDRESS, &reg, 1, 200);
    if (status != HAL_OK) {
        return status;
    }

    status = HAL_I2C_Master_Receive(hi2c, MAX17048_I2C_ADDRESS, data, 2, 200);
    if (status == HAL_OK) {
        *value = (data[0] << 8) | data[1];
    }
    return status;
}
