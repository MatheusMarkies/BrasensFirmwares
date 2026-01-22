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
#include "string.h"
#include <stdlib.h>
#include "st25dv_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LSB_UV 63
// Ajuste este valor na calibração prática com um peso conhecido
#define USTRAIN_PER_BIT_x1000 1506

#define ADS1115_ADDR  (0x48 << 1)

#define ADS_REG_CONV  0x00
#define ADS_REG_CONF  0x01

#define CHARGING_INTERVAL_TICKS 1
static uint16_t wakeup_counter = 0;
static uint16_t wakeup_cycles = 10; //CHARGING_INTERVAL_MS / (8 * 1000);

#define READING_SAMPLING_INTERVAL_MS 4

#define SAMPLES_FOR_TARING 10
#define SAMPLES_FOR_READING 10

int32_t strain_uE = 0;
int32_t voltage_uV = 0;

uint32_t last_charging_process = 0;

typedef enum {
	CHARGING = 1, TARING = 2, READING = 3, PROCESSING = 4,
} Process_State;

uint8_t isTared = 0;

Process_State state = CHARGING;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

#ifdef DEBUG
	#define DEBUG_PRINT(msg) HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), 200)
#else
	#define DEBUG_PRINT(msg) do {} while (0)
#endif
void I2C_Scanner(I2C_HandleTypeDef *hi2c);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Função auxiliar para converter byte em Hex (ex: 0x2A) sem usar sprintf */
/* Requer um buffer de pelo menos 3 bytes (2 digitos + \0) */
void Uint8ToHexStr(uint8_t num, char *str) {
	const char hexDigits[] = "0123456789ABCDEF";
	str[0] = hexDigits[(num >> 4) & 0x0F]; // Nibble alto
	str[1] = hexDigits[num & 0x0F];        // Nibble baixo
	str[2] = '\0';                         // Terminador nulo
}

/* Função auxiliar para converter Inteiro em String decimal simples */
void IntToDecStr(uint8_t num, char *str) {
	uint8_t i = 0;
	if (num >= 100)
		str[i++] = (num / 100) + '0';
	if (num >= 10)
		str[i++] = ((num / 10) % 10) + '0';
	str[i++] = (num % 10) + '0';
	str[i] = '\0';
}

void I2C_Scanner(I2C_HandleTypeDef *hi2c) {
	// Reduzido de 64 bytes para apenas 4 bytes!
	char smallBuf[4];
	uint8_t devices_found = 0;

	DEBUG_PRINT("\r\n--- Scanning I2C bus ---\r\n");

	for (uint8_t addr = 1; addr < 128; addr++) {
		/* O HAL usa o endereço deslocado (shifted) à esquerda */
		if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t) (addr << 1), 2, 10)
				== HAL_OK) {

			// Imprime em partes para não precisar de um buffer gigante
			DEBUG_PRINT("Device found at: 0x");

			// Converte endereço 7-bit
			Uint8ToHexStr(addr, smallBuf);
			DEBUG_PRINT(smallBuf);

			DEBUG_PRINT(" (8-bit: 0x");

			// Converte endereço 8-bit (write address)
			Uint8ToHexStr(addr << 1, smallBuf);
			DEBUG_PRINT(smallBuf);

			DEBUG_PRINT(")\r\n");

			devices_found++;
		}
	}

	if (devices_found == 0) {
		DEBUG_PRINT("No I2C devices found!\r\n");
	} else {
		DEBUG_PRINT("\r\nTotal devices found: ");
		IntToDecStr(devices_found, smallBuf);
		DEBUG_PRINT(smallBuf);
		DEBUG_PRINT("\r\n");
	}

	DEBUG_PRINT("--- Scan Complete ---\r\n");
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

void debug_print_int(int32_t v) {
#ifdef DEBUG  // <--- INÍCIO DO BLOCO CONDICIONAL
	char buf[12];
	int i = 9;

	if (v < 0) {
		// Se quiser imprimir o sinal, descomente a linha abaixo
		HAL_UART_Transmit(&hlpuart1, (uint8_t*) "-", 1, 100);
		v = -v;
	}

	do {
		buf[i--] = '0' + (v % 10);
		v /= 10;
	} while (v);

	buf[10] = '\r';
	buf[11] = '\n';

	// Envia o buffer processado
	HAL_UART_Transmit(&hlpuart1, (uint8_t*) &buf[i + 1], 11 - i, 100);
#endif // <--- FIM DO BLOCO CONDICIONAL
}

void rtc_start_wakeup(uint16_t ticks) {
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, ticks, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
}

void enter_stop_mode(void) {
	// 1. Desliga periféricos que consomem energia
	HAL_I2C_DeInit(&hi2c1);

	// Se estiver usando UART debug, desliga também (ou vai vazar corrente pelos pinos TX/RX)
#ifdef DEBUG
	HAL_UART_DeInit(&hlpuart1);
#endif

	// 2. Limpa flags
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	// 3. Entra em STOP
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// --- O MCU DORME AQUI ---

	// 4. Acorda e restaura Clocks
	SystemClock_Config();
	HAL_ResumeTick();

	// 5. Reinicializa periféricos
	MX_I2C1_Init();
#ifdef DEBUG
	MX_LPUART1_UART_Init();
#endif
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	wakeup_counter++;

	if (wakeup_counter >= wakeup_cycles) {
		wakeup_counter = 0;
		if (!isTared) {
			state = TARING;
		} else {
			state = READING;
		}
	} else {
		state = CHARGING;
	}
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

void platform_set_lpd(uint8_t state) {
	HAL_GPIO_WritePin(LPD_GPIO_Port, LPD_Pin,
			state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void platform_delay(uint32_t ms) {
	HAL_Delay(ms);
}

st25dv_io_t st25_driver = { .i2c_write = platform_i2c_write, .i2c_read =
		platform_i2c_read, .set_lpd_pin = platform_set_lpd, .delay_ms =
		platform_delay };

void Setup_NFC(void) {
	int32_t status;
	uint8_t eh_config, mb_config;

	DEBUG_PRINT("--- Iniciando Setup NFC ---\r\n");

	// 1. Inicialização Básica
	status = ST25DV_Init(&st25_driver);
	if (status != 0) {
		DEBUG_PRINT("ST25DV Init: FALHA\r\n");
		return;
	}
	DEBUG_PRINT("ST25DV Init: SUCESSO\r\n");

	// 2. Apresentar Senha (necessário para acesso à EEPROM)
	status = ST25DV_I2C_PresentPassword(&st25_driver);
	if (status != 0) {
		DEBUG_PRINT("ST25DV Senha: FALHA\r\n");
		return;
	}

	HAL_Delay(10);

	// 3. Verificar Sessão I2C
	if (!ST25DV_I2C_IsSessionOpen(&st25_driver)) {
		DEBUG_PRINT("Sessao I2C: FECHADA\r\n");
		return;
	}
	DEBUG_PRINT("Sessao I2C: ABERTA\r\n");

	// 4. ✅ CONFIGURAÇÃO PERMANENTE: Energy Harvesting
	status = ST25DV_EH_ReadConfig_Static(&st25_driver, &eh_config);

	if (status == 0 && eh_config == 1) {
		DEBUG_PRINT("EH: JA CONFIGURADO (Permanente)\r\n");
	} else {
		DEBUG_PRINT("EH: Configurando pela primeira vez...\r\n");
		status = ST25DV_EH_Enable_Static(&st25_driver);
	}

	// 5. Habilitar EH no modo dinâmico (sempre necessário)
	ST25DV_EH_Enable_Dyn(&st25_driver);

	// 6. ✅ CONFIGURAÇÃO PERMANENTE: Mailbox
	status = ST25DV_MB_ReadConfig_Static(&st25_driver, &mb_config);

	if (status == 0 && mb_config == 1) {
		DEBUG_PRINT("Mailbox: JA CONFIGURADO (Permanente)\r\n");
	} else {
		DEBUG_PRINT("Mailbox: Configurando pela primeira vez...\r\n");
		status = ST25DV_MB_Enable_Static(&st25_driver);

		if (status == 0) {
			DEBUG_PRINT("Mailbox: HABILITADO (PERMANENTE)\r\n");
		} else {
			DEBUG_PRINT("Mailbox: FALHA ao configurar\r\n");
		}
	}

	// 7. Habilitar Mailbox no modo dinâmico (sempre necessário)
	ST25DV_MB_Enable_Dyn(&st25_driver);
	DEBUG_PRINT("Mailbox: Modo dinamico ativo\r\n");

	DEBUG_PRINT("--- Setup NFC Finalizado ---\r\n");
}

void Loop_NFC(void) {
	if (ST25DV_Field_On(&st25_driver)) {
		uint8_t msg[] = "Ola RF";
		ST25DV_MB_WriteMessage(&st25_driver, msg, sizeof(msg));
	}
}

/* Converte int32_t para string (ASCII) de forma leve */
void LongToStr(int32_t num, char *str) {
	int i = 0;
	uint8_t isNegative = 0;

	// Caso especial para 0
	if (num == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	// Lida com números negativos
	if (num < 0) {
		isNegative = 1;
		num = -num; // Torna positivo para processamento
	}

	// Extrai dígitos (gera a string invertida)
	while (num != 0) {
		str[i++] = (num % 10) + '0';
		num /= 10;
	}

	// Adiciona o sinal se necessário
	if (isNegative) {
		str[i++] = '-';
	}

	str[i] = '\0'; // Finaliza string

	// Inverte a string para a ordem correta
	for (int j = 0; j < i / 2; j++) {
		char temp = str[j];
		str[j] = str[i - 1 - j];
		str[i - 1 - j] = temp;
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

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
#ifdef DEBUG
	MX_LPUART1_UART_Init();
#endif
	MX_I2C1_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */
	DEBUG_PRINT("--- Iniciando! ---\r\n");
	I2C_Scanner(&hi2c1);
	Setup_NFC();

	int32_t offset = 0;
	int32_t average = 0;
	int32_t sum = 0;
	int count = 0;
	int16_t raw_adc = 0;

	DEBUG_PRINT("Starting...\r\n");
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		switch (state) {
		case CHARGING:
			rtc_start_wakeup(CHARGING_INTERVAL_TICKS);
			enter_stop_mode();
			break;

		case TARING:
			raw_adc = ADS1115_Read();

			if (abs(raw_adc) > 0) {
				if (count <= SAMPLES_FOR_TARING) {
					sum += raw_adc;
					count++;
					offset = sum / count;
				} else {
					count = 0;
					sum = 0;

					DEBUG_PRINT("offset: \r\n");
					debug_print_int(offset);
					DEBUG_PRINT("\r\n");

					isTared = 1;

					state = CHARGING;
				}
			} else {

			}

			HAL_Delay(READING_SAMPLING_INTERVAL_MS);
			break;
		case READING:
			state = PROCESSING;
			break;

		case PROCESSING:
			raw_adc = ADS1115_Read();

			sum += raw_adc;
			count++;

			if (count <= SAMPLES_FOR_READING) {
				average = sum / count;

				int32_t delta_bits = average - offset;

				// if (abs(delta_bits) > 0) { // Removido para atualizar mesmo se for 0
				voltage_uV = delta_bits * LSB_UV;
				strain_uE = (delta_bits * USTRAIN_PER_BIT_x1000) / 1000;

				sum = 0;
				count = 0;

				DEBUG_PRINT("voltage_uV: \r\n");
				debug_print_int(voltage_uV);
				DEBUG_PRINT("\r\n");
				// --- Debug via UART ---
				DEBUG_PRINT("strain_uE: \r\n");
				debug_print_int(strain_uE);
				DEBUG_PRINT("\r\n");

				// --- ATUALIZAÇÃO NFC MAILBOX ---
				// Buffer para string (sinal + 10 digitos + null)W
				//char nfc_buffer[12];
				char nfc_buffer[12];

				// 2. Converter o valor REAL (strain_uE) para Texto Puro
				LongToStr(strain_uE, nfc_buffer);

				// 3. Calcular o tamanho da mensagem gerada
				uint8_t msg_len = (uint8_t) strlen(nfc_buffer);

				// 4. Enviar para o Mailbox
				// Cast para (uint8_t*) é necessário pois a função espera bytes crus
				if (ST25DV_MB_WriteMessage(&st25_driver, (uint8_t*) nfc_buffer,
						msg_len) == 0) {
					DEBUG_PRINT("NFC Updated\r\n");
				} else {
					DEBUG_PRINT("NFC Write Failed (Busy?)\r\n");
				}

				state = CHARGING;
			}

			HAL_Delay(READING_SAMPLING_INTERVAL_MS);
			break;

		}
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
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1
			| RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
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
 * @brief LPUART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_LPUART1_UART_Init(void) {

	/* USER CODE BEGIN LPUART1_Init 0 */

	/* USER CODE END LPUART1_Init 0 */

	/* USER CODE BEGIN LPUART1_Init 1 */

	/* USER CODE END LPUART1_Init 1 */
	hlpuart1.Instance = LPUART1;
	hlpuart1.Init.BaudRate = 9600;
	hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
	hlpuart1.Init.StopBits = UART_STOPBITS_1;
	hlpuart1.Init.Parity = UART_PARITY_NONE;
	hlpuart1.Init.Mode = UART_MODE_TX_RX;
	hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN LPUART1_Init 2 */

	/* USER CODE END LPUART1_Init 2 */

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
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LPD_GPIO_Port, LPD_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : NFC_INTERRUPT_Pin */
	GPIO_InitStruct.Pin = NFC_INTERRUPT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(NFC_INTERRUPT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LPD_Pin */
	GPIO_InitStruct.Pin = LPD_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LPD_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
