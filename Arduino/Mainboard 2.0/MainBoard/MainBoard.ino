#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "EmonLib.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "Adafruit_MCP9808.h"
#include <SoftwareSerial.h>

#define DEBUG_LED 9
#define RESET_GSM 6

#define LED_ERROR_RADIO 3
#define LED_ERROR_SENSOR 2
#define LED_SUCCESS 1
#define LED_FAILURE 0

SoftwareSerial esp8266(3, 2);

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

struct __attribute__ ((packed)) Data {
  char key[5];
  float vibration[3];
  float battery;
  float current;
  float temperature;
};

Data data;

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

void setup() {
  Serial.begin(115200);
  esp8266.begin(115200);

  if (!radio.begin()) {
    esp8266.println(F("[ERROR] Radio hardware is not responding..."));
    blinkLed(LED_ERROR_RADIO);
    while (1) {}
  }

    pinMode(DEBUG_LED, OUTPUT);
    pinMode(RESET_GSM, OUTPUT);

    radio.setPALevel(RF24_PA_MAX); // Definir o nível de potência como máximo
    radio.setDataRate(RF24_250KBPS); // Definir a taxa de dados como 250 kbps
    radio.setChannel(76); // Definir o canal de comunicação para evitar interferências (escolha um valor entre 0 e 125)
    radio.setCRCLength(RF24_CRC_16); // Ativar CRC de 16 bits
    radio.openReadingPipe(1, address);
    radio.startListening();

    esp8266.println("Radio set on receiver mode...");
    digitalWrite(RESET_GSM, HIGH);
}

const unsigned long RESTART_INTERVAL = 600000;
long lastRestartTime = 0;
void loop() {
  receiverRadio();

  if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    digitalWrite(DEBUG_LED, HIGH);
    digitalWrite(RESET_GSM, LOW);
    delay(100);
    digitalWrite(RESET_GSM, HIGH);
    lastRestartTime = millis();
    delay(10000);
    digitalWrite(DEBUG_LED, LOW);
  }

  if (esp8266.available()) {
    //Serial.print("[ESP8266] -> ");
    //Serial.println(esp8266.readString());
  }
}

void receiverRadio() {
  uint8_t pipe;
  if (radio.available(&pipe)) {
    Data receivedData;
    radio.read(&receivedData, sizeof(receivedData));
    esp8266.print(F(" Recieved "));
    esp8266.print(radio.getDynamicPayloadSize());  // print incoming payload size
    esp8266.print(F(" bytes on pipe "));
    esp8266.print(pipe);  // print pipe number that received the ACK
    esp8266.println(F(": "));

    printData(receivedData);
    blinkLed(LED_SUCCESS);
  }
}

void printData(Data data) {
  esp8266.print("key: ");
  esp8266.print(data.key);
  esp8266.print(", vibration: {");
  esp8266.print(data.vibration[0]);
  esp8266.print(", ");
  esp8266.print(data.vibration[1]);
  esp8266.print(", ");
  esp8266.print(data.vibration[2]);
  esp8266.print("}, battery: ");
  esp8266.print(data.battery);
  esp8266.print(", current: ");
  esp8266.print(data.current);
  esp8266.print(", temperature: ");
  esp8266.println(data.temperature);
}

void sendJson(Data data) {
  StaticJsonDocument<128> doc;

  doc["key"] = data.key;
  doc["acc_x"] = String(data.vibration[0], 2);
  doc["acc_y"] = String(data.vibration[1], 2);
  doc["acc_z"] = String(data.vibration[2], 2);
  doc["change_rate"] = String(0.00, 2);
  doc["battery"] = String(data.battery, 2);
  doc["current"] = String(data.current, 2);
  doc["temperature"] = String(data.temperature, 2);

  serializeJson(doc, Serial);
}