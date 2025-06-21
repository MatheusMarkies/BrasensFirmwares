/*
 * LORA.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */

#ifndef LORA_H
#define LORA_H

#include "stm32l0xx_hal.h"

// Define SPI handle
extern SPI_HandleTypeDef hspi1;

// LoRa registers
#define REG_FIFO                    0x00
#define REG_OP_MODE                 0x01
#define REG_FR_MSB                  0x06
#define REG_FR_MID                  0x07
#define REG_FR_LSB                  0x08
#define REG_PA_CONFIG               0x09
#define REG_SYNC_WORD               0x39
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_ADDR       0x0E
#define REG_FIFO_RX_BASE_ADDR       0x0F
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_DIO_MAPPING_1           0x40
#define REG_VERSION                 0x42

// LoRa constants
#define FREQ_915MHz                 0xE4C000   // 915 MHz
#define PA_BOOST                    0x80
#define MAX_POWER                   0x70
#define SYNC_WORD                   0x34
#define LORA_SLEEP                  0x00
#define LORA_STANDBY                0x01
#define LORA_TX                     0x83
#define LORA_RX_CONTINUOUS          0x85

// Pin definitions
#define LORA_DIO0_PIN               GPIO_PIN_1
#define LORA_DIO0_PORT              GPIOA
#define LORA_RST_PIN                GPIO_PIN_0
#define LORA_RST_PORT               GPIOB
#define LORA_NSS_PIN                GPIO_PIN_4
#define LORA_NSS_PORT               GPIOA

#define Maximum_Transmit_Payload 	256 //TRANSMISSION_DATA_PACKAGE = 60
#define Transmit_Payload 			64 //TRANSMISSION_DATA_PACKAGE = 12
#define EXPECTED_VERSION 0x12

// Function declarations
HAL_StatusTypeDef LoRa_Init(SPI_HandleTypeDef *hspi1);
void LoRa_SetFrequency(SPI_HandleTypeDef *hspi1, uint32_t freq);
void LoRa_SetTxPower(SPI_HandleTypeDef *hspi1, uint8_t power);
void LoRa_SetSyncWord(SPI_HandleTypeDef *hspi1, uint8_t sw);
void LoRa_Sleep(SPI_HandleTypeDef *hspi1);
void LoRa_Idle(SPI_HandleTypeDef *hspi1);
void LoRa_Transmit(SPI_HandleTypeDef *hspi1,uint8_t* data, uint8_t length);
uint8_t LoRa_Receive(SPI_HandleTypeDef *hspi1,uint8_t* buffer, uint8_t length);
uint8_t LoRa_IsDataAvailable(SPI_HandleTypeDef *hspi1);

#endif // LORA_H

