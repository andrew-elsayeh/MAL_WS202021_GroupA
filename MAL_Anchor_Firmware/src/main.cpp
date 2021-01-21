/**
 * The example checks for servuiceUUID on server during connecting and scanning..why?
 * Try removing nConnectedTags and replace arguments with counter, see if it's robust
 * TODO: write to doAdvertiseUUID characteristic on tag to "0" 48 ascii
 */

#include <BLEDevice.h>
#include "BLEScan.h"
#include <Arduino.h>
#include <M5StickC.h>
#include <stdlib.h>

#define LOOP_INTERVAL 200

#define nTags 3 //your number of tags here
#define SCAN_TIME_FIRST 7
#define SCAN_TIME_ROUTINE 3

// The remote service we wish to connect to. THE TAGS HAVE TO HAVE THIS ID
static BLEUUID serviceUUID[nTags] = {

  BLEUUID("51a6e40a-88df-444b-ad03-3fc40a0e035b"),
  BLEUUID("073623b4-ad83-4e72-a699-8be79fe42612"),
  BLEUUID("2eeebadf-8c43-4c2e-9246-1b9a45222401")

};

int nTick=0;
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID TagIDUUID("722163a8-8aa7-4d93-b502-100777de4619"); //obsolete in this ver
static BLEUUID doAdvUUID("87e692ad-d2eb-4ef2-972d-f624f0ab7847");
std::string stringAddress = "000000";
BLEAddress asd(stringAddress);

BLEAddress storedAddress[nTags] ={asd,asd,asd};

int nValidDevices=0;
bool isRoutineRunning=false;
int nConnectedTags = 0;
int counter=0;
int storedID[nTags];
bool isThisTagConnected[nTags];
bool isThisTagFound[nTags];

static boolean allConnected = false;
static boolean isScanning = false;
static BLERemoteCharacteristic* pRemoteCharacteristic[nTags];
BLEAdvertisedDevice* foundDevices[nTags];
BLEClient* pClientHandler[nTags];
BLEAdvertisedDevice* connectedDevice[nTags]; // do we even need this
static BLERemoteCharacteristic* pRemoteTagIDCharacteristic[nTags];
static BLERemoteCharacteristic* pRemoteTagdoAdvCharacteristic[nTags];
int WhichToPing=0;

int AddressToID(BLEAddress address){
  for(int i=0;i<nTags;i++){
    if(address.equals(storedAddress[i])){
      return i;
    }
  }
  return -1;
}


static void notifyCallback(

  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,

  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);

}

    //keeptrack of how many tags are connected
class CounterCallback : public BLEClientCallbacks {

  void onConnect(BLEClient* pclient) {
    nConnectedTags +=1;
    Serial.println("onConnect");
  }

  void onDisconnect(BLEClient* pclient) {
    nConnectedTags +=-1;
    counter += -1;
    int getId= AddressToID(pclient->getPeerAddress());
    isThisTagFound[getId] = false;
    isThisTagConnected[getId] = false;
    Serial.printf("\nonDisconnect, counter is now : %d (ID %d)",counter,getId);
    allConnected = false;

  }
};


class dummyCallback : public BLEClientCallbacks {

  void onConnect(BLEClient* pclient) {
    Serial.println("onConnectD");
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("onDisconnectD, ");

  }
};
// server = TAG


bool connectToServer(BLEAdvertisedDevice* myDevice,BLEClient* pClient, int internal) {
    
    Serial.printf("Forming a connection to %d ",internal);
    Serial.println(myDevice->getAddress().toString().c_str());
        
    Serial.println(serviceUUID[internal].toString().c_str());

    // Connect to the remote BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    M5.Lcd.println(" - Connected to server");
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID[internal]);
    

    if (pRemoteService == nullptr) {
      M5.Lcd.println("Failed to find our service UUID on device: ");
      Serial.println(serviceUUID[internal].toString().c_str());
      pClient->disconnect();
      return false;
    }

    if(isThisTagConnected[internal]==true){
      return false;
    }
    storedAddress[internal] = (myDevice->getAddress());
    int TagID = atoi(pRemoteService->getCharacteristic(TagIDUUID)->readValue().c_str());
    storedID[internal] = TagID;

    isThisTagConnected[internal] = true;
    // Obtain a reference to the service we are after in the remote BLE server.
   

    
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    Serial.printf("\n Found Tag was number : %d\n",TagID);

    //store the variables
    pRemoteCharacteristic[internal] = pRemoteService->getCharacteristic(charUUID);
    pRemoteTagIDCharacteristic[internal] = pRemoteService->getCharacteristic(TagIDUUID);
    pRemoteTagdoAdvCharacteristic[internal] = pRemoteService->getCharacteristic(doAdvUUID);
    connectedDevice[internal] = myDevice;

    if (pRemoteCharacteristic[internal] == nullptr) {
      M5.Lcd.println("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    M5.Lcd.println(" - Found our characteristics");
    if(pRemoteCharacteristic[internal]->canNotify()) {
      pRemoteCharacteristic[internal]->registerForNotify(notifyCallback);
    }

    pRemoteTagdoAdvCharacteristic[internal]->writeValue("0");
    isThisTagConnected[internal] = true;
    pClientHandler[internal] = pClient;
    return true;
    

}

void startConnectRoutine(){
    if(isRoutineRunning == false){

        isRoutineRunning = true; 

        if(counter>=nTags){
          isRoutineRunning = false;
          Serial.println("Connection to all devices established");
          allConnected=true;
          return;
          } else {
            for(int i=0;i<nTags;i++){
              if(isThisTagFound[i]==true && isThisTagConnected[i] ==false){
                if(connectToServer(foundDevices[i],pClientHandler[i],i)) {
                  Serial.printf("Number ID %d established",storedID[i]);
                  counter++;
                }

              }
            }
            isRoutineRunning=false;
          }

  }
}


/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */

  void onResult(BLEAdvertisedDevice advertisingDevice) {
    Serial.println("Scanned BLE Device: ");
    Serial.println(advertisingDevice.toString().c_str());
    // We have found a device, let us now see if it contains the service we are looking for.
    for(int i=0;i<nTags;i++){
      if (advertisingDevice.haveServiceUUID() && advertisingDevice.isAdvertisingService(serviceUUID[i]) ) {
        Serial.printf("Found our service : ");
        Serial.printf(serviceUUID[i].toString().c_str());
        foundDevices[i] = new BLEAdvertisedDevice(advertisingDevice);
        isThisTagFound[i] = true;
      }  
    }
  }
   // Found our server
   // onResult
}; // MyAdvertisedDeviceCallbacks

void startScan(){
  Serial.println("startScan() is called");
  if(isScanning==false && allConnected == false) { 
  isScanning=true;
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(SCAN_TIME_ROUTINE, false);

  return;
  }
}
void stopScan(){
  if(isScanning==true) { 
      BLEDevice::getScan()->stop();
      isScanning=false; }
}

void onButtonPress(int pressed){

  if(isThisTagConnected[pressed] == false || pRemoteTagIDCharacteristic[pressed] == nullptr){
    Serial.println("Device not connected / Wrong Device Connected");
    return;
  } 
  pRemoteCharacteristic[pressed]->writeValue("1");
  return;  
}

void setup() {
  for (int i=0;i<nTags;i++){
    isThisTagConnected[i] = false;
    isThisTagFound[i] = false;
    pClientHandler[i] = BLEDevice::createClient();
    pClientHandler[i]->setClientCallbacks(new CounterCallback());
    }

  M5.begin();
  Serial.begin(9600);
  Serial.println("Initializing...");
  isScanning=true;
  BLEDevice::init("Anchor");
  BLEScan* pBLEScan = BLEDevice::getScan();
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for SCAN_TIME seconds (defined above)

  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  //callback for advertising devices around.

  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(SCAN_TIME_FIRST, false);
  

} // End of setup.


void loop() {

  nTick +=1;
  M5.update();

  if (M5.BtnA.isPressed()) {
    onButtonPress(WhichToPing);
    M5.Lcd.println("Button is pressed");
  } 


  if (allConnected) {

    M5.Lcd.println("All connected");
    Serial.println("All connected");
    String newValue = "Time since boot: " + String(millis()/1000);
    if(isScanning==true){
      stopScan();
    }
    // here onwards is counter < nTags
  } else if(isRoutineRunning == false) {
          if(isScanning==false){startScan();}
          startConnectRoutine();
          if(nTick * LOOP_INTERVAL > 15 * 1000){
            nTick=0;
            stopScan();
          }
          
  }
delay(LOOP_INTERVAL);
}
