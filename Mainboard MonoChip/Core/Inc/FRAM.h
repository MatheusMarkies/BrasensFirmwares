/*
 * FRAM.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */

#ifndef INC_FRAM_H_
#define INC_FRAM_H_

#include "stm32l0xx.h"

#define FRAM_OK 0
#define FRAM_ERROR -1

#define FRAM_I2C_ADDRESS 0x50
#define FRAM_MEMORY_SIZE 131072  // Total memory size in bytes for MB85RS1MT

#define FRAM_MB85RC04 512
#define FRAM_MB85RC16 2048
#define FRAM_MB85RC64T 8192
#define FRAM_MB85RC64V 8192
#define FRAM_MB85RC128A 16384
#define FRAM_MB85RC256V 32768
#define FRAM_MB85RC512T 65536
#define FRAM_MB85RC1MT 131072

typedef struct {
    uint16_t lastSavedAddress;
    uint16_t nextFreeAddress;
    uint32_t memoryAvailable;
} FRAM_Metadata;

HAL_StatusTypeDef FRAM_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef FRAM_WriteByte(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint8_t data);
HAL_StatusTypeDef FRAM_ReadByte(I2C_HandleTypeDef *hi2c, uint16_t memAddress, uint8_t *data);
HAL_StatusTypeDef FRAM_WriteData(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint8_t *data, uint16_t dataSize);
HAL_StatusTypeDef FRAM_ReadData(I2C_HandleTypeDef *hi2c, uint16_t memAddress, uint8_t *data, uint16_t dataSize);
HAL_StatusTypeDef FRAM_WriteStruct(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, void *data, size_t dataSize);
HAL_StatusTypeDef FRAM_ReadStruct(I2C_HandleTypeDef *hi2c, uint16_t memAddress, void *data, size_t dataSize);
HAL_StatusTypeDef FRAM_WriteFloat(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, float value);
HAL_StatusTypeDef FRAM_ReadFloat(I2C_HandleTypeDef *hi2c, uint16_t memAddress, float *value);
HAL_StatusTypeDef FRAM_EraseData(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata, uint16_t memAddress, uint16_t dataSize);
HAL_StatusTypeDef FRAM_Format(I2C_HandleTypeDef *hi2c, FRAM_Metadata *metadata);

uint32_t FRAM_GetTotalMemorySize(void);
void FRAM_InitMetadata(FRAM_Metadata *metadata);
void FRAM_UpdateMetadata(FRAM_Metadata *metadata, uint16_t lastSavedAddress, uint16_t dataSize);

#endif
