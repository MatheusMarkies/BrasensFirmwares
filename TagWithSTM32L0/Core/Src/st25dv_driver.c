/*
 * st25dv_driver.c
 *
 *  Created on: 14 de jan. de 2026
 *      Author: Matheus Markies
 */


#include "st25dv_driver.h"

// Função auxiliar para verificar se os ponteiros de IO são válidos
static bool is_io_valid(st25dv_io_t *io) {
    return (io && io->i2c_write && io->i2c_read && io->set_lpd_pin && io->delay_ms);
}

int32_t ST25DV_Init(st25dv_io_t *io) {
    if (!is_io_valid(io)) return -1;

    // Garante que o chip não está em Low Power no início
    ST25DV_LPD_Disable(io);

    // Pequeno delay para boot do chip (Tboot) [cite: 3989]
    io->delay_ms(2);

    // Leitura dummy para verificar comunicação (lendo EH_CTRL_Dyn)
    uint8_t val;
    return io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &val, 1);
}

int32_t ST25DV_ReadID(st25dv_io_t *io, uint8_t *id) {
    // Leitura do registro 0x0017 na área de configuração do sistema
    return io->i2c_read(ST25DV_ADDR_SYSCFG, ST25DV_REG_IC_REF, id, 1);
}

/* ===============================================================================
   1. Low Power Mode Implementation
   O pino LPD controla o regulador interno de 1.8V.
   High = Regulator OFF (< 1uA). Low = Active.
   =============================================================================== */
void ST25DV_LPD_Enable(st25dv_io_t *io) {
    if(io->set_lpd_pin) {
        io->set_lpd_pin(1); // Set High para entrar em LPD
    }
}

void ST25DV_LPD_Disable(st25dv_io_t *io) {
    if(io->set_lpd_pin) {
        io->set_lpd_pin(0); // Set Low para ativar o chip
        io->delay_ms(1);    // Tempo de estabilização do regulador
    }
}

/* ===============================================================================
   2. Energy Harvesting (EH) Implementation
   Usa o registro dinâmico EH_CTRL_Dyn (Volátil).
   =============================================================================== */
int32_t ST25DV_EH_Enable_Dyn(st25dv_io_t *io) {
    uint8_t reg_val;
    int32_t status = io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &reg_val, 1);
    if (status != 0) return status;

    reg_val |= ST25DV_EH_EN_MASK; // Set bit 0
    return io->i2c_write(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &reg_val, 1);
}

int32_t ST25DV_EH_Disable_Dyn(st25dv_io_t *io) {
    uint8_t reg_val;
    int32_t status = io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &reg_val, 1);
    if (status != 0) return status;

    reg_val &= ~ST25DV_EH_EN_MASK; // Clear bit 0
    return io->i2c_write(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &reg_val, 1);
}

bool ST25DV_Field_On(st25dv_io_t *io) {
    uint8_t reg_val;
    io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_EH_CTRL_DYN, &reg_val, 1);
    return (reg_val & ST25DV_FIELD_ON_MASK) ? true : false;
}

/* ===============================================================================
   3. Mailbox (Fast Transfer Mode) Implementation
   Requer ativação estática primeiro (System Config) e depois uso dinâmico.
   =============================================================================== */

// Habilita o modo Mailbox no registro estático (System Config)
// Nota: Isso requer acesso ao endereço 0x57 (E2=1) e possivelmente senha se bloqueado.
// Assumindo estado default de fábrica onde a escrita é permitida com I2C Session Open.
int32_t ST25DV_MB_Init(st25dv_io_t *io) {
    uint8_t reg_val;

    // 1. Ler MB_MODE atual (System Config Area)
    int32_t status = io->i2c_read(ST25DV_ADDR_SYSCFG, ST25DV_REG_MB_MODE, &reg_val, 1);
    if (status != 0) return status;

    // 2. Se bit 0 não estiver setado, setar.
    if ((reg_val & ST25DV_MB_MODE_RW) == 0) {
        reg_val |= ST25DV_MB_MODE_RW;
        status = io->i2c_write(ST25DV_ADDR_SYSCFG, ST25DV_REG_MB_MODE, &reg_val, 1);
        if (status != 0) return status;
        io->delay_ms(10); // Tempo de escrita na EEPROM
    }

    // 3. Habilitar Mailbox Dinamicamente (EH_CTRL_Dyn -> bit MB_EN)
    // O registro MB_CTRL_Dyn precisa ter o bit 0 setado para funcionar
    status = io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_CTRL_DYN, &reg_val, 1);
    reg_val |= ST25DV_MB_EN_MASK;
    return io->i2c_write(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_CTRL_DYN, &reg_val, 1);
}

// Verifica se há mensagem do RF esperando leitura pelo I2C
bool ST25DV_MB_HasMessageFromRF(st25dv_io_t *io) {
    uint8_t reg_val;
    io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_CTRL_DYN, &reg_val, 1);
    // Verifica bit RF_PUT_MSG [cite: 4377]
    return (reg_val & ST25DV_MB_RF_PUT_MSG) ? true : false;
}

// Lê mensagem do Mailbox
int32_t ST25DV_MB_ReadMessage(st25dv_io_t *io, uint8_t *pData, uint8_t *pLen) {
    uint8_t len_reg;

    // 1. Verificar se tem mensagem
    if (!ST25DV_MB_HasMessageFromRF(io)) {
        *pLen = 0;
        return 0; // Sem mensagem nova
    }

    // 2. Ler tamanho da mensagem (MB_LEN_Dyn)
    // O valor no registro é (Comprimento - 1). Ex: 0x00 = 1 byte. [cite: 4384]
    int32_t status = io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_LEN_DYN, &len_reg, 1);
    if (status != 0) return status;

    *pLen = len_reg + 1;

    // 3. Ler o buffer do Mailbox
    status = io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_BUFFER, pData, *pLen);

    // A leitura do último byte via I2C não limpa a flag RF_PUT_MSG automaticamente,
    // mas libera o mailbox para o RF escrever novamente se o Watchdog não tiver estourado.

    return status;
}

// Escreve mensagem para o RF ler
int32_t ST25DV_MB_WriteMessage(st25dv_io_t *io, uint8_t *pData, uint8_t len) {
    uint8_t reg_val;

    if (len == 0) return -1;

    // 1. Verificar se Mailbox está livre (HOST_PUT_MSG deve ser 0)
    io->i2c_read(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_CTRL_DYN, &reg_val, 1);
    if (reg_val & ST25DV_MB_HOST_PUT_MSG) {
        return -2; // Ocupado, mensagem anterior ainda não lida pelo RF
    }

    // 2. Escrever dados no Buffer (0x2008)
    // Importante: A escrita I2C deve começar do endereço 0x2008
    int32_t status = io->i2c_write(ST25DV_ADDR_USER_DYN, ST25DV_REG_MB_BUFFER, pData, len);
    if (status != 0) return status;

    // 3. O registro MB_LEN_Dyn e a flag HOST_PUT_MSG são atualizados automaticamente
    // pelo ST25DV após o STOP condition da escrita I2C no buffer[cite: 4447].

    return 0;
}
