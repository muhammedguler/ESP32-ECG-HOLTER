#include "define.h"

ads1293 ADS1293(AdsDrdy, AdsCS);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
MCP7940_Class MCP7940;  // Create an instance of the MCP7940

uint8_t color = 0,color_1=0;          // a value from 0 to 255 representing the hue
uint32_t R, G, B;           // the Red Green and Blue color components
uint8_t brightness = 255;  // 255 is maximum brightness, but can be changed.  Might need 256 for common anode to fully turn off.
const boolean invert = true; // set true if common anode, false if common cathode


char TxBuffer[64] = { 0 };

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};


void setup() {
  
  //set the resolution to 12 bits (0-4096)
  analogReadResolution(12);

  Serial.begin(115200);

  SPI.begin(AdsSCLK, AdsSDO, AdsSDI);
    // Initialize pins as LEDC channels 
  // resolution 1-16 bits, freq limits depend on resolution
  ledcAttach(RedLED,   1200, 8); // 12 kHz PWM, 8-bit resolution
  ledcAttach(GreenLED, 1200, 8);
  ledcAttach(BlueLED,  1200, 8);

  ADS1293.setAds1293Pins();
  ADS1293.ads1293Begin5LeadECG();

  while (!MCP7940.begin(RtcSDA, RtcSCL)) {                                // Initialize RTC communications
    Serial.println(F("Unable to find MCP7940N. Checking again in 3s."));  // Show error text
    delay(3000);                                                          // wait three seconds
  }                                                                       // of loop until device is located
  Serial.println(F("MCP7940N initialized."));
  while (!MCP7940.deviceStatus()) {  // Turn oscillator on if necessary
    Serial.println(F("Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart();                       // Start oscillator and return state
    if (!deviceStatus) {                                             // If it didn't start
      Serial.println(F("Oscillator did not start, trying again."));  // Show error and
      delay(1000);                                                   // wait for a second
    }                                                                // of if-then oscillator didn't start
  }                                                                  // of while the oscillator is off
  MCP7940.adjust();                                                  // Set to library compile Date/Time
  Serial.println(F("Enabling battery backup mode"));
  MCP7940.setBattery(true);     // enable battery backup mode
  if (!MCP7940.getBattery()) {  // Check if successful
    Serial.println(F("Couldn't set Battery Backup, is this a MCP7940N?"));
  }  // if-then battery mode couldn't be set

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  color = (millis()/128)&255;
  if(color_1!=color)
  { 
    hueToRGB(color, brightness);  // call function to convert hue to RGB
    color_1 = color;
    // write the RGB values to the pins
    ledcWrite(RedLED,    R); // write red component to channel 1, etc.
    ledcWrite(GreenLED,  G);   
    ledcWrite(BlueLED,   B); 
  }
  // notify changed value
  if ((deviceConnected) /*&& (digitalRead(ADS1293.drdyPin) == false)*/) {
    int32_t ecgCh1 = ADS1293.getECGdata(1);
    int32_t ecgCh2 = ADS1293.getECGdata(2);
    int32_t ecgCh3 = ADS1293.getECGdata(3);
    DateTime  now = MCP7940.now();  // get the current time

    int BatteryVolts = 2*analogReadMilliVolts(VBat);
    sprintf(TxBuffer, "%d %d %d %d %04d-%02d-%02d %02d:%02d:%02d",
            BatteryVolts,ecgCh1, ecgCh2, ecgCh3,
            now.year(),  // Use sprintf() to pretty print
            now.month(), now.day(), now.hour(), now.minute(),
            now.second());   

    pCharacteristic->setValue(TxBuffer);
    pCharacteristic->notify();
    value++;
    delay(100);  // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}




// Courtesy http://www.instructables.com/id/How-to-Use-an-RGB-LED/?ALLSTEPS
// function to convert a color to its Red, Green, and Blue components.

void hueToRGB(uint8_t hue, uint8_t brightness)
{
    uint16_t scaledHue = (hue * 6);
    uint8_t segment = scaledHue / 256; // segment 0 to 5 around the
                                            // color wheel
    uint16_t segmentOffset =
      scaledHue - (segment * 256); // position within the segment

    uint8_t complement = 0;
    uint16_t prev = (brightness * ( 255 -  segmentOffset)) / 256;
    uint16_t next = (brightness *  segmentOffset) / 256;

    if(invert)
    {
      brightness = 255 - brightness;
      complement = 255;
      prev = 255 - prev;
      next = 255 - next;
    }

    switch(segment ) {
    case 0:      // red
        R = brightness;
        G = next;
        B = complement;
    break;
    case 1:     // yellow
        R = prev;
        G = brightness;
        B = complement;
    break;
    case 2:     // green
        R = complement;
        G = brightness;
        B = next;
    break;
    case 3:    // cyan
        R = complement;
        G = prev;
        B = brightness;
    break;
    case 4:    // blue
        R = next;
        G = complement;
        B = brightness;
    break;
   case 5:      // magenta
    default:
        R = brightness;
        G = complement;
        B = prev;
    break;
    }
}