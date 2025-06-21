/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "stm32l0xx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32l0xx_hal.h"

#include "math.h"
#include "string.h"
#include "stdio.h"
#include "LORA.h"
#include "FRAM.h"
#include "KX122.h"
#include "MCP9808.h"
#include "MAX17048.h"
#include "BrasensFirmware.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_KEY "1111A"
TIM_HandleTypeDef htim2;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define DEBUG_PRINT(msg) HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY)
#define min(a, b) ((a) < (b) ? (a) : (b))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
Transmission_Data data;
Transmission_VibrationPackage vibrationPackage;

unsigned int sampling_period_us;
unsigned long data_sender_period;

int package_factor = 0;
int acc_sample_factor = 0;

FRAM_Metadata metadata;

uint32_t sizeInBytes = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void writeVibrationInformation(void);
void setup(void);
void readAndSendFRAMData(void);
void loop(void);
void sendVibrationPackage(Transmission_VibrationPackage sendingVibrationPackage);
void sendData(Transmission_Data sendingData);
uint32_t millis(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	TIM2_Init();
	setup();

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		loop();
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
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
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
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1
			| RCC_PERIPHCLK_I2C1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

void TIM2_Init(void) {
	__HAL_RCC_TIM2_CLK_ENABLE();

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = (uint32_t) (HAL_RCC_GetPCLK1Freq() / 1000000) - 1; // 1 MHz
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 0xFFFFFFFF; // Máximo período
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
		Error_Handler();
	}
}

uint32_t micros() {
	return __HAL_TIM_GET_COUNTER(&htim2);
}

void delayMicroseconds(uint32_t us) {
	uint32_t start = micros();
	while ((micros() - start) < us) { // Safer to compute a delta, and compare that..
		// Espera até atingir o tempo desejado
	}
}

uint16_t address = 0;

unsigned long writeMicroseconds;
int current_sample = 0;

void writeVibrationInformation() {
	data.rms_accel[0] = 0.0;
	data.rms_accel[1] = 0.0;
	data.rms_accel[2] = 0.0;

	data.rms_vel[0] = 0.0;
	data.rms_vel[1] = 0.0;
	data.rms_vel[2] = 0.0;

	float velocity_x = 0.0;
	float velocity_y = 0.0;
	float velocity_z = 0.0;

	int d = 0;
	int count = 0;
	DEBUG_PRINT("read...\r\n");
	//FOR

	for (int i = 0; i < SAMPLES; i++) {
		Vibration vibration = KX122_ReadAccelData(&hi2c1);

		count++;

		FRAM_WriteFloat(&hi2c1, &metadata,
				address + SAMPLES * 0 * sizeof(float), vibration.x);
		FRAM_WriteFloat(&hi2c1, &metadata,
				address + SAMPLES * 1 * sizeof(float), vibration.y);
		FRAM_WriteFloat(&hi2c1, &metadata,
				address + SAMPLES * 2 * sizeof(float), vibration.z);

		address += sizeof(float);

		d++;
		count = 0;

		data.rms_accel[0] += (vibration.x * vibration.x);
		data.rms_accel[1] += (vibration.y * vibration.y);
		data.rms_accel[2] += (vibration.z * vibration.z);

		float deltaTime = sampling_period_us / 1e6;
		velocity_x += vibration.x * deltaTime; //Ax * dT
		velocity_y += vibration.y * deltaTime; //Ay * dT
		velocity_z += vibration.z * deltaTime; //Az * dT

		data.rms_vel[0] += (velocity_x * velocity_x);
		data.rms_vel[1] += (velocity_y * velocity_y);
		data.rms_vel[2] += (velocity_z * velocity_z);

		delayMicroseconds(sampling_period_us);
	}

	data.rms_accel[0] = sqrt(data.rms_accel[0] / SAMPLES);
	data.rms_accel[1] = sqrt(data.rms_accel[1] / SAMPLES);
	data.rms_accel[2] = sqrt(data.rms_accel[2] / SAMPLES);

	data.rms_vel[0] = sqrt(data.rms_vel[0] / SAMPLES);
	data.rms_vel[1] = sqrt(data.rms_vel[1] / SAMPLES);
	data.rms_vel[2] = sqrt(data.rms_vel[2] / SAMPLES);

	char buffer[256];
	snprintf(buffer, sizeof(buffer),
			"RMS Accel: %.2f, %.2f, %.2f\r\nRMS Vel: %.2f, %.2f, %.2f\r\n",
			data.rms_accel[0], data.rms_accel[1], data.rms_accel[2],
			data.rms_vel[0], data.rms_vel[1], data.rms_vel[2]);
	DEBUG_PRINT(buffer);
}

HAL_StatusTypeDef MCP9808_ReadTemperature_LowPower(double *temperature) {
	HAL_StatusTypeDef status;

	status = MCP9808_Wake(&hi2c1);
	if (status != HAL_OK) {
		return status;
	}

	HAL_Delay(50);

	status = MCP9808_ReadTemperature(&hi2c1, temperature);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Temp: %d \r\n", *temperature);
	DEBUG_PRINT(buffer);

	if (status != HAL_OK) {
		return status;
	}

	status = MCP9808_Shutdown(&hi2c1);
	return status;
}

void readBatteryData(uint16_t *soc) {
	uint16_t voltage;
	char msg[64];

	if (MAX17048_ReadVoltage(&hi2c1, &voltage) == HAL_OK) {
		snprintf(msg, sizeof(msg), "Voltage: %u mV\r\n", voltage);
		DEBUG_PRINT(msg);
	} else {
		DEBUG_PRINT("Failed to read voltage\r\n");
	}

	if (MAX17048_ReadSOC(&hi2c1, soc) == HAL_OK) {
		snprintf(msg, sizeof(msg), "State of Charge: %u%%\r\n", *soc);
		DEBUG_PRINT(msg);
	} else {
		DEBUG_PRINT("Failed to read SOC\r\n");
	}
}

void I2C_Scan() {
	char msg[64];
	HAL_StatusTypeDef result;
	uint8_t i;

	DEBUG_PRINT("Scanning I2C bus:\r\n");

	for (i = 1; i < 128; i++) {

		result = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t) (i << 1), 1, 10);
		if (result == HAL_OK) {
			snprintf(msg, sizeof(msg), "Device found at 0x%02X\r\n", i);
			DEBUG_PRINT(msg);
		} else {

		}
	}
	DEBUG_PRINT("I2C scan completed.\r\n");
}

void setup() {
	DEBUG_PRINT("Starting...\r\n");
	I2C_Scan();

	if (KX122_Init(&hi2c1) != HAL_ERROR)
		DEBUG_PRINT("Ready KX122\r\n");
	else
		DEBUG_PRINT("Error in KX122 connection\r\n");

	if (MCP9808_Init(&hi2c1) != HAL_ERROR)
		DEBUG_PRINT("Ready MCP9808\r\n");
	else
		DEBUG_PRINT("Error in MCP9808 connection\r\n");

	if (MAX17048_Init(&hi2c1) != HAL_ERROR)
		DEBUG_PRINT("Ready MAX17048\r\n");
	else
		DEBUG_PRINT("Error in MAX17048 connection\r\n");

	if (FRAM_Init(&hi2c1) != HAL_ERROR)
		DEBUG_PRINT("Ready FRAM\r\n");
	else
		DEBUG_PRINT("Error in FRAM connection\r\n");

	DEBUG_PRINT("Ready!\r\n");

	FRAM_InitMetadata(&metadata);

	char buffer[50];
	snprintf(buffer, sizeof(buffer), "Next Memory Adds: %d\r\n",
			metadata.nextFreeAddress);
	DEBUG_PRINT(buffer);

	package_factor = 3
			* ceil((float) (SAMPLES) / (float) TRANSMISSION_DATA_PACKAGE);
	data_sender_period = round(
			1000 * ((float) DATA_TRANSMISSION_PERIOD / (float) package_factor));
	sampling_period_us = round(1000000 * (SAMPLES / (float) ACC_DATA_RATE));
	acc_sample_factor = floor((float) ACC_DATA_RATE / (float) SAMPLES);

	sizeInBytes = FRAM_MEMORY_SIZE;

	FRAM_Format(&hi2c1, &metadata);

	DEBUG_PRINT("Starting Temperature Reading:\r\n");
	double temp;
	MCP9808_ReadTemperature_LowPower(&temp);

	char bufferTemp[50];
	snprintf(bufferTemp, sizeof(bufferTemp), "Temp: %.2f\r\n", temp); // Usar %.2f para imprimir double
	DEBUG_PRINT(bufferTemp);

	DEBUG_PRINT("Starting Battery Reading:\r\n");
	int16_t bat = 0;
	readBatteryData(&bat);

	char bufferBat[50];
	snprintf(bufferBat, sizeof(bufferBat), "Bat: %d\r\n", bat);
	DEBUG_PRINT(bufferBat);

	Vibration vibration = KX122_ReadAccelData(&hi2c1);

	char bufferVib[50];
	snprintf(bufferVib, sizeof(bufferVib), "RMS Accel: %.2f, %.2f, %.2f\r\n",
			vibration.x, vibration.y, vibration.z); // Usar %.2f para imprimir double
	DEBUG_PRINT(bufferVib);

	// Teste de escrita e leitura da FRAM
	float test_value = 123.456f;
	float read_value = 0.0f;
	uint16_t test_address = 0x0000;

	if (FRAM_WriteFloat(&hi2c1, &metadata, test_address, test_value)
			== HAL_OK) {
		DEBUG_PRINT("FRAM write success\r\n");

		if (FRAM_ReadFloat(&hi2c1, test_address, &read_value) == HAL_OK) {
			char buffer[100];
			snprintf(buffer, sizeof(buffer),
					"FRAM read success, value: %.3f\r\n", read_value);
			DEBUG_PRINT(buffer);

			if (fabs(test_value - read_value) < 0.001f) {
				DEBUG_PRINT("FRAM test passed\r\n");
			} else {
				DEBUG_PRINT("FRAM test failed: values do not match\r\n");
			}
		} else {
			DEBUG_PRINT("FRAM read failed\r\n");
		}
	} else {
		DEBUG_PRINT("FRAM write failed\r\n");
	}

	DEBUG_PRINT("Testando delayMicroseconds...\r\n");
	uint32_t start_test = micros();
	delayMicroseconds(1000000);  // 1 segundo
	uint32_t end_test = micros();
	char bufferDelay[50];
	snprintf(bufferDelay, sizeof(bufferDelay), "Atraso: %lu us\r\n",
			end_test - start_test);
	DEBUG_PRINT(bufferDelay);

	DEBUG_PRINT("Starting Vibration Reading:\r\n");
	writeVibrationInformation();
}

unsigned long readMilliseconds;

bool energy_save = false;

float temperatureValue;
int current_package = 0;
int reset_counter = 0;
bool sendDataDelay = false;
int axis = 0;
void readAndSendFRAMData() {
	DEBUG_PRINT("readAndSendFRAMData...\r\n");
	if (energy_save) {
		readMilliseconds = millis();
		sendDataDelay = true;
		energy_save = false;
	} else if (millis() > (readMilliseconds + data_sender_period)) {
		sendDataDelay = true;
	}

	if (current_package >= (2 * package_factor / 3))
		axis = 2;
	else if (current_package >= (package_factor / 3))
		axis = 1;
	else
		axis = 0;

	if (sendDataDelay) {
		int start = TRANSMISSION_DATA_PACKAGE * current_package
				- TRANSMISSION_DATA_PACKAGE * axis * package_factor / 3;

		int end = (start + TRANSMISSION_DATA_PACKAGE);

		if (current_package > 0)
			start += 1;

		for (int i = start; i <= end; i++) {
			uint16_t address = i * sizeof(float)
					+ SAMPLES * axis * sizeof(float);
			float value = 0.0;

			if (SAMPLES >= i) {
				FRAM_ReadFloat(&hi2c1, address, &value);
			}

			vibrationPackage.dataPackage[i - start] = value;
		}

		strncpy(vibrationPackage.key, SENSOR_KEY, sizeof(vibrationPackage.key));

		vibrationPackage.type = 'P';

		vibrationPackage.start = start;
		vibrationPackage.end = min(end, SAMPLES);

		vibrationPackage.axis = axis;

		LoRa_Idle(&hspi1);

		sendVibrationPackage(vibrationPackage);

		readMilliseconds = millis();
		sendDataDelay = false;

		//if (report) {
		//temperatureValue += readTemperature() / package_factor;
		current_package++;
		//reset_counter = 0;

		LoRa_Sleep(&hspi1);
		energy_save = true;
	}
}

void loop() {
	DEBUG_PRINT("loop...\r\n");
	readAndSendFRAMData();

	if (current_package >= package_factor) {
		writeVibrationInformation();
		temperatureValue = 0;
		MCP9808_ReadTemperature_LowPower(&temperatureValue);
		data.temperature = temperatureValue;
		strncpy(data.key, SENSOR_KEY, sizeof(data.key));
		data.type = 'D';

		uint16_t soc = 0;
		readBatteryData(&soc); // Passing the address of soc

		data.battery = soc;

		sendData(data);
		current_package = 0;
	}
}

void sendVibrationPackage(Transmission_VibrationPackage sendingVibrationPackage) {
	LoRa_Idle(&hspi1);

	uint8_t buffer[sizeof(Transmission_VibrationPackage)];
	memcpy(buffer, &sendingVibrationPackage,
			sizeof(Transmission_VibrationPackage));

	LoRa_Transmit(&hspi1, buffer, sizeof(Transmission_VibrationPackage));

	LoRa_Sleep(&hspi1);
}

void sendData(Transmission_Data sendingData) {
	LoRa_Idle(&hspi1);

	uint8_t buffer[sizeof(Transmission_Data)];
	memcpy(buffer, &sendingData, sizeof(Transmission_Data));

	LoRa_Transmit(&hspi1, buffer, sizeof(Transmission_Data));

	LoRa_Sleep(&hspi1);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	DEBUG_PRINT("error...\r\n");
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
		DEBUG_PRINT("error...\r\n");
		HAL_Delay(100);
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
	char bufferVib[200];
	snprintf("Wrong parameters value: file %s on line %d\r\n", file, line); // Usar %.2f para imprimir double
	DEBUG_PRINT(bufferVib);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
