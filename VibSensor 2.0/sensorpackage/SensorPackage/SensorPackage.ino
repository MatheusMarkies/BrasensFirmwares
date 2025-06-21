#include <SPI.h>
#include "RF24.h"
#include "EmonLib.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "Adafruit_MCP9808.h"
#include <SoftwareSerial.h>
#include "LowPower.h"

#define DEBUG_LED 9

#define LED_ERROR_RADIO 3
#define LED_ERROR_SENSOR 2
#define LED_SUCCESS 1
#define LED_FAILURE 0

#define DELAY_FOR_DATA_TRANSFER_SEG 30
#define SENSOR_KEY "0000A"

RF24 radio(7, 8);

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

#include "MAX17048.h"

MAX17048 pwr_mgmt;

const byte address[6] = "00001";

struct __attribute__((packed)) Data {
  char key[6];
  float vibration[3];
  float battery;
  float current;
  float temperature;
};

typedef struct {
  float x;
  float y;
  float z;
} Vibration;

Data data;

#include <arduinoFFT.h>

arduinoFFT FFT = arduinoFFT();
#define SAMPLES 124             //Must be a power of 2
#define SAMPLING_FREQUENCY 1600  //Hz. Determines maximum frequency that can be analysed by the FFT.


unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

int delay_int = 0;

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
      delayTime = 2000;
      blinkTime = 20;
      break;
  }

  for (int i = 0; i < blinkTime; i++) {
    digitalWrite(DEBUG_LED, HIGH);
    delay(delayTime);
    digitalWrite(DEBUG_LED, LOW);
    if (i < blinkTime - 1) {
      delay(delayTime);
    }
  }
}

void radioSetup() {
  if (!radio.begin()) {
    Serial.println(F("[ERROR] Radio hardware is not responding..."));
    blinkLed(LED_ERROR_RADIO);
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);    // Definir o nível de potência como máximo
  radio.setDataRate(RF24_250KBPS);  // Definir a taxa de dados como 250 kbps
  radio.setChannel(76);             // Definir o canal de comunicação para evitar interferências (escolha um valor entre 0 e 125)
  radio.setRetries(15, 15);         // Definir o número de retransmissões e o atraso entre as tentativas
  radio.setCRCLength(RF24_CRC_16);  // Ativar CRC de 16 bits
  radio.openWritingPipe(address);
  radio.stopListening();
  Serial.println("Radio set on transmitter mode...");
}

void aDXL345Setup() {
  if (!accel.begin()) {
    Serial.println("[ERROR] No ADXL345 detected...");
    blinkLed(LED_ERROR_SENSOR);
    while (1)
      ;
  }
  Serial.println("Found ADXL345!");

  accel.setRange(ADXL345_RANGE_8_G);
  accel.setDataRate(ADXL345_DATARATE_3200_HZ);
}

void mCP9808Setup() {
  if (!tempsensor.begin(0X18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    blinkLed(LED_ERROR_SENSOR);
    while (1)
      ;
  }

  Serial.println("Found MCP9808!");

  tempsensor.setResolution(3);
}

void mAX17048Setup() {
  Wire.begin(0X76);
  pwr_mgmt.attatch(Wire);
}

void setup() {
  Serial.begin(115200);

  mAX17048Setup();
  aDXL345Setup();
  mCP9808Setup();

  radioSetup();

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  delay_int = round(DELAY_FOR_DATA_TRANSFER_SEG / 8);
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

double voltage = 0;     // Variable to keep track of LiPo voltage
double soc = 0;         // Variable to keep track of LiPo state-of-charge (SOC)
double changeRate = 0;  // Variable to keep track of LiPo state-of-charge (SOC)
bool alert;             // Variable to keep track of whether alert has been triggered
double readBattery() {
  soc = pwr_mgmt.percent();
  return soc;
}

double readBatteryChangeRate() {
  //changeRate = pwr_mgmt.voltage();
  return changeRate;
}

void getVibrationInfo() {
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();

    Vibration vibration = readVibration();

    

    while (micros() < (microseconds + sampling_period_us)) {}
  }
}

int idleTimer = 800;
void loop() {
  idleTimer += 1;

  if (idleTimer >= delay_int) {
    radio.powerUp();
    transmitterRadio();

    radio.powerDown();
    //avr_enter_sleep_mode(); // Custom function to sleep the device
    idleTimer = 0;
  }

  if (idleTimer == (delay_int - 1)) {
    createFFT();
  }

  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF,
  //TIMER1_OFF, TIMER0_OFF, SPI_OFF,
  //USART0_OFF, TWI_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void transmitterRadio() {
  Vibration vibration = readVibration();

  strncpy(data.key, SENSOR_KEY, sizeof(data.key));

  data.vibration[0] = vibration.x;
  data.vibration[1] = vibration.y;
  data.vibration[2] = vibration.z;
  data.current = 0;
  //lipo.wake();
  //data.changeRate = ;
  data.battery = (float)readBattery();
  //lipo.sleep();
  data.temperature = (float)readTemperature();
  printData(data);
  sendData(data);
}

void printData(Data data) {
  Serial.print("key: ");
  Serial.print(data.key);
  Serial.print(", vibration: {");
  Serial.print(data.vibration[0]);
  Serial.print(", ");
  Serial.print(data.vibration[1]);
  Serial.print(", ");
  Serial.print(data.vibration[2]);
  Serial.print("}, battery: ");
  Serial.print(data.battery);
  Serial.print(", current: ");
  Serial.print(data.current);
  Serial.print(", temperature: ");
  Serial.println(data.temperature);
}

bool sendData(Data sendingData) {
  unsigned long start_timer = micros();
  bool report = radio.write(&sendingData, sizeof(sendingData));
  unsigned long end_timer = micros();

  if (report) {
    //Serial.print(F("Transmission successful! "));
    //Serial.print(F("Time to transmit = "));
    //Serial.print(end_timer - start_timer);
    blinkLed(LED_SUCCESS);
  } else {
    //Serial.println(F("Transmission failed or timed out"));
    blinkLed(LED_FAILURE);
  }
  return report;
}