#include <Wire.h>
#include "FRAM.h"
#include <math.h>

#include <SPI.h>
#include "RF24.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "Adafruit_MCP9808.h"
#include "LowPower.h"

#define DEBUG_LED 9

#define LED_ERROR_RADIO 3
#define LED_ERROR_SENSOR 2
#define LED_SUCCESS 1
#define LED_FAILURE 0

#define SENSOR_KEY "0000D"

RF24 radio(7, 8);

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

#include "MAX17048.h"

MAX17048 pwr_mgmt;

const byte radio_address[6] = "00006";

FRAM fram;

#define FRAM_I2C_ADDRESS 0x50

//Data Transmission Parameters
#define DATA_TRANSMISSION_PERIOD 350
#define TRANSMISSION_DATA_PACKAGE 5
#define SAMPLES 1024
#define ACC_DATA_RATE 1600

unsigned int sampling_period_us;
unsigned long data_sender_period;

int package_factor = 0;
int acc_sample_factor = 0;

uint32_t sizeInBytes = 0;

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

typedef struct {
  float x;
  float y;
  float z;
} Vibration;

void blinkLed(int type) {
  int delayTime = 0;
  int blinkTime = 0;

  switch (type) {
    case LED_SUCCESS:
      delayTime = 100;
      blinkTime = 1;
      break;
    case LED_FAILURE:
      delayTime = 200;
      blinkTime = 2;
      break;
    case LED_ERROR_SENSOR:
      delayTime = 500;
      blinkTime = 20;
      break;
    case LED_ERROR_RADIO:
      delayTime = 1000;
      blinkTime = 20;
      break;
  }

  for (int i = 0; i < blinkTime; i++) {
    //digitalWrite(DEBUG_LED, HIGH);
    delay(delayTime);
    //digitalWrite(DEBUG_LED, LOW);
    if (i < blinkTime - 1) {
      delay(delayTime);
    }
  }
}

void printLog(String log, bool type) {
  if (type) {
    Serial.println(log);
  } else {
    Serial.print(log);
  }
}

void customDelay(unsigned long milliseconds) {
  unsigned long startTime = millis();

  while (millis() - startTime < milliseconds) {}
}

void (*resetFunc)(void) = 0;

void radioSetup() {
  if (!radio.begin()) {
    printLog("[ERROR] Radio hardware is not responding...", true);
    blinkLed(LED_ERROR_RADIO);

    resetFunc();
  }
  //RF24_PA_MIN , RF24_PA_MAX
  radio.setPALevel(RF24_PA_MAX);    // Definir o nível de potência como máximo
  radio.setDataRate(RF24_250KBPS);  // Definir a taxa de dados como 250 kbps
  radio.setChannel(76);             // Definir o canal de comunicação para evitar interferências (escolha um valor entre 0 e 125)
  radio.setRetries(15, 15);         // Definir o número de retransmissões e o atraso entre as tentativas
  radio.setCRCLength(RF24_CRC_16);  // Ativar CRC de 16 bits
  radio.openWritingPipe(radio_address);
  radio.stopListening();
  printLog("Radio set on transmitter mode...", true);
}

void aDXL345Setup() {
  if (!accel.begin()) {
    printLog("[ERROR] No ADXL345 detected...", true);
    blinkLed(LED_ERROR_SENSOR);
    resetFunc();
  }
  printLog("Found ADXL345!", true);

  accel.setRange(ADXL345_RANGE_16_G);
  accel.setDataRate(ADXL345_DATARATE_1600_HZ);
}

void mCP9808Setup() {
  if (!tempsensor.begin(0X18)) {
    printLog("Couldn't find MCP9808! Check your connections and verify the address is correct.", true);
    blinkLed(LED_ERROR_SENSOR);
    //resetFunc();
  }

  printLog("Found MCP9808!", true);

  //tempsensor.setResolution(2);
}

bool hasFRAM = false;

void setup() {
  Wire.begin();

  Serial.begin(115200);

  int rv = fram.begin(FRAM_I2C_ADDRESS);
  if (rv != 0) {
    printLog("INIT ERROR: ", false);
    printLog(String(rv), true);
    hasFRAM = false;
  } else
    hasFRAM = true;


  mCP9808Setup();
  aDXL345Setup();
  radioSetup();

  radio.powerDown();

  package_factor = ceil((float)SAMPLES / (float)TRANSMISSION_DATA_PACKAGE);
  data_sender_period = round(1000 * ((float)DATA_TRANSMISSION_PERIOD / (float)package_factor));
  //sampling_period_us = round(1000000 * (1.0 / ACC_DATA_RATE));
  sampling_period_us = round(1000000 * (1.0 / (float)SAMPLES));
  acc_sample_factor = floor((float)ACC_DATA_RATE / (float)SAMPLES);

  printLog("", true);
  ;
  printLog("data period: ", false);
  printLog(String(data_sender_period), true);
  printLog("sampling period: ", false);
  printLog(String(sampling_period_us), true);
  printLog("", true);

  sizeInBytes = fram.getSize() * 1024;

  //  clear FRAM
  for (uint32_t addr = 0; addr < sizeInBytes; addr++) {
    fram.write8(addr, 0x00);
  }

  writeVibrationInformation();
}

float readTemperature() {
  tempsensor.wake();
  float c = tempsensor.readTempC();
  tempsensor.shutdown_wake(1);
  return c;
}

Vibration readVibration() {
  Vibration vibration;

  sensors_event_t event;
  accel.getEvent(&event);
  vibration.x = event.acceleration.x;
  vibration.y = event.acceleration.y;
  vibration.z = event.acceleration.z;
  return vibration;
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

  for (int i = 0; i < SAMPLES; i++) {
    writeMicroseconds = micros();

    Vibration vibration = readVibration();

    address = (i) * sizeof(float);
    fram.writeFloat(address, (float)sqrt(vibration.x * vibration.x + vibration.y * vibration.y + vibration.z * vibration.z));

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

  data.rms_accel[0] = sqrt(data.rms_accel[0] / SAMPLES);
  data.rms_accel[1] = sqrt(data.rms_accel[1] / SAMPLES);
  data.rms_accel[2] = sqrt(data.rms_accel[2] / SAMPLES);

  data.rms_vel[0] = sqrt(data.rms_vel[0] / SAMPLES);
  data.rms_vel[1] = sqrt(data.rms_vel[1] / SAMPLES);
  data.rms_vel[2] = sqrt(data.rms_vel[2] / SAMPLES);
}

void printVibrationPackage(VibrationPackage vp) {
  printLog("VibrationPackage:", true);
  printLog("Size: ", false);
  printLog(String(sizeof(vp)), false);
  printLog(" Bytes", true);
  printLog("Type: ", false);
  printLog(String(vp.type), true);
  printLog("Key: ", false);
  printLog(String(vp.key), true);
  printLog("Data Package: ", false);
  for (int i = 0; i < TRANSMISSION_DATA_PACKAGE; i++) {
    printLog(String(vp.dataPackage[i]), false);
    printLog(" ", false);
  }
  printLog("", true);
  printLog("A: ", false);
  printLog(String(vp.start), true);
  printLog("B: ", false);
  printLog(String(vp.end), true);

  printLog("", true);
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

    if (hasFRAM) {

      int start = TRANSMISSION_DATA_PACKAGE * current_package;
      int end = (start + TRANSMISSION_DATA_PACKAGE);

      int k = 0;
      //address = 0;
      for (int i = (start); i < (end); i += 1) {

        address = i * sizeof(float);

        //printLog("address: ", false);
        //printLog(String(address), true);

        float value = 0.0;

        if (address < sizeInBytes) {
          value = fram.readFloat(address);

          //printLog("value: ", false);
          //printLog(String(value), true);
        }

        vibrationPackage.dataPackage[(i - start)] = value;
      }

      strncpy(vibrationPackage.key, SENSOR_KEY, sizeof(vibrationPackage.key));

      vibrationPackage.type = 'P';

      vibrationPackage.start = start;
      vibrationPackage.end = min(end, SAMPLES);

      printVibrationPackage(vibrationPackage);

      radio.powerUp();

      int a = 0;
      int countSendMaxTries = 200;
      if (start == 0)
        countSendMaxTries = 60;

      while (!report) {
        report = radio.write(&vibrationPackage, sizeof(vibrationPackage));
        delay(20);
        reset_counter++;
        if (reset_counter >= countSendMaxTries) {
          radio.powerDown();
          if (start != 0)
            resetFunc();
          else {
            a++;
            if (a >= 10)
              resetFunc();
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
          }
        }
      }
    }

    readMilliseconds = millis();
    sendDataDelay = false;

    if (report) {
      temperatureValue += readTemperature() / package_factor;
      current_package++;
      reset_counter = 0;
    } else {
      reset_counter++;
      if (reset_counter >= 80)
        resetFunc();
    }
  }

  radio.powerDown();
  energy_save = true;
}

void loop() {

  readAndSendFRAMData();

  if (current_package >= package_factor) {
    //Get new Package
    writeVibrationInformation();

    data.temperature = readTemperature();
    temperatureValue = 0;
    strncpy(data.key, SENSOR_KEY, sizeof(data.key));

    data.type = 'D';

    sendData(data);

    current_package = 0;
  }

  if (energy_save) {
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    //LowPower.idle(SLEEP_4S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
    //SPI_OFF, USART0_OFF, TWI_OFF);
  }
  //SLEEP_15MS,
  //SLEEP_30MS,
  //SLEEP_60MS,
  //SLEEP_120MS,
  //SLEEP_250MS,
  //SLEEP_500MS,
  //SLEEP_1S,
  //SLEEP_2S,
  //SLEEP_4S
}

bool sendData(Data sendingData) {
  radio.powerUp();

  bool report = false;

  while (!report) {
    report = radio.write(&sendingData, sizeof(sendingData));
    delay(20);
  }

  radio.powerDown();
  energy_save = true;
  return report;
}