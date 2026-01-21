#include <SoftwareSerial.h>

#define LED_ERROR_RADIO 3
#define LED_ERROR_SENSOR 2
#define LED_SUCCESS 1
#define LED_FAILURE 0
#define DEBUG_LED 9

#define LOGGING

#define TINY_GSM_MODEM_SIM7600

#define TINY_GSM_USE_WIFI false

#define GSM_PIN ""

#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 3
#define PIN_RX 2
#define PIN_DEBUG_TX 5
#define PIN_DEBUG_RX 4
#define PWR_PIN 7

#define RESET_ATMEGA 6

int requestState;

SoftwareSerial SerialAT(PIN_RX, PIN_TX);
//SoftwareSerial SerialDebug(PIN_DEBUG_RX, PIN_DEBUG_TX);

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

typedef union DataI2C_t {
 Data datastruct;
 uint8_t I2CPacket[sizeof(Data)];
};

typedef union VibrationPackageI2C_t {
 VibrationPackage datastruct;
 uint8_t I2CPacket[sizeof(VibrationPackage)];
};

DataI2C_t readdata;
VibrationPackageI2C_t readvibdata;

void (*resetFunc)(void) = 0;

void initGSM() {
  if (!connectedToNetwork) {
    //SerialDebug.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
      //SerialDebug.print("fail!");
      delay(10000);
      return;
    }//SerialDebug.println("success");
    connectedToNetwork = true;
    if (modem.isNetworkConnected()) {//SerialDebug.print("Network connected!");
    }
  }//SerialDebug.println("GPRS Connecting...");
  if (!modem.gprsConnect(apn, gprs, gprs)) {//SerialDebug.println("fail");
    return;
  }
  //SerialDebug.println("success");
  g = 1;
}

unsigned long previousMillis = 0;

void blinkLed(int type) {
  int delayTime = 0;
  int blinkTime = 0;

  switch (type) {
    case LED_SUCCESS:
      delayTime = 100;
      blinkTime = 1;
      break;
    case LED_FAILURE:
      delayTime = 50;
      blinkTime = 2;
      break;
    case LED_ERROR_SENSOR:
      delayTime = 100;
      blinkTime = 20;
      break;
    case LED_ERROR_RADIO:
      delayTime = 200;
      blinkTime = 10;
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
  pinMode(RESET_ATMEGA, OUTPUT);
  digitalWrite(RESET_ATMEGA, HIGH);
  Serial.begin(115200);

  SerialAT.begin(9600);
  //SerialDebug.begin(115200);

  pinMode(PWR_PIN, OUTPUT);

  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);


  delay(11000);
  if (!modem.init()) {
    //SerialDebug.println("");
    return;
  }

  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }

  previousMillis = millis();
}
bool waitingForNX = true;
bool wait = false;
void httpRequest(String json) {
  wait = true;

  http.beginRequest();
  http.post(resource);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", json.length());
  http.beginBody();
  http.print(json);
  http.endRequest();
  http.stop();

  int statusCode = http.responseStatusCode();
  
  sendSerialCommand("OK"); 
  requestState = 0; 
  waitingForNX = true;
 
  wait = false;
}

void sendSerialCommand(String cmd){
    SerialAT.println(cmd);
    //SerialDebug.println();
    //SerialDebug.print("Send: ");
    //SerialDebug.println(cmd);
    blinkLed(LED_FAILURE);
}

void processReceivedData(const byte* packet) {
    char objectType = packet[0];

    switch (objectType) {
        case 'D': {
        Data receivedData;
        memcpy(&receivedData, packet, sizeof(Data));

        String dataJson = "";

        dataJson += "{\"type\":\"";
        dataJson += "D";
        dataJson += "\",\"key\":\"";
        dataJson += receivedData.key;
        dataJson += "\",\"rms\":[";
        dataJson += receivedData.rms[0];
        dataJson += ",";
        dataJson += receivedData.rms[1];
        dataJson += ",";
        dataJson += receivedData.rms[2];
        dataJson += "],\"temperature\":";
        dataJson += receivedData.temperature;
        dataJson += "}";

        httpRequest(dataJson);
        break;
        }
        case 'V': {
            VibrationPackage receivedVibration;
            memcpy(&receivedVibration, packet, sizeof(VibrationPackage));
            
        String vibrationJson = "";

        vibrationJson += "{\"type\":\"";
        vibrationJson += "P";
        vibrationJson += "\",\"key\":\"";
        vibrationJson += receivedVibration.key;
        vibrationJson += "\",\"dataPackage\":[";

        for (int i = 0; i < TRANSMISSION_DATA_PACKAGE; i++) {
          vibrationJson += receivedVibration.dataPackage[i];
          if (i < TRANSMISSION_DATA_PACKAGE - 1) {
            vibrationJson += ",";
          }
        }

        vibrationJson += "],\"start\":";
        vibrationJson += receivedVibration.start;
        vibrationJson += ",\"end\":";
        vibrationJson += (receivedVibration.start + TRANSMISSION_DATA_PACKAGE);
        vibrationJson += "}";

        httpRequest(vibrationJson);
        break;
        }
        default:
          sendSerialCommand("NX"); 
        break;
    }
}

String receivedMessage;
void readSerial() {
while (SerialAT.available() > 0) {
  if (requestState == 1) {
      
      byte packet[50];
      SerialAT.readBytes((uint8_t*)&packet,sizeof(packet)); 
      processReceivedData(packet);

    } else if (requestState == 0) {
      char receivedChar = SerialAT.read();
      if (receivedChar == '\n' || receivedChar == '\r') {
        if (receivedMessage.length() == 0) continue;

        if (receivedMessage.equals("RD")) { 
          //SerialDebug.print("Processing RD message");
          sendSerialCommand("NX");
          waitingForNX = false;
          requestState = 1;
        } else { 
          //SerialDebug.print("Processing ANY message");
          delay(500); 
          sendSerialCommand("RD");
        }

        receivedMessage = ""; 
      } else {
        receivedMessage += receivedChar;
      }
    }

    }
}

const unsigned long interval = 3600000;
void loop() {

  if (g == 0){
    initGSM();
    sendSerialCommand("RD");
    wait = false;
    requestState = 0;
  }    

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    resetFunc();

    previousMillis = currentMillis;
  }

  if(waitingForNX){
    requestState =0;
    sendSerialCommand("RD");
    blinkLed(LED_FAILURE);
    delay(5000);
  }

  if (!wait) {
    readSerial();
  }
}