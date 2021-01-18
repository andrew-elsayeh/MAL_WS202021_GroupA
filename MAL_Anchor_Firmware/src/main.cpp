/**
 * The example checks for servuiceUUID on server during connecting and scanning..why?
 * Try removing nConnectedTags and replace arguments with counter, see if it's robust
 * TODO: write to doAdvertiseUUID characteristic on tag to "0" 48 ascii
 */

#include <BLEDevice.h>
#include "BLEScan.h"
#include <Arduino.h>
#include <M5StickC.h>

// The remote service we wish to connect to. THE TAGS HAVE TO HAVE THIS ID
static BLEUUID serviceUUID("b09dc577-bc1b-4d86-963b-e6d6b9fdacdb");

// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID charIDUUID("722163a8-8aa7-4d93-b502-100777de4619");
static BLEUUID doAdvUUID("87e692ad-d2eb-4ef2-972d-f624f0ab7847");

#define nTags 1
#define SCAN_TIME 0

bool isRoutineRunning=false;
int nConnectedTags = 0;
int counter=0;

static boolean allConnected = false;
static boolean isScanning = false;
static BLERemoteCharacteristic* pRemoteCharacteristic[nTags];
static BLEAdvertisedDevice* myDevice;
static BLEAdvertisedDevice* connectedDevice[nTags]; // do we even need this
static BLERemoteCharacteristic* pRemoteTagIDCharacteristic[nTags];
static BLERemoteCharacteristic* pRemoteTagdoAdvCharacteristic[nTags];

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
class MyClientCallback : public BLEClientCallbacks {

  void onConnect(BLEClient* pclient) {
    nConnectedTags +=1;
    Serial.println("PING");
  }

  void onDisconnect(BLEClient* pclient) {
    nConnectedTags +=-1;
    counter += -1; 
    Serial.println("onDisconnect, counter is now : ");
    Serial.println(counter);
    allConnected = false;


  }
};

// server = TAG


bool connectToServer() {

    M5.Lcd.println("Forming a connection to ");
    M5.Lcd.println(myDevice->getAddress().toString().c_str());
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    M5.Lcd.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);

    if (pRemoteService == nullptr) {
      M5.Lcd.println("Failed to find our service UUID on device: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    Serial.println(" - Found our service");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    Serial.println(counter);
    pRemoteCharacteristic[counter] = pRemoteService->getCharacteristic(charUUID);
    pRemoteTagIDCharacteristic[counter] = pRemoteService->getCharacteristic(charIDUUID);
    pRemoteTagdoAdvCharacteristic[counter] = pRemoteService->getCharacteristic(doAdvUUID);
    connectedDevice[nConnectedTags] = myDevice;

    if (pRemoteCharacteristic[counter] == nullptr) {
      M5.Lcd.println("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    M5.Lcd.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic[counter]->canRead()) {
      std::string value = pRemoteCharacteristic[counter]->readValue();
      Serial.println("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    // ???

    if(pRemoteCharacteristic[counter]->canNotify())
      pRemoteCharacteristic[counter]->registerForNotify(notifyCallback);
    //Once here assumption is connection is formed
    pRemoteTagdoAdvCharacteristic[counter]->writeValue("0");
    connectedDevice[counter] = myDevice;
    M5.Lcd.println(counter);
    return true;
    

}

void startConnectRoutine(){
    if(isRoutineRunning == false){
        isRoutineRunning = true; 
        if(connectToServer()){
          //returns a true upon an established connection
          counter += 1;
          if(nConnectedTags==nTags){
            isRoutineRunning=false;
            return;
          }
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
    if (advertisingDevice.haveServiceUUID() && advertisingDevice.isAdvertisingService(serviceUUID) && nConnectedTags < nTags) {
      //TODO:Tell the tag to stop advertising
      myDevice = new BLEAdvertisedDevice(advertisingDevice);


      
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void startScan(){
  if(isScanning==false) { 
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(SCAN_TIME, false);
  isScanning=true;
  }
}
void stopScan(){
  if(isScanning==true) { 
      BLEDevice::getScan()->stop();
      isScanning=false; }
}

void setup() {

  M5.begin();
  Serial.begin(9600);
  Serial.println("Initializing...");
  isScanning=true;
  BLEDevice::init("ANCHOR");
  BLEScan* pBLEScan = BLEDevice::getScan();
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for SCAN_TIME seconds (defined above)

  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  //callback for advertising devices around.

  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(SCAN_TIME, false);
  

} // End of setup.
// This is the Arduino main loop function.

void loop() {

  /* Routine description
      1. Stay connected with all 3 tags
      2. onDisconnect do Anchor rescan
      3. when nConnectedTags == nTags stop scan stop advertise
      4. For tags on disconnect -> start advertise (anchor side controlled)
  */

  // If the flag "isConnected" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.

  M5.update();
  Serial.println(nConnectedTags);
  // Whenever the button is pressed a value of "1" is written to the characteristic to be read by the server to blink.

  if (M5.BtnA.isPressed()) {
    pRemoteCharacteristic[0]->writeValue("1");
    M5.Lcd.println("Button is pressed");
  } 


  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.

  if (allConnected) {

    String newValue = "Time since boot: " + String(millis()/1000);
    // Set the characteristic's value to be the array of bytes that is actually a string.  
  } else {
        if (nConnectedTags == nTags) {
          allConnected=true;
          isRoutineRunning=false;
          M5.Lcd.println("All connected");
          Serial.println("All connected");
          stopScan();
       } else {
          
          if(isRoutineRunning==false){
            startScan();
            startConnectRoutine();
      
          }
       }
    }

  delay(200); // Delay a 100ms between loops.

} // End of loop
