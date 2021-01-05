#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define dacPin1 25

bool deviceConnected = false;
uint8_t anchorPing = 0;
/* Define the UUID for our Custom Service */


BLECharacteristic dacCharacteristic(
  BLEUUID(CHARACTERISTIC_UUID), 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_WRITE
);

//Server Callbacks handling

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class charCallbacks: 
  public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("\r\nNew value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println();
      }
    }
};


void setup() {
  Serial.begin(9600);
  pinMode(dacPin1, OUTPUT);


  BLEDevice::init("Tag_ESP32");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //I still don't know what the callback is exactly
  dacCharacteristic.setCallbacks(new charCallbacks());
  dacCharacteristic.setValue("0");
  pService->addCharacteristic(&dacCharacteristic);
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a Client to connect...");
}

void loop() {
  anchorPing = *(dacCharacteristic.getData());
  if(deviceConnected == true) {
    Serial.println(anchorPing);
    // BLECharacteristics setValue() i haven't figured it out how to set to 0, but setting value to "0" gives me the value 48 (it's ascii)
    if(anchorPing != 48 ) {

      dacWrite(dacPin1, 255);

    } else {

      dacWrite(dacPin1, 0);

    }


  } else dacWrite(dacPin1,0);
delay(50);
}