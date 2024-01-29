#ifndef BleUart
#define BleUart

#include "defines.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"

extern bool DataReady;
extern bool oldDeviceConnected;
extern bool deviceConnected;
extern char txArray[256];

void BleUartTasksBegin(void);
void BleUartTasks(void* parameters);
//void SetDataReady(bool status);
//void SetDeviceConnected(bool status);
//void SetSendData(void* txData);

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer);
  void onDisconnect(BLEServer* pServer);
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic);
};


#endif