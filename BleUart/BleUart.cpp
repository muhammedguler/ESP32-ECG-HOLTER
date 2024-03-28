#include "BleUart.h"
//BLE server tanımlaması
BLEServer* pServer = NULL;
// BLE veri gönderme karakterstiğinin tanımlaması.
BLECharacteristic* pTxCharacteristic;
// bağlantı durumu bayraklarının tanımlanması
bool deviceConnected = false, oldDeviceConnected = false, bleDataReaded;
//BLE Veri gönderme bufferlerinin tanımlanması
uint8_t txArray0[9 * bufferSize] = { 0 };
uint8_t txArray1[9 * bufferSize] = { 0 };
// cihaz ID'si değişkeninin tanımlanması
uint32_t chipId;
// BLE'den gönderilmeye hazır veri olduğunu gösterir bayrak
bool bleDataReady = false;
// BLE operasyonlarını yöneten semaforun tanımlanması
SemaphoreHandle_t SemBLE;
// BLE karakteristiklerinin ID'lerinin tanımlanması
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void BleUartTasksBegin(void) {
    /*!
    * @brief     ADS1293TasksBegin() fonksiyonu BLE haberleşme işlemleri için thread ve semafor kurulumunu içerir 
    * @return    none
    */

    SemBLE = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        BleUartTasks, "BleUartTasks",
        4096 * 4,
        NULL, 5,
        NULL, ARDUINO_RUNNING_CORE);
    //Serial.println("Waiting a client connection to notify...");
}

void BleUartTasks(void* parameters) {
    /*!
    * @brief        BleUartTasks() fonksiyonu BLE işlemleri için oluşturulan thread
    * @details      Cihaz ID'sinin oluşturulması, BLE haberleşmesinin kurulmasını, 
    BLE üzerinden gelen bağlantı taleplerinin kabul edilmesini,
    kopan bağlantının sonlandırılmasını,
    EKG kaydı aktif değilse cihaz durumunun BLE üzerinden gönderilmesini,
    EKG kaydı aktif ise cihaz durumunun ve EKG verisinin BLE üzerinden gönderilmesini sağlar
    * @param[in]    parameters thread kurulumunda aktarılan parametreler
    * @return       none
    */
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

    while (true) {
        //taskYIELD();
        if (deviceConnected) {
            if (recordStatus) {
                if (bleDataReady) {
                    bleDataReady = false;
                    if (sendBuffer == 0) {
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
                        txArray0[10] = 0x01;
                        txArray0[11] = 0x90;
                        txArray0[12] = now.year() - 2000;
                        txArray0[13] = now.month();
                        txArray0[14] = now.day();
                        txArray0[15] = now.hour();
                        txArray0[16] = now.minute();
                        txArray0[17] = now.second();
                        pTxCharacteristic->setValue((uint8_t*)txArray0, sizeof(txArray0));
                        //else
                        //    pTxCharacteristic->setValue(txArray2);
                        pTxCharacteristic->notify();
                    } else {
                        txArray1[0] = (chipId >> 24) & 0xFF;
                        txArray1[1] = (chipId >> 16) & 0xFF;
                        txArray1[2] = (chipId >> 8) & 0xFF;
                        txArray1[3] = (chipId)&0xFF;
                        txArray1[4] = (BatteryVoltage >> 8) & 0xFF;
                        txArray1[5] = BatteryVoltage & 0xFF;
                        txArray1[6] = BatteryPercentage;
                        txArray1[7] = CardTemperature;
                        txArray1[8] = Button2Pressed;
                        txArray1[9] = ChgStatRead << 1 | ChgInokRead;
                        txArray1[10] = 0x02;
                        txArray1[11] = 0x15;
                        txArray1[12] = now.year() - 2000;
                        txArray1[13] = now.month();
                        txArray1[14] = now.day();
                        txArray1[15] = now.hour();
                        txArray1[16] = now.minute();
                        txArray1[17] = now.second();
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
        // BLE'yi ilgilendiren işlem yoksa thread'in uyumasını sağlar
        xSemaphoreTake(SemBLE, portMAX_DELAY);
    }
}

void ServerCallbacks::onConnect(BLEServer* pServer) {
/*!
    * @brief        onConnect() fonksiyonu BLE'den bağlantı talebi olduğunda tetiklenen fonksiyon
    * @param[in]    pServer bağlantı talebinin geldiği BLEServer'i
    * @return       none
    */
    deviceConnected = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemBLE, &xHigherPriorityTaskWoken);
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
/*!
    * @brief        onDisconnect() fonksiyonu BLE bağlantısı kesildiğinde tetiklenen fonksiyon
    * @param[in]    pServer bağlantının kesildiği BLEServer'i
    * @return       none
    */
    deviceConnected = false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemBLE, &xHigherPriorityTaskWoken);
}

void bleSetDataReady() {
/*!
    * @brief        bleSetDataReady() fonksiyonu BLE'den gönderilmeye hazır EKG verisi olduğunda tetiklenen fonksiyon.
    * @return       none
    */
    bleDataReady = true;
    xSemaphoreGive(SemBLE);
}

void CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
/*!
    * @brief        onWrite() fonksiyonu BLE bağlantısı üzerinden veri yazılmaya çalışıldığında tetiklenen fonksiyon
    * @param[in]    pCharacteristic verinin yazılmaya çalışındığı BLE karakteristiği
    * @return       none
    */
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
