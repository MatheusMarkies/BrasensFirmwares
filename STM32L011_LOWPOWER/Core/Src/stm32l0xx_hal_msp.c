/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32l0xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/**
  * @brief I2C MSP Initialization
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PA9 -> I2C1_SCL (Alternate Function 1)
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 -> I2C1_SDA (Alternate Function 0)
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Alternate = 0x00; // AF0 resolve o problema do STM32L011
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LIGA O CLOCK DO I2C1 (Evita o HardFault 0x0) */
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
  }
}

/**
  * @brief UART MSP Initialization
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==LPUART1)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /** LPUART1 GPIO Configuration
        PA0-CK_IN  ------> LPUART1_RX
        PA4        ------> LPUART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_LPUART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LIGA O CLOCK DA LPUART1 (Evita o HardFault 0x0) */
    __HAL_RCC_LPUART1_CLK_ENABLE();
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==LPUART1)
  {
    __HAL_RCC_LPUART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_4);
  }
}

/**
  * @brief RTC MSP Initialization
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
  if(hrtc->Instance==RTC)
  {
    __HAL_RCC_RTC_ENABLE();
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
  if(hrtc->Instance==RTC)
  {
    __HAL_RCC_RTC_DISABLE();
  }
}
