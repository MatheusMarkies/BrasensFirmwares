#include <Vector.h>

#include <FRAM.h>

#include <SoftwareSerial.h>
#include "RF24.h"

#include <FRAM_MB85RC_I2C.h>
FRAM_MB85RC_I2C memory;

RF24 radio(7, 8);
const byte radio_address[6] = "00006";

#define UART_BAUD 115200
#define PIN_TX 3
#define PIN_RX 2

SoftwareSerial SerialAT(PIN_RX, PIN_TX);

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

DataI2C_t writedata;
DataI2C_t readdata;

VibrationPackageI2C_t writevibdata;
VibrationPackageI2C_t readvibdata;

#define SERIAL_DEBUG 0

int objectsCount = 0;

int storage_array[5];
typedef Vector<int> Elements;
Elements dataIndices;

void (*resetFunc)(void) = 0;

void sendSerialCommand(String cmd){
    SerialAT.println(cmd);
    //delay(10);

    Serial.print("");
    Serial.print("Enviado: ");
    Serial.println(cmd);
}

char getNextHTTPSendItem() {
    if(dataIndices.size() > 0)
      if(dataIndices[0] == 0)
        return 'D';
    return 'V';
}

void removeFirstItem() {
    if (objectsCount > 0) {
        objectsCount--;
        
        //Serial.println();
        //Serial.println("-----------------------------");
        //Serial.println("Removing!");
        int size = dataIndices.size();
        int temp[size];

        //Serial.println("dataIndices");
        //for(int i : dataIndices)
        //Serial.println(i);

        for (int i =0; i < size;i++) {
            int index = dataIndices[i] -1;
            temp[i] = index;
            //Serial.println(index);
        }

        dataIndices.clear();
        //Serial.println("dataIndices 2");
        //for(int i : dataIndices)
        //Serial.println(i);

        for (int i =0; i < size;i++) {
            if(temp[i] >= 0)
              dataIndices.push_back(temp[i]);
        }
        //Serial.println("dataIndices 3");
        //for(int i : dataIndices)
        //Serial.println(i);

        //Serial.println("-----------------------------");
        //Serial.println();
    }
}

bool hasFRAM = false;

void setup() {
  SerialAT.begin(9600);
  Serial.begin(UART_BAUD);

  Wire.begin();

  if (!radio.begin()) {
    Serial.println("Radio Error");
    resetFunc();
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setCRCLength(RF24_CRC_16);
  radio.openReadingPipe(1, radio_address);
  radio.startListening();

  memory.begin();

  Serial.println("RECIVER READY!");

  dataIndices.setStorage(storage_array);
}

uint16_t sendNextStructInFRAM(char next){
  uint16_t address = 0;

    if(next == 'D'){
        byte result = memory.readArray(address, sizeof(Data), readdata.I2CPacket);
        //if (result == 0) Serial.println("Reading data...");
        if (result != 0) Serial.println("Read failed");
        //Serial.println("...... ...... ......");

        address = sizeof(Data);

        //Serial.print("readded data: ");
        //Serial.println(readdata.datastruct.temperature);

        String dataJson = "";

        dataJson += "{\"type\":\"";
        dataJson += readdata.datastruct.type;
        dataJson += "\",\"key\":\"";
        dataJson += readdata.datastruct.key;
        dataJson += "\",\"rms\":[";
        dataJson += readdata.datastruct.rms[0];
        dataJson += ",";
        dataJson += readdata.datastruct.rms[1];
        dataJson += ",";
        dataJson += readdata.datastruct.rms[2];
        dataJson += "],\"temperature\":";
        dataJson += readdata.datastruct.temperature;
        dataJson += "}";

        //sendSerialCommand(dataJson);
        SerialAT.write((uint8_t*)&readdata.datastruct, sizeof(Data));
    }else{
        byte result = memory.readArray(address, sizeof(VibrationPackage), readvibdata.I2CPacket);
        //if (result == 0) Serial.println("Reading vp...");
        if (result != 0) Serial.println("Read failed");
        //Serial.println("...... ...... ......");

        address = sizeof(VibrationPackage);

        //Serial.print("readded vp: ");
        //Serial.println(readvibdata.datastruct.start);

        String vibrationJson = "";

        vibrationJson += "{\"type\":\"";
        vibrationJson += readvibdata.datastruct.type;
        vibrationJson += "\",\"key\":\"";
        vibrationJson += readvibdata.datastruct.key;
        vibrationJson += "\",\"dataPackage\":[";

        for (int i = 0; i < TRANSMISSION_DATA_PACKAGE; i++) {
          vibrationJson += readvibdata.datastruct.dataPackage[i];
          if (i < TRANSMISSION_DATA_PACKAGE - 1) {
            vibrationJson += ",";
          }
        }

        vibrationJson += "],\"start\":";
        vibrationJson += readvibdata.datastruct.start;
        vibrationJson += ",\"end\":";
        vibrationJson += (readvibdata.datastruct.start + TRANSMISSION_DATA_PACKAGE);
        vibrationJson += "}";

        //sendSerialCommand(vibrationJson);
        SerialAT.write((uint8_t*)&readvibdata.datastruct, sizeof(VibrationPackage));
    }
    return address;
}

uint16_t calculateFRAMAddress() {
    uint16_t nextAddress = 0;
    int o = 0;

    for (int i = 0; i < objectsCount; i++) {
        if (o < dataIndices.size() && i == dataIndices[o]) {
            nextAddress += sizeof(Data);
            o++;
        } else {
            nextAddress += sizeof(VibrationPackage);
        }
    }

    return nextAddress;
}

uint16_t reorganizeStartAddress = 0;
void reorganizeFRAM(){
  uint16_t nextAddress = reorganizeStartAddress;
  uint16_t oldAddress = 0;

    int o = 0;

    for(int i =0; i< objectsCount; i++){
        if (o < dataIndices.size() && i == dataIndices[o]) {
          
          byte rresult = memory.readArray(nextAddress, sizeof(Data), readdata.I2CPacket);
          delay(2);
          byte wresult = memory.writeArray(oldAddress, sizeof(Data), readdata.I2CPacket);

          nextAddress += sizeof(Data);
          oldAddress += sizeof(Data);
          o++;
        } else {

          byte rresult = memory.readArray(nextAddress, sizeof(VibrationPackage), readvibdata.I2CPacket);
          delay(2);
          byte wresult = memory.writeArray(oldAddress, sizeof(VibrationPackage), readvibdata.I2CPacket);
          
          nextAddress += sizeof(VibrationPackage);
          oldAddress += sizeof(VibrationPackage);
      }
    }
}

String receivedMessage;
void readSerial() {
  while (SerialAT.available() > 0) {
    char receivedChar = SerialAT.read();
    if (receivedChar == '\n' || receivedChar == '\r') {
      if (receivedMessage.length() == 0) continue;

      Serial.print("Recebido: ");
      Serial.println(receivedMessage);

      if (receivedMessage.equals("OK")) {
        //Serial.println("Processing OK message");

        removeFirstItem();
        reorganizeFRAM();

        if(objectsCount > 0)
          sendSerialCommand("RD");

      } else if (receivedMessage.equals("NX")) {
        //Serial.println("Processing NX message");
        
        reorganizeStartAddress = sendNextStructInFRAM(getNextHTTPSendItem());
      } else if (receivedMessage.equals("RD")) {
        //Serial.println("Processing RD message");
        sendSerialCommand("RD");
      } else {

      }

      Serial.println("");
      receivedMessage = "";

    } else {
      receivedMessage += receivedChar;
    }
  }

  delay(10);
}

void addObjectInCounter(bool isDataPackage) {
    objectsCount++;
    if (isDataPackage) {
        dataIndices.push_back(objectsCount - 1);
    }
}

void loop() {
  uint8_t pipe;
    if (radio.available(&pipe)) {
      int size = radio.getDynamicPayloadSize();

      byte packet[size];
      radio.read(&packet, size);

      if (packet[0] == 'D') {
        uint16_t nextAddr = calculateFRAMAddress();
        memcpy(&data, &packet, sizeof(data));
        addObjectInCounter(true);

        writedata.datastruct.type = 'D';
        strcpy(writedata.datastruct.key, data.key);

        for (int i = 0; i < 3; i++) {'1'
          writedata.datastruct.rms[i] = data.rms[i];
        }

        writedata.datastruct.temperature = data.temperature;

        //Serial.print("vibdata: ");
        //Serial.println(data.temperature);
        //Serial.print("writedata: ");
        //Serial.println(writedata.datastruct.temperature);

        byte result = memory.writeArray(nextAddr, sizeof(Data), writedata.I2CPacket);

        //if (result == 0){ Serial.println("Write Done - array loaded in FRAM chip"); }
        if (result != 0) Serial.println("Write failed");
	      //Serial.println("...... ...... ......");
      } else {
        uint16_t nextAddr = calculateFRAMAddress();
        memcpy(&vibrationPackage, &packet, sizeof(vibrationPackage));
        addObjectInCounter(false);

        writevibdata.datastruct.type = 'V';
        strcpy(writevibdata.datastruct.key, vibrationPackage.key);

        for (int i = 0; i < TRANSMISSION_DATA_PACKAGE; i++) {
          writevibdata.datastruct.dataPackage[i] = vibrationPackage.dataPackage[i]; // Inicializa com 0.0 ou qualquer valor desejado
        }
        writevibdata.datastruct.start = vibrationPackage.start;
        writevibdata.datastruct.end = vibrationPackage.end;

        //Serial.print("vibdata: ");
        //Serial.println(vibrationPackage.start);
        //Serial.print("writevibdata: ");
        //Serial.println(writevibdata.datastruct.start);

        byte result = memory.writeArray(nextAddr, sizeof(vibrationPackage), writevibdata.I2CPacket);

        //if (result == 0){ Serial.println("Write Done - array loaded in FRAM chip"); }
        if (result != 0) Serial.println("Write failed");
	      //Serial.println("...... ...... ......");

        //TEST
        //reorganizeStartAddress = sendNextStructInFRAM(getNextHTTPSendItem());
        //removeFirstItem();
        //reorganizeFRAM();
      }
    }

  if(objectsCount > 0){
    readSerial();
  }
}