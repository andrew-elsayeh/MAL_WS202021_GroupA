#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <M5StickC.h>

// Speaker Set-up
const int servo_pin = 26;
int freq = 50;
int ledChannel = 0;
int resolution = 10;

//Server Set-up
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      M5.Lcd.println("Connected.");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) {
      // Do something before the read completes.
      M5.update();
      M5.Lcd.println("READING");
      pCharacteristic->setValue("This is the key tag.");
      pCharacteristic->notify();
    };
    
    void onWrite(BLECharacteristic* pCharacteristic) {
      // Do something because a new value is written.
      M5.update();
      M5.Lcd.println("BEEEEP!");
      ledcWriteTone(ledChannel, 1250);
      delay(200);
      ledcWrite(ledChannel, 0);
      
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        M5.Lcd.println("*********");
        M5.Lcd.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          M5.Lcd.print(rxValue[i]);
        M5.Lcd.println();
        M5.Lcd.println("*********");
      }
    }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  M5.begin();
  
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(servo_pin, ledChannel);
  ledcWrite(ledChannel, 0);
  
  // Create the BLE Device
  BLEDevice::init("KEY TAG");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Creating the Callback handler
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  M5.Lcd.println("Waiting a client connection...");
    
}

void loop() {
    M5.update();

    //This part is currently unnecessary:
    
    // notify changed value
    if (deviceConnected) {
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        M5.Lcd.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
