/*
 * FRAM.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */

#ifndef INC_GSM_H_
#define INC_GSM_H_

#include "stm32l0xx.h"

const uint32_t timeOut =10000;

const char apn[]  = "live.vodafone.com";
const char server[] = "nizarmohideen.atwebpages.com";
const int  port = 80;
const char resource[] = "/insert.php";

char content[80];
char ATcommand[80];
uint8_t buffer[100] = {0};
uint8_t ATisOK = 0;
uint8_t CGREGisOK = 0;
uint8_t CIPOPENisOK = 0;
uint8_t NETOPENisOK = 0;
uint32_t previousTick;
uint16_t distance;

uint8_t sendAT(char command[], char answer[]) {
	uint8_t ATisOK = 0;
	sendTick = HAL_GetTick();

	HAL_UART_Receive_IT(&huart2, &Rx_data, 1);

	uint8_t commandBuffer[200] = { 0 };
	memcpy(commandBuffer, (uint8_t*) command, strlen(command) + 1);

	DEBUG_PRINT("Sending AT Command: \r\n");
	DEBUG_PRINT(command);

	while (!ATisOK) {
		if (gsmStatus == FREE) {
			HAL_UART_Transmit_IT(&huart2, commandBuffer, sizeof(commandBuffer));
		}

		if (gsmStatus >= WAITING) {
			if (gsmStatus == RX) {
				if (strstr((char*) GSM_RX_Buffer, answer)) {
					resetBuffers();
					ATisOK = 1;
				}
			}

			if (HAL_GetTick() >= sendTick + timeOut) {
					DEBUG_PRINT("TimeOut\r\n");
					resetBuffers();
					gsmStatus = FREE;
					break;
			}
		}
		HAL_Delay(50);
	}

	resetBuffers();
	gsmStatus = FREE;

	return ATisOK;
}

#endif
