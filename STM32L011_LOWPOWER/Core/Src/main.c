/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st25dv_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LSB_UV 63
#define USTRAIN_PER_BIT_x1000 1506 // Ajuste este valor na calibração prática com um peso conhecido

#define WAKEUP_INTERVAL_SECONDS  5
#define WAKEUP_COUNT_TARGET      4

#define ADS1115_ADDR  (0x48 << 1)
#define ADS_REG_CONV  0x00
#define ADS_REG_CONF  0x01

#define READING_SAMPLING_INTERVAL_MS 4

#define SAMPLES_FOR_TARING 1
#define SAMPLES_FOR_READING 1

int16_t offset = 0;
int16_t sum = 0;
int16_t count = 0;

int32_t strain_uE = 0;
int32_t voltage_uV = 0;

int8_t is_taring = 0;

typedef enum {
	MSI_RANGE_131KHZ = RCC_MSIRANGE_2,   // 131 kHz - Ultra low power
	MSI_RANGE_2MHZ = RCC_MSIRANGE_4,     // 2.097 MHz - I2C mode
	MSI_RANGE_4MHZ = RCC_MSIRANGE_5      // 4.194 MHz - Fast I2C mode
} MSI_Range_t;

typedef enum {
	CHARGING = 1, TARING = 2, READING = 3, PROCESSING = 4,
} Process_State;

Process_State state = CHARGING;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */
volatile uint32_t wakeup_counter = 0;
volatile uint8_t should_take_action = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void MX_GPIO_LowPower_Init(void);
void Enter_Stop_Mode(void);
void I2C_Init(void);
void I2C_DeInit(void);
int16_t ADS1115_Read(void);
void Set_MSI_Range(MSI_Range_t range);
void Setup_RTC_WakeUp(void);
void Handle_Taring(void);
void Handle_Processing(void);
void Handle_Reading(void);
void Perform_Periodic_Action(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Set_MSI_Range(MSI_Range_t range) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	// 1. Configurar novo range do MSI
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = range;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	// 2. Ajustar Flash Latency se necessário
	uint32_t flash_latency =
			(range == MSI_RANGE_131KHZ) ? FLASH_LATENCY_0 : FLASH_LATENCY_0;

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flash_latency) != HAL_OK) {
		Error_Handler();
	}
}

void Setup_RTC_WakeUp(void) {
	// Desabilitar timer existente
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	// Limpar flags
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

	// Calcular contador para N segundos
	// LSI = ~37kHz, com DIV16 = ~2.3kHz (período ~0.43ms)
	// Para X segundos: contador = X / 0.00043 ≈ X * 2304
	uint32_t wakeup_counter_value = WAKEUP_INTERVAL_SECONDS * 2304;

	// Configurar WakeUp Timer
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeup_counter_value,
	RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
		Error_Handler();
	}
}

void Enter_Stop_Mode(void) {
	// Setar clock para o mínimo antes de dormir
	Set_MSI_Range(MSI_RANGE_131KHZ);

	// Limpar flags de wakeup
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

	// Suspender SysTick
	HAL_SuspendTick();

	// Entrar em STOP Mode
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// ⚠️ ACORDOU! Código continua aqui após WakeUp
	// Reconfigurar clock MSI após acordar (fica em MSI 131kHz por padrão)
	SystemClock_Config();

	// Retomar SysTick
	HAL_ResumeTick();
}

void I2C_Init(void) {
	// Setar clock para 2MHz (mínimo para I2C funcionar bem)
	Set_MSI_Range(MSI_RANGE_4MHZ);

	// Habilitar clock I2C
	__HAL_RCC_I2C1_CLK_ENABLE();

	// Inicializar I2C
	MX_I2C1_Init();
}

void I2C_DeInit(void) {
	// Desinicializar I2C
	HAL_I2C_DeInit(&hi2c1);

	// Desabilitar clock I2C
	__HAL_RCC_I2C1_CLK_DISABLE();

	// Retornar clock para o mínimo
	Set_MSI_Range(MSI_RANGE_131KHZ);
}

int16_t ADS1115_Read(void) {
	uint8_t config[3];
	uint8_t data[2];
	int16_t raw_val;

	config[0] = ADS_REG_CONF;
	// --- CONFIGURAÇÃO DO MSB (Byte mais significativo) ---
	// Bit 15 (OS): 1 (Start Single Conversion)
	// Bits 14-12 (MUX): 100 -> AIN0 vs GND (Single-Ended) <--- CORRETO PARA SEU CIRCUITO
	// Bits 11-9 (PGA): 010 -> +/- 2.048V (Cobre seu range de 0 a 2V)
	// Bit 8 (MODE): 1 (Single-Shot)
	// Binário: 1100 0101 = 0xC5
	config[1] = 0xC5;

	// --- CONFIGURAÇÃO DO LSB (Byte menos significativo) ---
	// Bits 7-5 (DR): 100 -> 128 SPS (Padrão)
	// Bits 4-0 (COMP): Padrão (Disable comparator)
	// Binário: 1000 0011 = 0x83
	config[2] = 0x83;

	if (HAL_I2C_Master_Transmit(&hi2c1, ADS1115_ADDR, config, 3, 100) != HAL_OK)
		return -1;

	HAL_Delay(10);

	uint8_t reg_ptr = ADS_REG_CONV;
	HAL_I2C_Master_Transmit(&hi2c1, ADS1115_ADDR, &reg_ptr, 1, 100);

	if (HAL_I2C_Master_Receive(&hi2c1, ADS1115_ADDR, data, 2, 100) == HAL_OK) {
		raw_val = (int16_t) ((data[0] << 8) | data[1]);
		return raw_val;
	}
	return -1;
}

int32_t platform_i2c_write(uint8_t addr, uint16_t reg, uint8_t *data,
		uint16_t len) {
	return HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg, I2C_MEMADD_SIZE_16BIT,
			data, len, 100);
}

int32_t platform_i2c_read(uint8_t addr, uint16_t reg, uint8_t *data,
		uint16_t len) {
	return HAL_I2C_Mem_Read(&hi2c1, addr << 1, reg, I2C_MEMADD_SIZE_16BIT, data,
			len, 100);
}

void platform_delay(uint32_t ms) {
	HAL_Delay(ms);
}

st25dv_io_t st25_driver = { .i2c_write = platform_i2c_write, .i2c_read =
		platform_i2c_read, .delay_ms = platform_delay };

void Setup_NFC(void) {
	int32_t status;
	uint8_t eh_config, mb_config;

	//DEBUG_PRINT("--- Iniciando Setup NFC ---\r\n");

	// 1. Inicialização Básica
	status = ST25DV_Init(&st25_driver);
	if (status != 0) {
		//DEBUG_PRINT("ST25DV Init: FALHA\r\n");
		return;
	}
	//DEBUG_PRINT("ST25DV Init: SUCESSO\r\n");

	// 2. Apresentar Senha (necessário para acesso à EEPROM)
	status = ST25DV_I2C_PresentPassword(&st25_driver);
	if (status != 0) {
		//DEBUG_PRINT("ST25DV Senha: FALHA\r\n");
		HAL_Delay(20);
		    if (ST25DV_I2C_PresentPassword(&st25_driver) != 0) {
		        I2C_DeInit();
		        return;
		    }
	}

	HAL_Delay(10);

	// 3. Verificar Sessão I2C
	if (!ST25DV_I2C_IsSessionOpen(&st25_driver)) {
		//DEBUG_PRINT("Sessao I2C: FECHADA\r\n");
		return;
	}
	//DEBUG_PRINT("Sessao I2C: ABERTA\r\n");

	// 4. ✅ CONFIGURAÇÃO PERMANENTE: Energy Harvesting
	status = ST25DV_EH_ReadConfig_Static(&st25_driver, &eh_config);

	if (status == 0 && eh_config == 1) {
		//DEBUG_PRINT("EH: JA CONFIGURADO (Permanente)\r\n");
	} else {
		//DEBUG_PRINT("EH: Configurando pela primeira vez...\r\n");
		status = ST25DV_EH_Enable_Static(&st25_driver);
	}

	// 5. Habilitar EH no modo dinâmico (sempre necessário)
	ST25DV_EH_Enable_Dyn(&st25_driver);

	// 6. CONFIGURAÇÃO PERMANENTE: Mailbox
	status = ST25DV_MB_ReadConfig_Static(&st25_driver, &mb_config);

	if (status == 0 && mb_config == 1) {
		//DEBUG_PRINT("Mailbox: JA CONFIGURADO (Permanente)\r\n");
	} else {
		//DEBUG_PRINT("Mailbox: Configurando pela primeira vez...\r\n");
		status = ST25DV_MB_Enable_Static(&st25_driver);

		if (status == 0) {
			//DEBUG_PRINT("Mailbox: HABILITADO (PERMANENTE)\r\n");
		} else {
			//DEBUG_PRINT("Mailbox: FALHA ao configurar\r\n");
		}
	}

	// 7. Habilitar Mailbox no modo dinâmico (sempre necessário)
	ST25DV_MB_Enable_Dyn(&st25_driver);
}

void LongToStr(int32_t num, char *str) {
	int i = 0;
	uint8_t isNegative = 0;

	if (num == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	if (num < 0) {
		isNegative = 1;
		num = -num;
	}

	while (num != 0) {
		str[i++] = (num % 10) + '0';
		num /= 10;
	}

	if (isNegative) {
		str[i++] = '-';
	}

	str[i] = '\0';

	for (int j = 0; j < i / 2; j++) {
		char temp = str[j];
		str[j] = str[i - 1 - j];
		str[i - 1 - j] = temp;
	}
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	wakeup_counter++;

	if (wakeup_counter >= WAKEUP_COUNT_TARGET) {
		should_take_action = 1;
		wakeup_counter = 0;
	}
}

int16_t taring_sum = 0;
int16_t taring_count = 0;

void Handle_Taring(void) {
	int16_t raw_adc = 0;

	I2C_Init();
	HAL_Delay(50);

	while (1) {
		raw_adc = ADS1115_Read();

		taring_sum += raw_adc;
		taring_count++;

		if (taring_count >= SAMPLES_FOR_TARING) {
			offset = taring_sum / taring_count;

			is_taring = 1;
			state = CHARGING;

			HAL_Delay(50);
			I2C_DeInit();
			break;
		}

		HAL_Delay(READING_SAMPLING_INTERVAL_MS);
	}
}

//int8_t processing_init = 0;
int16_t processing_sum = 0;
int16_t processing_count = 0;
int16_t processing_average = 0;

void Handle_Processing(void) {
	int16_t raw_adc;
	int32_t status;

	I2C_Init();
	HAL_Delay(50);

	status = ST25DV_I2C_PresentPassword(&st25_driver);
	if (status != 0) {
		HAL_Delay(20);
		    if (ST25DV_I2C_PresentPassword(&st25_driver) != 0) {
		        I2C_DeInit();
		        state = CHARGING;
		        return;
		    }
	}

	HAL_Delay(10);

	if (!ST25DV_I2C_IsSessionOpen(&st25_driver)) {
		I2C_DeInit();
		return;
	}

	ST25DV_MB_Enable_Dyn(&st25_driver);
	HAL_Delay(10);

	while (1) {
		raw_adc = ADS1115_Read();

		processing_sum += raw_adc;
		processing_count++;

		if (processing_count >= SAMPLES_FOR_READING) {
			processing_average = processing_sum / processing_count;

			int32_t delta_bits = processing_average - offset;

			voltage_uV = delta_bits * LSB_UV;
			strain_uE = (delta_bits * USTRAIN_PER_BIT_x1000) / 1000;

			processing_sum = 0;
			processing_count = 0;

			char nfc_buffer[12];

			LongToStr(strain_uE, nfc_buffer);

			uint8_t msg_len = (uint8_t) strlen(nfc_buffer);

			status = ST25DV_MB_WriteMessage(&st25_driver, (uint8_t*) nfc_buffer,
					msg_len);

			if (status != 0) {
				HAL_Delay(50);
				ST25DV_MB_WriteMessage(&st25_driver, (uint8_t*) nfc_buffer,
						msg_len);
			}

			//processing_init = 0;
			state = CHARGING;

			HAL_Delay(50);
			I2C_DeInit();
			break;
		}

		HAL_Delay(READING_SAMPLING_INTERVAL_MS);
	}
}

void Handle_Reading(void){
	int16_t raw_adc = 0;

	I2C_Init();
	HAL_Delay(50);

	raw_adc = ADS1115_Read();

	sum += raw_adc;
	count++;

	if (!is_taring)
		if(count >= SAMPLES_FOR_TARING)
			state = TARING;
	else if(count >= SAMPLES_FOR_READING)
		state = PROCESSING;

	HAL_Delay(50);
	I2C_DeInit();
}

void Perform_Periodic_Action(void) {
	switch (state) {
	case CHARGING:
		if (!is_taring)
			state = TARING;
		else
			state = PROCESSING;
		break;
	case READING:
		Handle_Reading();
		break;
	case TARING:
		Handle_Taring();
		break;

	case PROCESSING:
		Handle_Processing();
		break;

	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	MX_GPIO_LowPower_Init();
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	//MX_GPIO_Init();
	MX_RTC_Init();
	//MX_I2C1_Init();
	/* USER CODE BEGIN 2 */

	__HAL_RCC_LPUART1_CLK_DISABLE();
	__HAL_RCC_SPI1_CLK_DISABLE();
	__HAL_RCC_ADC1_CLK_DISABLE();

	I2C_Init();
	HAL_Delay(100);

	// 2. Configurar NFC (apenas uma vez)
	Setup_NFC();
	HAL_Delay(100);

	// 3. ✅ IMPORTANTE: Desinicializar I2C após setup
	I2C_DeInit();
	HAL_Delay(50);

	// 4. Configurar RTC WakeUp Timer
	Setup_RTC_WakeUp();

	// 5. Pequeno delay inicial
	HAL_Delay(100);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		if (should_take_action) {
			should_take_action = 0;
			Perform_Periodic_Action();
		}

		// Retornar para STOP Mode
		Enter_Stop_Mode();

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x00000608;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */

	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}

	/** Enable the WakeUp
	 */

	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void MX_GPIO_LowPower_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	// ✅ Habilitar TODOS os clocks
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	GPIO_InitStruct.Pin = GPIO_PIN_All
			& ~(GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_10 | GPIO_PIN_9);
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// ✅ TODOS os pinos da Porta B
	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// ✅ TODOS os pinos da Porta C
	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	// Se tiver GPIOD e GPIOH, fazer o mesmo
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
