#include <ArduinoJson.h>
#include "RF24.h"

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

/*
   Tests enabled
*/
#define TINY_GSM_TEST_GPRS    true
#define TINY_GSM_TEST_GPS     true
#define TINY_GSM_POWERDOWN    true

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "zap.vivo.com.br";     //SET TO YOUR APN
const char gprsUser[] = "vivo";
const char gprsPass[] = "vivo";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     14
#define SD_CS       13
#define LED_PIN     12

#define NR_MISO     39
#define NR_MOSI     36
#define NR_SCLK     0
#define NR_CS     12

int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;
RF24 radio(2,5);

#define DEBUG_LED 39

const char server[] = "ec2-52-32-217-211.us-west-2.compute.amazonaws.com";
const char resource[] = "/data/register";
const int  port = 8080;

TinyGsmClient client(modem);
HttpClient http(client, server, port);

#include <ArduinoJson.h>

struct __attribute__ ((packed)) Data {
  char key[1];
  float vibration[3];
  float battery;
  float current;
  float temperature;
};

Data data;
const byte address[6] = "00001";
void convertJsonToData() {
  StaticJsonDocument<256> doc;

  DeserializationError error = deserializeJson(doc, Serial);

  if (error)
     return;

     // Lê os valores do JSON e preenche a estrutura Data
    Data receivedData;
    strncpy(receivedData.key, doc["key"], sizeof(receivedData.key));
    receivedData.vibration[0] = doc["vibration"][0];
    receivedData.vibration[1] = doc["vibration"][1];
    receivedData.vibration[2] = doc["vibration"][2];
    receivedData.battery = doc["battery"];
    receivedData.current = doc["current"];
    receivedData.temperature = doc["temperature"];

    // Faça o que for necessário com os dados recebidos
    // Por exemplo, exiba os valores:
    Serial.println("Dados recebidos:");
    Serial.print("Key: ");

    Serial.println(receivedData.key[0]);
    Serial.print("Vibration: ");
    for (int i = 0; i < 3; i++) {
      Serial.print(receivedData.vibration[i]);
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Battery: ");
    Serial.println(receivedData.battery);
    Serial.print("Current: ");
    Serial.println(receivedData.current);
    Serial.print("Temperature: ");
    Serial.println(receivedData.temperature);

  sendDataToServer(receivedData);
}

void disconnectGPRS() {
    if (modem.isGprsConnected()) {
        modem.gprsDisconnect();
    }
    modem.restart();
}

void sendDataToServer(Data rdata) {
  if (modem.isGprsConnected()) {
      StaticJsonDocument<256> jsonDoc;

    String a = String(rdata.key[0]);

    jsonDoc["key"] = a;
    jsonDoc["acc_x"] = String(rdata.vibration[0], 2);
    jsonDoc["acc_y"] = String(rdata.vibration[1], 2);
    jsonDoc["acc_z"] = String(rdata.vibration[2], 2);

    jsonDoc["battery"] = String(rdata.battery, 2);
    jsonDoc["current"] = String(rdata.current, 2);
    jsonDoc["temperature"] = String(rdata.temperature, 2);

    String jsonString;
    serializeJson(jsonDoc, jsonString);

Serial.println("");
Serial.println("");
Serial.println("");
Serial.println("");

Serial.println(jsonString);

Serial.println("");
Serial.println("");
Serial.println("");
Serial.println("");

    http.post(resource, "application/json", jsonString);
    int httpResponseCode = http.responseStatusCode();

    String responsePayload = http.responseBody();

  } else {
    Serial.println("Gprs Disconnected");
  }
}

void restartModem(){
Serial.println("Initializing modem...");
    if (!modem.restart()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
    }

    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }

    Serial.println("Connecting to network...");
    if (!modem.waitForNetwork()) {
        delay(10000);
        return;
    }

    Serial.println("Connecting to GPRS...");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        delay(10000);
        return;
    }

    Serial.println("GPRS status: ");
    if (modem.isGprsConnected()) {
        Serial.println("Connected");
    } else {
        Serial.println("Not connected");
    }

  delay(1000);
  Serial.println("Wating....");
  }

void setup()
{
    // Set console baud rate
    Serial.begin(115200);
    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    // Starting the machine requires at least 1 second of low level, and with a level conversion, the levels are opposite
    delay(1000);
    digitalWrite(PWR_PIN, LOW);

    Serial.println("\nWait...");

    delay(1000);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    restartModem();
}
const unsigned long RESTART_INTERVAL = 600000; // 10 minutos em milissegundos
long lastRestartTime = 0;
void loop(){
        if (millis() - lastRestartTime >= RESTART_INTERVAL) {
            disconnectGPRS(); // Desconecta o GPRS após o envio dos dados
            delay(10000);
            restartModem();
            lastRestartTime = millis(); // Reiniciar o timer para o próximo intervalo
        }
  if (Serial.available()) {
    convertJsonToData();
  }
}