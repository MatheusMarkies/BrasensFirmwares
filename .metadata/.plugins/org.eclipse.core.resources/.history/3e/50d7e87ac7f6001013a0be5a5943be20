/*
 * st25dv_driver.h
 *
 *  Created on: 14 de jan. de 2026
 *      Author: Matheus Markies
 */

#ifndef ST25DV_DRIVER_H
#define ST25DV_DRIVER_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Endereços I2C (7-bit) baseados em E1=0, E0=0. Ajuste conforme seu hardware */
#define ST25DV_ADDR_USER_DYN   0x53 // E2=0
#define ST25DV_ADDR_SYSCFG     0x57 // E2=1

#define ST25DV_REG_IC_REF      0x0017

/* --- Mapeamento de Registros (System Configuration E2=1) --- */
#define ST25DV_REG_EH_MODE     0x0002 // [cite: 4259]
#define ST25DV_REG_MB_MODE     0x000D // [cite: 4259]
#define ST25DV_REG_MB_WDG      0x000E // [cite: 4259]

/* --- Mapeamento de Registros (Dynamic E2=0) --- */
#define ST25DV_REG_EH_CTRL_DYN 0x2002 // [cite: 4293]
#define ST25DV_REG_MB_CTRL_DYN 0x2006 // [cite: 4293]
#define ST25DV_REG_MB_LEN_DYN  0x2007 // [cite: 4293]
#define ST25DV_REG_MB_BUFFER   0x2008 // Início do buffer de 256 bytes [cite: 4311]

/* --- Máscaras de Bits --- */
// EH_CTRL_Dyn
#define ST25DV_EH_EN_MASK      0x01
#define ST25DV_EH_ON_MASK      0x02
#define ST25DV_FIELD_ON_MASK   0x04
#define ST25DV_VCC_ON_MASK     0x08

// MB_MODE [cite: 4361]
#define ST25DV_MB_MODE_RW      0x01

// MB_CTRL_Dyn [cite: 4377]
#define ST25DV_MB_EN_MASK      0x01
#define ST25DV_MB_HOST_PUT_MSG 0x02 // I2C colocou mensagem
#define ST25DV_MB_RF_PUT_MSG   0x04 // RF colocou mensagem

/* --- Estrutura do Driver (Abstração de Hardware) --- */
typedef struct {
    // Ponteiros para funções de plataforma (I2C Read/Write, GPIO Write, Delay)
    // addr: Endereço I2C do dispositivo (7-bit)
    // reg: Endereço do registro (16-bit)
    int32_t (*i2c_write)(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len);
    int32_t (*i2c_read)(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len);
    void    (*set_lpd_pin)(uint8_t state); // 1 = High, 0 = Low
    void    (*delay_ms)(uint32_t ms);
} st25dv_io_t;

/* --- Protótipos das Funções --- */

// Inicialização
int32_t ST25DV_Init(st25dv_io_t *io);
int32_t ST25DV_ReadID(st25dv_io_t *io, uint8_t *id);

// 1. Low Power Mode
void ST25DV_LPD_Enable(st25dv_io_t *io);  // Entra em baixo consumo
void ST25DV_LPD_Disable(st25dv_io_t *io); // Sai do baixo consumo (boot)

// 2. Energy Harvesting (Dinâmico)
int32_t ST25DV_EH_Enable_Dyn(st25dv_io_t *io);
int32_t ST25DV_EH_Disable_Dyn(st25dv_io_t *io);
bool    ST25DV_EH_IsActive(st25dv_io_t *io);
bool    ST25DV_Field_On(st25dv_io_t *io);

// 3. Mailbox (Fast Transfer Mode)
int32_t ST25DV_MB_Init(st25dv_io_t *io); // Habilita MB_MODE (Static)
int32_t ST25DV_MB_WriteMessage(st25dv_io_t *io, uint8_t *pData, uint8_t len);
int32_t ST25DV_MB_ReadMessage(st25dv_io_t *io, uint8_t *pData, uint8_t *pLen);
bool    ST25DV_MB_HasMessageFromRF(st25dv_io_t *io);

int32_t ST25DV_I2C_PresentPassword(st25dv_io_t *io);
bool ST25DV_I2C_IsSessionOpen(st25dv_io_t *io); // <--- Adicionar

#endif // ST25DV_DRIVER_H
