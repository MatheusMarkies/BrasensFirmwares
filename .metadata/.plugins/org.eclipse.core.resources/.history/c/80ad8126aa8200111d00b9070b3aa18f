/*
 * FRAM.c
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */


#include "FRAM.h"

HAL_StatusTypeDef FRAM_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t dummyData = 0x00;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, FRAM_I2C_ADDRESS << 1, &dummyData, 1, 200);
    return status;
}

HAL_StatusTypeDef FRAM_WriteByte(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint8_t data) {
    uint8_t buffer[3];
    buffer[0] = (memAddress >> 8) & 0xFF;
    buffer[1] = memAddress & 0xFF;
    buffer[2] = data;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, FRAM_I2C_ADDRESS << 1, buffer, 3, 200);
    if (status == HAL_OK) {
        FRAM_UpdateMetadata(metadata, memAddress, sizeof(data));
    }
    return status;
}

HAL_StatusTypeDef FRAM_ReadByte(I2C_HandleTypeDef *hi2c, uint16_t memAddress, uint8_t *data) {
    uint8_t buffer[2];
    buffer[0] = (memAddress >> 8) & 0xFF;
    buffer[1] = memAddress & 0xFF;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, FRAM_I2C_ADDRESS << 1, buffer, 2, 200);
    if (status == HAL_OK) {
        status = HAL_I2C_Master_Receive(hi2c, FRAM_I2C_ADDRESS << 1, data, 1, 200);
    }
    return status;
}

HAL_StatusTypeDef FRAM_WriteData(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint8_t *data, uint16_t dataSize) {
    if (dataSize + 2 > 256) return HAL_ERROR;
    uint8_t buffer[256];

    buffer[0] = (memAddress >> 8) & 0xFF;
    buffer[1] = memAddress & 0xFF;
    memcpy(&buffer[2], data, dataSize);
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, FRAM_I2C_ADDRESS << 1, buffer, dataSize + 2, 200);
    if (status == HAL_OK && metadata != NULL) {
        FRAM_UpdateMetadata(metadata, memAddress, dataSize);
    }
    return status;
}

HAL_StatusTypeDef FRAM_ReadData(I2C_HandleTypeDef *hi2c, uint16_t memAddress, uint8_t *data, uint16_t dataSize) {
    uint8_t buffer[2];
    buffer[0] = (memAddress >> 8) & 0xFF;
    buffer[1] = memAddress & 0xFF;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, FRAM_I2C_ADDRESS << 1, buffer, 2, 200);
    if (status == HAL_OK) {
        status = HAL_I2C_Master_Receive(hi2c, FRAM_I2C_ADDRESS << 1, data, dataSize, 200);
    }
    return status;
}

HAL_StatusTypeDef FRAM_WriteStruct(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, void *data, size_t dataSize) {
    return FRAM_WriteData(hi2c, metadata, memAddress, (uint8_t *)data, dataSize);
}

HAL_StatusTypeDef FRAM_ReadStruct(I2C_HandleTypeDef *hi2c, uint16_t memAddress, void *data, size_t dataSize) {
    return FRAM_ReadData(hi2c, memAddress, (uint8_t *)data, dataSize);
}

HAL_StatusTypeDef FRAM_WriteFloat(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, float value) {
    return FRAM_WriteStruct(hi2c, metadata, memAddress, &value, sizeof(float));
}

HAL_StatusTypeDef FRAM_ReadFloat(I2C_HandleTypeDef *hi2c, uint16_t memAddress, float *value) {
    return FRAM_ReadStruct(hi2c, memAddress, value, sizeof(float));
}

HAL_StatusTypeDef FRAM_EraseData(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint16_t dataSize) {
    uint8_t *buffer = malloc(dataSize);
    if (buffer == NULL) return HAL_ERROR;
    memset(buffer, 0xFF, dataSize);
    HAL_StatusTypeDef status = FRAM_WriteData(hi2c, NULL, memAddress, buffer, dataSize);

    FRAM_UpdateMetadata(metadata,  memAddress - dataSize,  0);

    free(buffer);
    return status;
}

HAL_StatusTypeDef FRAM_Format(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata) {
    HAL_StatusTypeDef status = FRAM_EraseData(hi2c, metadata, 0, FRAM_MEMORY_SIZE);
    if (status == HAL_OK) {
        FRAM_InitMetadata(metadata);
    }

    FRAM_UpdateMetadata(metadata,  0,  0);

    return status;
}

uint32_t FRAM_GetTotalMemorySize(void) {
    return FRAM_MEMORY_SIZE;
}

void FRAM_InitMetadata(FRAM_Metadata *metadata) {
    metadata->lastSavedAddress = 0;
    metadata->nextFreeAddress = 0;
    metadata->memoryAvailable = FRAM_MEMORY_SIZE;
}

void FRAM_UpdateMetadata(FRAM_Metadata *metadata, uint16_t lastSavedAddress, uint16_t dataSize) {
    if (metadata) {
        metadata->lastSavedAddress = lastSavedAddress;
        metadata->nextFreeAddress = lastSavedAddress + dataSize;
        metadata->memoryAvailable = FRAM_MEMORY_SIZE - metadata->nextFreeAddress;
    }
}
