#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
// define client-server UUIDs
#define SERVICE_UUID        "b09dc577-bc1b-4d86-963b-e6d6b9fdacdb"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define ADVERTISE_UUID      "87e692ad-d2eb-4ef2-972d-f624f0ab7847"
#define ID_UUID             "c94b85d1-bff6-4faa-88f2-4647326b576e"
#define dacPin1 25
//Variable init
bool deviceConnected = false;
uint8_t anchorPing = 0;

BLEServer *holdServer;

/**
 * doAdvCharacteristic is Anchor (BLEClient) side controlled
 * Only the anchor should be able to tell the device to stop advertising incase device loses connection
 * 
 * 
 */

//our characteristics
BLECharacteristic dacCharacteristic(
  BLEUUID(CHARACTERISTIC_UUID), 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_WRITE
);
BLECharacteristic IDCharacteristic(
  BLEUUID(ID_UUID), 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_WRITE 
);

BLECharacteristic doAdvCharacteristic(
  BLEUUID(ADVERTISE_UUID), 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_WRITE
);

//Server Callbacks handling

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      doAdvCharacteristic.setValue("1");

    }
};
//Part of the initialized code, not sure what this is for
class charCallbacks: 
  public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      uint8_t dumnum;
      dumnum = *(dacCharacteristic.getData());
      if(deviceConnected == true) {
    // BLECharacteristics setValue() i haven't figured it out how to set to 0, but setting value to "0" gives me the value 48 (it's ascii) (init at line 60)
        if(dumnum == 49 ){
          dacWrite(dacPin1, 255);
          delay(3000);
          dacWrite(dacPin1, 0);
          pCharacteristic->setValue("0");

          }
      }

      
      if (value.length() > 0) {
        Serial.println("\r\nChar value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println("\n");
      }
    }
};

class doAdvCallbacks: 
  public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.println("\r\ndoAdv value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println("\n");
      }
    }
};

class IDCallbacks: 
  public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.println("\r\ndoAdv value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println("\n");
      }
    }
};


void setup() {
  Serial.begin(9600);
  pinMode(dacPin1, OUTPUT);

  BLEDevice::init("Tag_ESP32");
  BLEServer *pServer = BLEDevice::createServer();
  holdServer = pServer;
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //I still don't know what the callback is exactly
  dacCharacteristic.setCallbacks(new charCallbacks());
  IDCharacteristic.setCallbacks(new IDCallbacks());
  doAdvCharacteristic.setCallbacks(new doAdvCallbacks());

  dacCharacteristic.setValue("0");
  doAdvCharacteristic.setValue("1");
  IDCharacteristic.setValue("1");

  pService->addCharacteristic(&dacCharacteristic);
  pService->addCharacteristic(&doAdvCharacteristic);
  pService->addCharacteristic(&IDCharacteristic);

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a Client to connect...");
}

void loop() {

  if(doAdvCharacteristic.getValue())


 delay(1000);

}
