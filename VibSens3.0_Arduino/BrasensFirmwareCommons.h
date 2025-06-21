#ifndef SRC_BRASENSCOMMONS_H_
#define SRC_BRASENSCOMMONS_H_

class Transmission
{
  public:   
    static const int DATA_TRANSMISSION_PERIOD = 350;
    static const int TRANSMISSION_DATA_PACKAGE = 5;
    static const int SAMPLES = 12400; //2^13 8.192 or 2^14 16384

    struct __attribute__((__packed__)) Data {
      char type;  // 'D' para Data
      char key[6];
      float rms_accel[3];
      float rms_vel[3];
      float temperature;
    } data;

    struct __attribute__((__packed__)) VibrationPackage {
      char type;  // 'V' para VibrationPackage
      char key[6];
      float dataPackage[TRANSMISSION_DATA_PACKAGE];
      int start;
      int end;
    } vibrationPackage;
}; 

#endif /* SRC_BRASENSCOMMONS_H_ */