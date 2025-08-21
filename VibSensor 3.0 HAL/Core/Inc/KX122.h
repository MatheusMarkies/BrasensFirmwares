/*
 * KX122.h
 *
 *  Created on: Jun 7, 2024
 *      Author: Matheus Markies
 */

#ifndef SRC_KX122_H_
#define SRC_KX122_H_

#include "stm32l0xx.h"

#include <stdint.h>
#include <stdbool.h>

#define ACC_DATA_RATE 25800

#define KX122_I2C_ADDRESS 0x1F
#define KX122_ODR_SPEED 0x1B

#define KX122_XOUT_L 0x06
#define KX122_XOUT_H 0x07

#define KX122_YOUT_L 0x08
#define KX122_YOUT_H 0x09

#define KX122_ZOUT_L 0x0A
#define KX122_ZOUT_H 0x0B

#define ODR_25600HZ 0x0F
#define SENSITIVITY_2G (0.000061)

#define BUF_CNTL1 0x3A
#define BUF_CNTL2 0x3B
#define BUF_STATUS_1 0x3C
#define BUF_STATUS_2 0x3D
#define BUF_CLEAR 0x3E
#define BUF_READ 0x3F

typedef struct {
  float x;
  float y;
  float z;
} Vibration;

HAL_StatusTypeDef KX122_Init(I2C_HandleTypeDef *hi2c);
Vibration KX122_ReadAccelData(I2C_HandleTypeDef *hi2c);
int8_t KX122_GetBufferSize(I2C_HandleTypeDef *hi2c);

#endif /* SRC_KX122_H_ */
