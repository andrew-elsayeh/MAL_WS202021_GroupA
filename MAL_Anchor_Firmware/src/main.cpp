/**
 * The example checks for servuiceUUID on server during connecting and scanning..why?
 * Try removing nConnectedTags and replace arguments with counter, see if it's robust
 * TODO: write to doAdvertiseUUID characteristic on tag to "0" 48 ascii
 */

#include "BLEDevice.h"
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
#define SCAN_TIME 10

int nConnectedTags = 0;
int counter=0;

static boolean doConnect = false;
static boolean allConnected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static BLEAdvertisedDevice* connectedDevice[nTags];
static BLERemoteCharacteristic* pRemoteTagIDCharacteristic;
static BLERemoteCharacteristic* pRemoteTagdoAdvCharacteristic;

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
    Serial.println("onConnect");
  }

  void onDisconnect(BLEClient* pclient) {
    nConnectedTags +=-1;
    Serial.println("onDisconnect");
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

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    pRemoteTagIDCharacteristic = pRemoteService->getCharacteristic(charIDUUID);
    pRemoteTagdoAdvCharacteristic = pRemoteService->getCharacteristic(doAdvUUID);
    connectedDevice[nConnectedTags] = myDevice;

    if (pRemoteCharacteristic == nullptr) {
      M5.Lcd.println("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    M5.Lcd.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.println("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    // ???

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    //Once here assumption is connection is formed
    connectedDevice[counter] = myDevice;
    counter += 1;
    return true;

}

bool isAllConnected() {
  if (connectToServer()) {
    //would this work if instead nConnectedTags we write counter == nTags
    if (nConnectedTags == nTags) {
      doConnect = false;
      M5.Lcd.println("All connected");
      return true;
    }
  }
  doConnect = true;
  delay(200);
  return isAllConnected();

}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */

  void onResult(BLEAdvertisedDevice advertisingDevice) {
    Serial.println("BLE Advertising Device found: ");
    Serial.println(advertisingDevice.toString().c_str());
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisingDevice.haveServiceUUID() && advertisingDevice.isAdvertisingService(serviceUUID) && nConnectedTags < nTags) {
      BLEDevice::getScan()->stop();
      //TODO:Tell the tag to stop advertising
      myDevice = new BLEAdvertisedDevice(advertisingDevice);
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {

  M5.begin();
  Serial.begin(9600);
  Serial.println("Initializing...");
  BLEDevice::init("ANCHOR");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for SCAN_TIME seconds (defined above)

  doConnect = true;
  BLEScan* pBLEScan = BLEDevice::getScan();
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

  // Whenever the button is pressed a value of "1" is written to the characteristic to be read by the server to blink.

  if (M5.BtnA.isPressed()) {
    pRemoteCharacteristic->writeValue("1");
    M5.Lcd.println("Button is pressed");
  } 

  if (doConnect == true) {
    //calls and waits until true
      Serial.println("Connecting to tags..");
    if (isAllConnected()) {
      Serial.println("We are now connected to all BLE Tags");
    } 
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.

  if (allConnected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    Serial.println("Starting scan \"");
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(100); // Delay a second between loops.

} // End of loop