#include <SoftwareSerial.h>
#include "RF24.h"

RF24 radio(7, 8);
const byte radio_address[6] = "00006";

#define TINY_GSM_MODEM_SIM7600

#define PIN_TX 3
#define PIN_RX 2
#define PWR_PIN 7

SoftwareSerial SerialAT(PIN_RX, PIN_TX);

const char apn[] PROGMEM = "zap.vivo.com.br";
const char gprs[] PROGMEM = "vivo";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#define TINY_GSM_USE_GPRS true

//#include <StreamDebugger.h>
//StreamDebugger debugger(SerialAT, Serial);
//TinyGsm modem(debugger);
TinyGsm modem(Serial);

TinyGsmClient client(modem);

#define TRANSMISSION_DATA_PACKAGE 5

struct __attribute__((__packed__)) Data {
  char type;
  char key[6];
  float rms[3];
  float temperature;
} data;

struct __attribute__((__packed__)) VibrationPackage {
  char type;
  char key[6];
  float dataPackage[TRANSMISSION_DATA_PACKAGE];
  int start;
  int end;
} vibrationPackage;

void initGSM() {
    if (!modem.waitForNetwork()) {
      delay(10000);
      return;
    }
    SerialAT.println(F(" success"));

    if (modem.isNetworkConnected()) {
      SerialAT.println(F("Network connected"));
    }
  

  if (!modem.gprsConnect(apn, gprs, gprs)) {
    return;
  }
}

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200);

  pinMode(PWR_PIN, OUTPUT);

  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);

  delay(11000);
  if (!modem.init()) {
    return;
  }

  if (!radio.begin()) {
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setCRCLength(RF24_CRC_16);
  radio.openReadingPipe(1, radio_address);
  radio.startListening();

  initGSM();
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void loop() {

  uint8_t pipe;
  if (radio.available(&pipe)) { 
    SerialAT.println();
    
    SerialAT.print(F("Recieved "));
    SerialAT.println(freeRam());
    int size = radio.getDynamicPayloadSize();

    byte packet[size];
    radio.read(&packet, size);
    
    HttpClient http(client, "ec2-52-32-217-211.us-west-2.compute.amazonaws.com", 8080);
    SerialAT.println(freeRam());
    if (packet[0] == 'D') {
      memcpy(&data, &packet, sizeof(data));  //0 | Type 1-6 | Key
      char dataJson[50];
      snprintf(dataJson, sizeof(dataJson),
               "{\"type\":\"%c\",\"key\":\"%s\",\"rms\":[%.2f,%.2f,%.2f],\"temperature\":%.2f}",
               data.type, data.key, data.rms[0], data.rms[1], data.rms[2], data.temperature);

      http.post("/data/register", "application/json", dataJson); SerialAT.print(F("post ")); 
      SerialAT.println(freeRam());
    } else {
      memcpy(&vibrationPackage, &packet, sizeof(vibrationPackage));
      String vibrationJson = "";

      vibrationJson += "{\"type\":\"";
      vibrationJson += vibrationPackage.type;
      vibrationJson += "\",\"key\":\"";
      vibrationJson += vibrationPackage.key;
      vibrationJson += "\",\"dataPackage\":[";

      for (int i = 0; i < TRANSMISSION_DATA_PACKAGE; i++) {
        vibrationJson += vibrationPackage.dataPackage[i];
        if (i < TRANSMISSION_DATA_PACKAGE - 1) {
          vibrationJson += ",";
        }
      }
      vibrationJson += "],\"start\":";
      vibrationJson += vibrationPackage.start;
      vibrationJson += ",\"end\":";
      vibrationJson += vibrationPackage.end;
      vibrationJson += "}";

      http.post("/data/register", "application/json", vibrationJson); SerialAT.print(F("post "));
      SerialAT.println(freeRam());
    }
  }
}