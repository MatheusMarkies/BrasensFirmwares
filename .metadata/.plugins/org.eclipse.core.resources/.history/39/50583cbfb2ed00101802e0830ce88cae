/*
 * st25r300.h
 *
 *  Created on: Dec 25, 2025
 *      Author: Matheus Markies
 */

#ifndef INC_ST25R300_H_
#define INC_ST25R300_H_

#include "stm32f1xx_hal.h" // Ajuste conforme seu MCU
#include <stdint.h>
#include <stdbool.h>

/* Estrutura de configuração do ST25R300 */
typedef struct ST25R300_setting {
    GPIO_TypeDef*       CS_port;
    uint16_t            CS_pin;
    GPIO_TypeDef*       reset_port;
    uint16_t            reset_pin;
    GPIO_TypeDef*       IRQ_port;
    uint16_t            IRQ_pin;
    SPI_HandleTypeDef*  hSPIx;
} ST25R300;

/* Estrutura para dados de RSSI */
typedef struct {
    uint8_t rssi_i;      // RSSI canal I (7 bits)
    uint8_t rssi_q;      // RSSI canal Q (7 bits)
    uint8_t rssi_total;  // RSSI combinado
    float rssi_dbm;      // RSSI em dBm (aproximado)
} ST25R300_RSSI;

/* Definições de registros */
#define ST25R300_REG_OPERATION              0x00
#define ST25R300_REG_GENERAL_CONFIG         0x01
#define ST25R300_REG_RX_ANALOG_SETTINGS_1   0x09
#define ST25R300_REG_RX_ANALOG_SETTINGS_2   0x0A
#define ST25R300_REG_RX_DIGITAL_SETTINGS_1  0x0D
#define ST25R300_REG_PROTOCOL_1             0x14
#define ST25R300_REG_IRQ_STATUS_3           0x3E
#define ST25R300_REG_IC_IDENTITY            0x3F
#define ST25R300_REG_STATUS_1               0x40
#define ST25R300_REG_RSSI_DISPLAY_1         0x4A
#define ST25R300_REG_RSSI_DISPLAY_2         0x4B

/* Bits do Operation Register */
#define ST25R300_OP_EN          (1 << 3)  // Enable ready mode
#define ST25R300_OP_RX_EN       (1 << 5)  // Enable RX
#define ST25R300_OP_TX_EN       (1 << 6)  // Enable TX
#define ST25R300_OP_VDDDR_EN    (1 << 4)  // Enable VDD_DR regulator

/* Bits do Status Register 1 */
#define ST25R300_STATUS_OSC_OK  (1 << 4)  // Oscillator stable

/* Comandos diretos */
#define ST25R300_CMD_SET_DEFAULT        0x60
#define ST25R300_CMD_CLEAR_FIFO         0x64
#define ST25R300_CMD_ADJUST_REGULATORS  0x68
#define ST25R300_CMD_CALIBRATE_RC       0xEE

/* Timeouts */
#define ST25R300_TIMEOUT_MS     100
#define ST25R300_RESET_DELAY_MS 10
#define ST25R300_OSC_TIMEOUT_MS 50

/* Protótipos de funções */
bool ST25R300_Init(ST25R300 *dev);
bool ST25R300_Reset(ST25R300 *dev);
bool ST25R300_ConfigureRxMode(ST25R300 *dev);
bool ST25R300_EnableRx(ST25R300 *dev);
bool ST25R300_DisableRx(ST25R300 *dev);
bool ST25R300_ReadRSSI(ST25R300 *dev, ST25R300_RSSI *rssi);
bool ST25R300_ReadRegister(ST25R300 *dev, uint8_t reg_addr, uint8_t *data);
bool ST25R300_WriteRegister(ST25R300 *dev, uint8_t reg_addr, uint8_t data);
bool ST25R300_SendCommand(ST25R300 *dev, uint8_t cmd);
uint8_t ST25R300_ReadID(ST25R300 *dev);
bool ST25R300_WaitOscillatorReady(ST25R300 *dev);

#endif /* INC_ST25R300_H_ */
