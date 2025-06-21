#include <BrasensFirmwareCommons.h>
#include <KX122_1037.h>
#include <SPI.h>
#include <LoRa_STM32.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <Adafruit_MCP9808.h>

#include <Wire.h>
#include "FRAM.h"
#include <math.h>

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

Transmission::Data data;
Transmission::VibrationPackage vibrationPackage;

unsigned int sampling_period_us;
unsigned long data_sender_period;

int package_factor = 0;
int acc_sample_factor = 0;

uint32_t sizeInBytes = 0;

#define SENSOR_KEY "0000D"

#define SS PA4
#define RST PB0
#define DI0 PA1
#define TX_POWER 17
#define BAND 915E6
#define ENCRYPT 0x78

FRAM fram;
#define FRAM_I2C_ADDRESS 0x50

void setup() {
  Wire.begin();
  Serial.begin(115200);
  while (!Serial);

  KX122_Init();
  if (!tempsensor.begin(0X18)) {}

  Serial.println("LoRa Sender");
 
  LoRa.setTxPower(TX_POWER);
  LoRa.setSyncWord(ENCRYPT);
  
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) 
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.sleep();

  int rv = fram.begin(FRAM_I2C_ADDRESS);
  if (rv != 0) {
    //printLog("INIT ERROR: ", false);
  }

  package_factor = ceil((float)Transmission::SAMPLES / (float)Transmission::TRANSMISSION_DATA_PACKAGE);
  data_sender_period = round(1000 * ((float)Transmission::DATA_TRANSMISSION_PERIOD / (float)package_factor));
  sampling_period_us = round(1000000 * (1.0 / (float)KX122::ACC_DATA_RATE));
  acc_sample_factor = floor((float)KX122::ACC_DATA_RATE / (float)Transmission::SAMPLES);

  sizeInBytes = fram.getSize() * 1024;

  for (uint16_t addr = 0; addr < sizeInBytes; addr++) {
    fram.write8(addr, 0x00);
  }

  writeVibrationInformation();
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
  for (int i = 0; i < Transmission::SAMPLES; i++) {
    writeMicroseconds = micros();

    Vibration vibration = KX122_ReadAccelData();
    
    count++;
    
    if(count >= acc_sample_factor){
      address = (d) * sizeof(float);
      fram.writeFloat(address, (float)sqrt(vibration.x * vibration.x + vibration.y * vibration.y + vibration.z * vibration.z));
      d++;
      count = 0;
  }

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

    while (micros() < (writeMicroseconds + sampling_period_us)) {}
  }

  data.rms_accel[0] = sqrt(data.rms_accel[0] / Transmission::SAMPLES);
  data.rms_accel[1] = sqrt(data.rms_accel[1] / Transmission::SAMPLES);
  data.rms_accel[2] = sqrt(data.rms_accel[2] / Transmission::SAMPLES);

  data.rms_vel[0] = sqrt(data.rms_vel[0] / Transmission::SAMPLES);
  data.rms_vel[1] = sqrt(data.rms_vel[1] / Transmission::SAMPLES);
  data.rms_vel[2] = sqrt(data.rms_vel[2] / Transmission::SAMPLES);
}

unsigned long readMilliseconds;

bool energy_save = false;

float temperatureValue;
int current_package = 0;
int reset_counter = 0;
bool sendDataDelay = false;
void readAndSendFRAMData() {

  if (energy_save) {
    readMilliseconds = millis();
    sendDataDelay = true;
    energy_save = false;
  } else if (millis() > (readMilliseconds + data_sender_period)) {
    sendDataDelay = true;
  }

  if (sendDataDelay) {
    bool report = false;

      int start = Transmission::TRANSMISSION_DATA_PACKAGE * current_package;
      int end = (start + Transmission::TRANSMISSION_DATA_PACKAGE);

      int k = 0;
      for (int i = (start); i < (end); i += 1) {

        address = i * sizeof(float);
        float value = 0.0;

        if (address < sizeInBytes) {
          value = fram.readFloat(address);
        }

        vibrationPackage.dataPackage[(i - start)] = value;
      }

    strncpy(vibrationPackage.key, SENSOR_KEY, sizeof(vibrationPackage.key));

    vibrationPackage.type = 'P';

    vibrationPackage.start = start;
    vibrationPackage.end = min(end, Transmission::SAMPLES);

    LoRa.idle();

    sendVibrationPackage(vibrationPackage);

    readMilliseconds = millis();
    sendDataDelay = false;

    //if (report) {
      //temperatureValue += readTemperature() / package_factor;
      current_package++;
      //reset_counter = 0;

  LoRa.sleep();
  energy_save = true;
}
}

void loop() {
  readAndSendFRAMData();

  if (current_package >= package_factor) {
    //Get new Package
    writeVibrationInformation();

    //data.temperature = readTemperature();
    temperatureValue = 0;
    strncpy(data.key, SENSOR_KEY, sizeof(data.key));

    data.type = 'D';
    sendData(data);

    current_package = 0;
  }
}

bool sendVibrationPackage(Transmission::VibrationPackage sendingVibrationPackage) {
  LoRa.idle();

  uint8_t buffer[sizeof(Transmission::VibrationPackage)];
  memcpy(buffer, &sendingVibrationPackage, sizeof(Transmission::VibrationPackage));

  LoRa.beginPacket();
  LoRa.write(buffer, sizeof(Transmission::VibrationPackage));
  LoRa.endPacket();

  LoRa.sleep();
  return false;
}

bool sendData(Transmission::Data sendingData) {
  LoRa.idle();

  uint8_t buffer[sizeof(Transmission::Data)];
  memcpy(buffer, &sendingData, sizeof(Transmission::Data));

  LoRa.beginPacket();
  LoRa.write(buffer, sizeof(Transmission::Data));
  LoRa.endPacket();

  LoRa.sleep();
  return false;
}
