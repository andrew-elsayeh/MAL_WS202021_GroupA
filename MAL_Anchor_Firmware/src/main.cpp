#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>

static BLEUUID serviceUUID("7cddf5af-453f-40fa-808e-37ae6ad8facd"); //Interested Device or Service
static BLEUUID charUUID("3383a686-a53c-42e4-a88c-e0e1de4c4bda"); //Interested Characterstics

static BLEAddress pServerAddress;
static boolean doConnect = false;
static boolean doScan = false;
static boolean connected = false;
static BLERemoteCharacteristic pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
BLEScan *pBLEScan; //Name the scanning device as pBLEScan

static void notifyCallback(BLERemoteCharacteristic pBLERemoteCharacteristic, uint8_t pData, size_t length, bool isNotify) {
/Serial.print("Notify callback for characteristic ");
Serial.println(pData[0]);
for (int i = 0; i < length; i++) {
Serial.print(pData[i]);
Serial.print(" ");
}
Serial.println();/
}

class MyClientCallback : public BLEClientCallbacks {
void onConnect(BLEClient* pclient) {
}
void onDisconnect(BLEClient* pclient) {
connected = false;
Serial.println("onDisconnect");
}
};

bool connectToServer() {
Serial.print("Forming a connection to ");
Serial.println(myDevice->getAddress().toString().c_str());

 BLEClient *pClient = BLEDevice::createClient();
 Serial.println("Client created");

 // Connect to the remove BLE Server.
 pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
 Serial.println("Connected to server");
 
 // Obtain a reference to the service we are after in the remote BLE server.
 BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
 if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
   }
 Serial.println("Found our service");

 // Obtain a reference to the characteristic in the service of the remote BLE server.
 pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
 if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    return false;
   }
 Serial.println("Found our characteristic");

 pRemoteCharacteristic->registerForNotify(notifyCallback);
  
 connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
void onResult(BLEAdvertisedDevice advertisedDevice) {
Serial.print("BLE Advertised Device found: ");
Serial.println(advertisedDevice.toString().c_str());

     if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
         Serial.print("Found our device!!!  Address: ");
         BLEDevice::getScan()->stop();
         myDevice = new BLEAdvertisedDevice(advertisedDevice);
         doConnect = true;
         doScan = true;
     }
 }
};

void setup() {
Serial.begin(115200);
Serial.println("Starting Arduino BLE Client application...Scanning...");
BLEDevice::init("");

pBLEScan = BLEDevice::getScan(); //create new scan
pBLEScan-> setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
pBLEScan-> setActiveScan(true); //active scan uses more power, but get results faster
pBLEScan-> start(10);
}

void loop() {
if (doConnect == true) {
if (connectToServer()) {
Serial.println("We are now connected to the BLE Server.");
connected = true;
}
else {
Serial.println("We have failed to connect to the server; there is nothin more we will do.");
}
doConnect = false;
}

if (connected) {
  std::string newValue = pRemoteCharacteristic->readValue(); 
  String totalString = newValue.c_str();
  String temper = totalString.substring(0,5);
  String humid = totalString.substring(6,11);
  Serial.print(temper); 
  Serial.print(" | ");
  Serial.println(humid);
}
else if(doScan){
  BLEDevice::getScan()->start(0);
}
delay(1000);
}