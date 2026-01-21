#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "RF24.h"

RF24 radio(7, 8);  // CE, CSN
const byte address[6] = "00001";

#define TINY_GSM_MODEM_SIM7600

#define TINY_GSM_USE_WIFI false

#define GSM_PIN ""

#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 3
#define PIN_RX 2
#define PWR_PIN 7
#define BAT_ADC 35
#define BAT_EN 12
#define PIN_RI 33
#define RESET 5

#define RESET_ATMEGA 6

SoftwareSerial SerialAT(PIN_RX, PIN_TX);

// Your GPRS credentials, if any
const char apn[] = "zap.vivo.com.br";
const char gprsUser[] = "vivo";
const char gprsPass[] = "vivo";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#define DEBUG_LED 9

#define LED_ERROR_RADIO 3
#define LED_ERROR_SENSOR 2
#define LED_SUCCESS 1
#define LED_FAILURE 0

#define TINY_GSM_USE_GPRS true

//#include <StreamDebugger.h>
//StreamDebugger debugger(SerialAT, Serial);
//TinyGsm modem(debugger);
TinyGsm modem(Serial);

const char server[] = "ec2-52-32-217-211.us-west-2.compute.amazonaws.com";
const char resource[] = "/data/register";
const int port = 8080;
const char resourceTest[] = "/data/test";

// Define o id do contexto PDP (Packet Data Protocol)
const uint8_t cid = 1;

// Define a URL para o pedido HTTP
const char url[] = "http://opinion.people.com.cn/GB/n1/2018/0815/c1003-30228758.html";

TinyGsmClient client(modem);
HttpClient http(client, server, port);

struct __attribute__((packed)) Data {
  char key[5];
  float vibration[3];
  float battery;
  float current;
  float temperature;
};

Data data;

int g = 0;
bool connectedToNetwork = false;

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
      delayTime = 50;
      blinkTime = 3;
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

void initGSM() {
  if (!connectedToNetwork) {

    SerialAT.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
      SerialAT.println(" fail");

      delay(10000);
      return;
    }
    SerialAT.println(" success");
    connectedToNetwork = true;


    if (modem.isNetworkConnected()) {
      SerialAT.println("Network connected");
    }
  }

  // GPRS connection parameters are usually set after network registration
  SerialAT.print(F("Connecting to "));
  SerialAT.println(apn);

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialAT.println("fail");

    return;
  }

  bool res = modem.isGprsConnected();
  SerialAT.print("GPRS status:");
  SerialAT.println(res ? "connected" : "not connected");
  SerialAT.println("----------------------------------------------------------------");
  String ccid = modem.getSimCCID();
  SerialAT.print("CCID:");
  SerialAT.println(ccid);

  String imei = modem.getIMEI();
  SerialAT.print("IMEI:");
  SerialAT.println(imei);

  String cop = modem.getOperator();
  SerialAT.print("Operator:");
  SerialAT.println(cop);

  IPAddress local = modem.localIP();
  SerialAT.print("Local IP:");
  SerialAT.println(local);

  int csq = modem.getSignalQuality();
  SerialAT.print("Signal quality:");
  SerialAT.println(csq);
  SerialAT.println("----------------------------------------------------------------");

  SerialAT.println("");
  SerialAT.println("success");

  if (modem.isGprsConnected()) {
    SerialAT.println("GPRS connected");
  }
  g = 1;
}

void setup() {
  // Set console baud rate
  pinMode(RESET_ATMEGA,OUTPUT);
  digitalWrite(RESET_ATMEGA, HIGH);
  Serial.begin(115200);
  SerialAT.begin(UART_BAUD);

  pinMode(DEBUG_LED, OUTPUT);
  pinMode(PWR_PIN, OUTPUT);

  digitalWrite(DEBUG_LED, HIGH);

  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);

  SerialAT.println("Wait...");

  delay(11000);

  SerialAT.println("Initializing modem...");
  if (!modem.init()) {
    SerialAT.println("Failed to restart modem, delaying 10s and retrying");
    return;
  }

  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }

  String name = modem.getModemName();
  SerialAT.print("Modem Name:");
  SerialAT.println(name);

  String modemInfo = modem.getModemInfo();
  SerialAT.print("Modem Info:");
  SerialAT.println(modemInfo);
  SerialAT.println("");
  SerialAT.println("");

  if (!radio.begin()) {
    SerialAT.println(F("[ERROR] Radio hardware is not responding..."));

    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setCRCLength(RF24_CRC_16);
  radio.openReadingPipe(1, address);
  radio.startListening();
}

unsigned long previousMillis = 0;
const unsigned long interval = 180000;
void(* resetFunc) (void) = 0;
void loop() {

  if (g == 0)
    initGSM();

      unsigned long currentMillis = millis(); // Obtém o tempo atual

  // Verifica se o intervalo de tempo desejado foi atingido
  if (currentMillis - previousMillis >= interval) {
    SerialAT.println(F("Reset"));
    resetFunc();

    // Atualize o valor de previousMillis
    previousMillis = currentMillis;
  }

  uint8_t pipe;
  if (radio.available(&pipe)) {
    Data receivedData;
    radio.read(&receivedData, sizeof(receivedData));
    SerialAT.print(F(" Recieved "));
    SerialAT.print(radio.getDynamicPayloadSize());  // print incoming payload size
    SerialAT.print(F(" bytes on pipe "));
    SerialAT.print(pipe);  // print pipe number that received the ACK
    SerialAT.println(F(": "));

    blinkLed(LED_SUCCESS);

    String jsonData = "{\"key\":\"";
    jsonData += String(receivedData.key);
    jsonData += "\",\"acc_x\":\"";
    jsonData += String(receivedData.vibration[0], 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"acc_y\":\"";
    jsonData += String(receivedData.vibration[1], 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"acc_z\":\"";
    jsonData += String(receivedData.vibration[2], 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"change_rate\":\"";
    jsonData += String(receivedData.current, 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"battery\":\"";
    jsonData += String(receivedData.battery, 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"current\":\"";
    jsonData += String(receivedData.current, 2);  // Arredonda para 2 casas decimais
    jsonData += "\",\"temperature\":\"";
    jsonData += String(receivedData.temperature, 2);  // Arredonda para 2 casas decimais
    jsonData += "\"}";

    SerialAT.println("");
    SerialAT.println("");

    SerialAT.println("making POST request");

    http.beginRequest();
    http.post(resource);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", jsonData.length());
    http.beginBody();
    http.print(jsonData);
    http.endRequest();
    http.stop();

    blinkLed(LED_ERROR_SENSOR);
  }
}