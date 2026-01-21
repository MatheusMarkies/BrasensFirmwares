#include "KX122_1037.h"

void KX122_Init() {
    KX122_WriteRegister( 0x18, 0x00 );
    KX122_WriteRegister( 0x18, 0x80 );
    KX122_WriteRegister( 0x19, 0x00 );
    KX122_WriteRegister( 0x1A, 0x00 );
    KX122_WriteRegister( KX122_ODR_SPEED, 0x0F ); //25.8KHz
}

Vibration KX122_ReadAccelData() {
    uint8_t data[6];

    Vibration vibData;
    
    data[0] =KX122_ReadRegister(KX122_XOUT_L);
    data[1] =KX122_ReadRegister(KX122_XOUT_H);

    data[2] =KX122_ReadRegister(KX122_YOUT_L);
    data[3] =KX122_ReadRegister(KX122_YOUT_H);

    data[4] = KX122_ReadRegister(KX122_ZOUT_L);
    data[5] = KX122_ReadRegister(KX122_ZOUT_H);

    //Bitwise 00010010 00000000 | 00000000 00110100 = 00010010 00110100
    //LSB (data[5] << 8)
    vibData.x = (int16_t)((data[1] << 8) | data[0]);
    vibData.y = (int16_t)((data[3] << 8) | data[2]);
    vibData.z = (int16_t)((data[5] << 8) | data[4]);

    return vibData;
}

uint8_t KX122_WriteRegister(uint8_t reg, uint8_t data) {
    _i2cPort->beginTransmission(KX122_I2C_ADDRESS);
    _i2cPort->write(reg);
    _i2cPort->write(data);
    return (_i2cPort->endTransmission());
}

uint8_t KX122_ReadRegister(uint8_t reg) {
  bool success = false;
  uint8_t retries = 3;
  uint8_t result = 0;

  while ((success == false) && (retries > 0)) {
    _i2cPort->beginTransmission(KX122_I2C_ADDRESS);
    _i2cPort->write(reg); 
    _i2cPort->endTransmission(false); 

    if (_i2cPort->requestFrom(KX122_I2C_ADDRESS, (uint8_t)1) == 1) { 
      result = _i2cPort->read();
      success = true;
    } else {
      retries--;
    }
  }

  return result;
}

