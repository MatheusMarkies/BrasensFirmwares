#include "LoRa.h"

void LoRa_WriteReg(SPI_HandleTypeDef *hspi1, uint8_t addr, uint8_t value) {
	uint8_t data[2] = { addr | 0x80, value };
	HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET);
}

uint8_t LoRa_ReadReg(SPI_HandleTypeDef *hspi1, uint8_t addr) {
    uint8_t value;
    uint8_t data = addr & 0x7F;
	HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, (uint8_t)&data, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, (uint8_t)&value, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET);
	return value;
}

void LoRa_Reset(void) {
	HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_SET);
	HAL_Delay(10);
}

HAL_StatusTypeDef LoRa_Init(SPI_HandleTypeDef *hspi1) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = LORA_NSS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LORA_NSS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LORA_RST_PIN;
    HAL_GPIO_Init(LORA_RST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LORA_DIO0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LORA_DIO0_PORT, &GPIO_InitStruct);

    LoRa_Reset();

    // Verificação do módulo LoRa lendo o registro de versão
    uint8_t version = LoRa_ReadReg(hspi1, REG_VERSION);
    if (version != EXPECTED_VERSION) {
        return HAL_ERROR;
    }

    LoRa_Sleep(hspi1);
    LoRa_WriteReg(hspi1, REG_OP_MODE, 0x80);
    LoRa_SetFrequency(hspi1, FREQ_915MHz);
    LoRa_SetTxPower(hspi1, 17);
    LoRa_SetSyncWord(hspi1, SYNC_WORD);
    LoRa_WriteReg(hspi1, REG_FIFO_TX_BASE_ADDR, 0x00);
    LoRa_WriteReg(hspi1, REG_FIFO_RX_BASE_ADDR, 0x00);
    LoRa_Idle(hspi1);

    return HAL_OK;
}

void LoRa_SetFrequency(SPI_HandleTypeDef *hspi1, uint32_t freq) {
	uint32_t frf = (freq << 19) / 32000000;
	LoRa_WriteReg(hspi1, REG_FR_MSB, (uint8_t) (frf >> 16));
	LoRa_WriteReg(hspi1, REG_FR_MID, (uint8_t) (frf >> 8));
	LoRa_WriteReg(hspi1, REG_FR_LSB, (uint8_t) (frf));
}

void LoRa_SetTxPower(SPI_HandleTypeDef *hspi1, uint8_t power) {
	if (power > 20)
		power = 20;
	LoRa_WriteReg(hspi1, REG_PA_CONFIG, PA_BOOST | (power - 2));
}

void LoRa_SetSyncWord(SPI_HandleTypeDef *hspi1, uint8_t sw) {
	LoRa_WriteReg(hspi1, REG_SYNC_WORD, sw);
}

void LoRa_Sleep(SPI_HandleTypeDef *hspi1) {
	LoRa_WriteReg(hspi1, REG_OP_MODE, LORA_SLEEP);
}

void LoRa_Idle(SPI_HandleTypeDef *hspi1) {
	LoRa_WriteReg(hspi1, REG_OP_MODE, LORA_STANDBY);
}

void LoRa_Transmit(SPI_HandleTypeDef *hspi1, uint8_t *data, uint8_t length) {
	LoRa_WriteReg(hspi1, REG_OP_MODE, LORA_STANDBY);
	LoRa_WriteReg(hspi1, REG_FIFO_ADDR_PTR, 0x00);
	for (uint8_t i = 0; i < length; i++) {
		LoRa_WriteReg(hspi1, REG_FIFO, data[i]);
	}
	LoRa_WriteReg(hspi1, REG_OP_MODE, LORA_TX);
	while ((LoRa_ReadReg(hspi1, REG_IRQ_FLAGS) & 0x08) == 0);
	LoRa_WriteReg(hspi1, REG_IRQ_FLAGS, 0x08);
}

uint8_t LoRa_Receive(SPI_HandleTypeDef *hspi1, uint8_t *buffer, uint8_t length) {
	LoRa_WriteReg(hspi1, REG_OP_MODE, LORA_RX_CONTINUOUS);
	while ((LoRa_ReadReg(hspi1, REG_IRQ_FLAGS) & 0x40) == 0);
	uint8_t rxLength = LoRa_ReadReg(hspi1, REG_RX_NB_BYTES);
	LoRa_WriteReg(hspi1, REG_FIFO_ADDR_PTR, LoRa_ReadReg(hspi1, REG_FIFO_RX_CURRENT_ADDR));
	for (uint8_t i = 0; i < rxLength; i++) {
		buffer[i] = LoRa_ReadReg(hspi1, REG_FIFO);
	}
	LoRa_WriteReg(hspi1, REG_IRQ_FLAGS, 0x40);
	return rxLength;
}

uint8_t LoRa_IsDataAvailable(SPI_HandleTypeDef *hspi1) {
	return (LoRa_ReadReg(hspi1, REG_IRQ_FLAGS) & 0x40) != 0;
}
