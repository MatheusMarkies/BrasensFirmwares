/*
 * BrasensFirmware.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */


#ifndef SRC_BRASENSCOMMONS_H_
#define SRC_BRASENSCOMMONS_H_

#define DATA_TRANSMISSION_PERIOD 350
#define TRANSMISSION_DATA_PACKAGE 10
#define SAMPLES 1024 // 2^13 8.192 ou 2^14 16384

typedef struct{
    char type;  // 'D' para Data
    char key[6];
    float rms_accel[3];
    float rms_vel[3];
    float temperature;
    float battery;
} Transmission_Data;

typedef struct{
    char type;  // 'V' para VibrationPackage
    char key[6];
    int axis; //X = 0, Y = 1, Z = 2
    float dataPackage[TRANSMISSION_DATA_PACKAGE];
    int start;
    int end;
} Transmission_VibrationPackage;

#endif /* SRC_BRASENSCOMMONS_H_ */
