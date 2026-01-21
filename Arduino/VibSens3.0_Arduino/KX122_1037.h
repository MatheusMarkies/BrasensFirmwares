#ifndef SRC_KX122_H_
#define SRC_KX122_H_

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>

TwoWire *_i2cPort;

typedef struct {
  float x;
  float y;
  float z;
} Vibration;

class KX122{
  public:
    static const int ACC_DATA_RATE = 25800;
};

#define KX122_I2C_ADDRESS 0x1F
#define KX122_ODR_SPEED 0x1B

#define KX122_XOUT_L 0x06
#define KX122_XOUT_H 0x07

#define KX122_YOUT_L 0x08
#define KX122_YOUT_H 0x09

#define KX122_ZOUT_L 0x0A
#define KX122_ZOUT_H 0x0B

void KX122_Init();
Vibration KX122_ReadAccelData();
uint8_t KX122_WriteRegister(uint8_t reg, uint8_t data);
uint8_t KX122_ReadRegister(uint8_t reg);

#endif /* SRC_KX122_H_ */