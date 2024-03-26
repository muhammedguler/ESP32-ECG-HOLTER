#ifndef BleUart
#define BleUart

#include "defines.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "ECG.h"
#include "CardControl.h"
#include "esp_random.h"



extern uint32_t chipId;
extern bool oldDeviceConnected, deviceConnected, bleDataReaded;
extern uint8_t txArray0[];
extern uint8_t txArray1[];

void BleUartTasksBegin(void);
void BleUartTasks(void* parameters);
void bleSetDataReady();
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
