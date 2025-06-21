#include <SoftwareSerial.h>
#include "RF24.h"

RF24 radio(7, 8);
const byte radio_address[6] = "00006";
#define LOGGING

#define TINY_GSM_MODEM_SIM7600

#define TINY_GSM_USE_WIFI false

#define GSM_PIN ""

#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 3
#define PIN_RX 2
#define PWR_PIN 7

#define RESET_ATMEGA 6

SoftwareSerial SerialAT(PIN_RX, PIN_TX);

const char apn[] = "zap.vivo.com.br";
const char gprs[] = "vivo";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#define TINY_GSM_USE_GPRS true

TinyGsm modem(Serial);

const char resource[] = "/data/register";

const uint8_t cid = 1;

TinyGsmClient client(modem);
HttpClient http(client, "ec2-34-220-119-142.us-west-2.compute.amazonaws.com", 8080);

int g = 0;
bool connectedToNetwork = false;

#define TRANSMISSION_DATA_PACKAGE 5

struct __attribute__((__packed__)) Data {
  char type;  // 'D' para Data
  char key[6];
  float rms[3];
  float temperature;
} data;

struct __attribute__((__packed__)) VibrationPackage {
  char type;  // 'V' para VibrationPackage
  char key[6];
  float dataPackage[TRANSMISSION_DATA_PACKAGE];
  int start;
  int end;
} vibrationPackage;

void (*resetFunc)(void) = 0;

void initGSM() {
  if (!connectedToNetwork) {

    SerialAT.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
    //SerialAT.println(" fail");

      delay(10000);
      return;
    }
    SerialAT.println(" success");
    connectedToNetwork = true;

    if (modem.isNetworkConnected()) {
    SerialAT.println("Network connected");
    }
  }

  if (!modem.gprsConnect(apn, gprs, gprs)) {
    SerialAT.println("fail");
    return;
  }

  g = 1;
}

unsigned long previousMillis = 0;

void setup() {
  pinMode(RESET_ATMEGA, OUTPUT);
  digitalWrite(RESET_ATMEGA, HIGH);
  Serial.begin(115200);

  SerialAT.begin(UART_BAUD);

  pinMode(PWR_PIN, OUTPUT);

  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);


  delay(11000);
  SerialAT.println("Initializing modem...");
  if (!modem.init()) {
    SerialAT.println("Failed to restart modem, delaying 10s and retrying");
    return;
  }

  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
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

  previousMillis = millis();
}

bool wait = false;
void httpRequest(String json) {
  radio.stopListening();
  wait = true;

  SerialAT.println("");

  SerialAT.println("making POST request");

  http.beginRequest();
  http.post(resource);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", json.length());
  http.beginBody();
  http.print(json);
  http.endRequest();
  http.stop();

  radio.startListening();
  wait = false;
}

const unsigned long interval = 3600000;
void loop() {

  if (g == 0)
    initGSM();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
  resetFunc();

  previousMillis = currentMillis;
  }

     if (!wait) {
    uint8_t pipe;
    if (radio.available(&pipe)) {
      int size = radio.getDynamicPayloadSize();

      byte packet[size];
      radio.read(&packet, size);

      if (packet[0] == 'D') {
        memcpy(&data, &packet, sizeof(data));  //0 | Type 1-6 | Key
        String dataJson = "";

        dataJson += "{\"type\":\"";
        dataJson += data.type;
        dataJson += "\",\"key\":\"";
        dataJson += data.key;
        dataJson += "\",\"rms\":[";
        dataJson += data.rms[0];
        dataJson += ",";
        dataJson += data.rms[1];
        dataJson += ",";
        dataJson += data.rms[2];
        dataJson += "],\"temperature\":";
        dataJson += data.temperature;
        dataJson += "}";
        SerialAT.println(dataJson);  
        httpRequest(dataJson);
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
        SerialAT.println(vibrationJson);  
        httpRequest(vibrationJson);
      }
    }
  }
}