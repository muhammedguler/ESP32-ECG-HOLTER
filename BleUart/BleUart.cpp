#include "BleUart.h"

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false, oldDeviceConnected = false, bleDataReaded;
uint8_t txArray0[9 * bufferSize] = { 0 };
uint8_t txArray1[9 * bufferSize] = { 0 };

uint32_t chipId, random1;

union {
    uint32_t intArray[2];
    uint8_t ByteArray[8];  //3F9D70A4; dataArray[0] = A4; dataArray[1] = 70; dataArray[2] =9D, ... ,
} myUnion;

SemaphoreHandle_t SemBLE;
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void BleUartTasksBegin(void) {
    for (int i = 0; i < 17; i = i + 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    // Create the BLE Device
    BLEDevice::init("Hamle EKG " + String(chipId, HEX));
    BLEDevice::setMTU(517);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create the BLE Service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pRxCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();

    SemBLE = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        BleUartTasks, "BleUartTasks",
        4096 * 4,
        NULL, 4,
        NULL, ARDUINO_RUNNING_CORE);
    //Serial.println("Waiting a client connection to notify...");
}

void BleUartTasks(void* parameters) {
    while (true) {
        //taskYIELD();
        if ((deviceConnected)) {
            if (recordStatus) {
                if (bleDataReady) {
                    bleDataReady = false;
                    if (sendBuffer == 0) {
                        pTxCharacteristic->setValue((uint8_t*)txArray0, sizeof(txArray0));
                        //else
                        //    pTxCharacteristic->setValue(txArray2);
                        pTxCharacteristic->notify();
                    } else {
                        pTxCharacteristic->setValue((uint8_t*)txArray1, sizeof(txArray1));
                        //else
                        //    pTxCharacteristic->setValue(txArray2);
                        pTxCharacteristic->notify();
                    }
                    if ((millis() - buttonPressTime[1]) > 2000)
                        Button2Pressed = false;
                    //if (BufferSelect)
                    //delay(3);  // bluetooth stack will go into congestion, if too many packets are sent
                }
            } else {
                txArray0[0] = (chipId >> 24) & 0xFF;
                txArray0[1] = (chipId >> 16) & 0xFF;
                txArray0[2] = (chipId >> 8) & 0xFF;
                txArray0[3] = (chipId)&0xFF;
                txArray0[4] = (BatteryVoltage >> 8) & 0xFF;
                txArray0[5] = BatteryVoltage & 0xFF;
                txArray0[6] = BatteryPercentage;
                txArray0[7] = CardTemperature;
                txArray0[8] = Button2Pressed;
                txArray0[9] = ChgStatRead << 1 | ChgInokRead;
                txArray0[10] = 0x03;
                txArray0[11] = 0x20;
                txArray0[12] = now.year() - 2000;
                txArray0[13] = now.month();
                txArray0[14] = now.day();
                txArray0[15] = now.hour();
                txArray0[16] = now.minute();
                txArray0[17] = now.second();
                pTxCharacteristic->setValue((uint8_t*)txArray0, 18);
                //else
                //    pTxCharacteristic->setValue(txArray2);
                pTxCharacteristic->notify();
            }
        }
        // disconnecting
        if (!deviceConnected && oldDeviceConnected) {

            delay(500);                   // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising();  // restart advertising
            //Serial.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }
        // connecting
        if (deviceConnected && !oldDeviceConnected) {
            // do stuff here on connecting
            oldDeviceConnected = deviceConnected;
        }
        xSemaphoreTake(SemBLE, portMAX_DELAY);
    }
}

void ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemBLE, &xHigherPriorityTaskWoken);
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemBLE, &xHigherPriorityTaskWoken);
}

void bleSetDataReady() {
    if (deviceConnected) {}
    bleDataReady = true;
    xSemaphoreGive(SemBLE);
}

void CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
        //Serial.println("*********");
        //Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
            //Serial.print(rxValue[i]);
            inputBuffer[i] = rxValue[i];
        }
        //Serial.println();
        //Serial.println("*********");
        bleDataReaded = true;
    }
}
