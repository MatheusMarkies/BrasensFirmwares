/*
 * BrasensFirmware.h
 *
 *  Created on: Jun 12, 2024
 *      Author: Matheus Markies
 */


#ifndef SRC_BRASENSCOMMONS_H_
#define SRC_BRASENSCOMMONS_H_

#define DATA_TRANSMISSION_PERIOD 350
#define TRANSMISSION_DATA_PACKAGE 60
#define SAMPLES 2048 // 2^13 8.192 ou 2^14 16384

typedef struct __attribute__((__packed__)) {
    char type;  // 'D' para Data
    char key[6];
    double rms_accel[3];
    double rms_vel[3];
    double temperature;
    double battery;
} Transmission_Data;

typedef struct __attribute__((__packed__)) {
    char type;  // 'V' para VibrationPackage
    char key[6];
    int axis; //X = 0, Y = 1, Z = 2
    double dataPackage[TRANSMISSION_DATA_PACKAGE];
    int start;
    int end;
} Transmission_VibrationPackage;

#endif /* SRC_BRASENSCOMMONS_H_ */
