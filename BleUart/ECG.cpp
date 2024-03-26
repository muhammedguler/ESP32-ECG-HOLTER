#include "ECG.h"
#include "fdacoefs.h"
int32_t ecgCh1 = 0;
int32_t ecgCh2 = 0;
int32_t ecgCh3 = 0;

int32_t ecgFilteredCh1 = 0;
int32_t ecgFilteredCh2 = 0;
int32_t ecgFilteredCh3 = 0;


uint32_t ch1CircleIndex = 0, ch2CircleIndex = 0, ch3CircleIndex = 0;
uint32_t ch1ReadsBuffer[BL], ch2ReadsBuffer[BL], ch3ReadsBuffer[BL];
hw_timer_t* My_timer = NULL;

ads1293 ADS1293(AdsDrdy, AdsCS);
SemaphoreHandle_t SemADS1293;
int64_t startTime, diffTime = 10;
//uint8_t CircularBuffer[9 * bufferSize];

uint16_t point = 0;
bool bufferFilled = false;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
bool AdsAlarmOccured = false;
uint8_t sendBuffer = 1, writeBuffer = 0;
bool bleDataReady = false, SdCardDataReady = false;

void ARDUINO_ISR_ATTR isrAdsALARM() {
    AdsAlarmOccured = true;
}
void ARDUINO_ISR_ATTR isrADS1293Complete() {
    xHigherPriorityTaskWoken = pdFALSE;
    //xSemaphoreGiveFromISR(SemADS1293, &xHigherPriorityTaskWoken);
}
void IRAM_ATTR onTimer() {
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemADS1293, &xHigherPriorityTaskWoken);
}
void ADS1293TasksBegin(void) {
    SemADS1293 = xSemaphoreCreateBinary();
    //xSemaphoreGive(SemADS1293);
    xTaskCreatePinnedToCore(
        ADS1293Tasks, "ADS1293Tasks",
        4096 * 4,
        NULL, 7,
        NULL, ARDUINO_RUNNING_CORE);
}
void ADS1293Tasks(void* Parameters) {

    SPI.begin(AdsSCLK, AdsSDO, AdsSDI);
    SPI.setFrequency(10 * 1000 * 1000);
    ADS1293.setAds1293Pins();

    pinMode(AdsALARM, INPUT_PULLUP);
    attachInterrupt(AdsALARM, isrAdsALARM, FALLING);

    //pinMode(AdsDrdy, INPUT_PULLUP);
    attachInterrupt(AdsDrdy, isrADS1293Complete, FALLING);
    //ADS1293.ads1293Begin3LeadECG();
    ADS1293.ads1293Begin5LeadECG(Hz_400);
    //delay(1);

    //Serial.printf("Free Heap: %d \n", xPortGetFreeHeapSize());
    delay(5000);
    //Serial.printf("%04d%02d%02d%02d%02d%02d\n\r", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    point = 0;
    //esp_timer_init();

    My_timer = timerBegin(1000 * 1000);
    timerAttachInterrupt(My_timer, &onTimer);
    timerAlarm(My_timer, 2500, true, 0);

    while (true) {

        ecgCh1 = ADS1293.getECGdata(1);
        ecgCh2 = ADS1293.getECGdata(2);
        ecgCh3 = ADS1293.getECGdata(3);

        ecgFilteredCh1 = FIR_Filter(ecgCh1, 1) + 4179304;
        ecgFilteredCh2 = FIR_Filter(ecgCh2, 2) + 4194304;
        ecgFilteredCh3 = FIR_Filter(ecgCh3, 3) + 4214304;

        /*Serial.print("ECG , ");
        Serial.print(ecgFilteredCh1);
        Serial.print(" , ");
        Serial.print(ecgFilteredCh2);
        Serial.print(" , ");
        Serial.println(ecgFilteredCh3);*/

        if (recordStatus) {
            if (point >= bufferSize) {
                point = 2;
                sendBuffer = 1 - sendBuffer;
                writeBuffer = 1 - writeBuffer;
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
                    bleSetDataReady();
                    SdCardSetDataReady();
                }
            }
            if (writeBuffer == 0) {
                txArray0[9 * point] = ecgFilteredCh1 >> 16;       //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_H);
                txArray0[9 * point + 1] = ecgFilteredCh1 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_M);
                txArray0[9 * point + 2] = ecgFilteredCh1 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_L);  //ecgCh2 = ADS1293.getECGdata(2);
                txArray0[9 * point + 3] = ecgFilteredCh2 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_H);
                txArray0[9 * point + 4] = ecgFilteredCh2 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_M);
                txArray0[9 * point + 5] = ecgFilteredCh2 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_L);  //ecgCh3 = ADS1293.getECGdata(3);
                txArray0[9 * point + 6] = ecgFilteredCh3 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_H);
                txArray0[9 * point + 7] = ecgFilteredCh3 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_M);
                txArray0[9 * point + 8] = ecgFilteredCh3 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_L);
            }
            if (writeBuffer == 1) {
                txArray1[9 * point] = ecgFilteredCh1 >> 16;       //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_H);
                txArray1[9 * point + 1] = ecgFilteredCh1 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_M);
                txArray1[9 * point + 2] = ecgFilteredCh1 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_L);  //ecgCh2 = ADS1293.getECGdata(2);
                txArray1[9 * point + 3] = ecgFilteredCh2 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_H);
                txArray1[9 * point + 4] = ecgFilteredCh2 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_M);
                txArray1[9 * point + 5] = ecgFilteredCh2 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_L);  //ecgCh3 = ADS1293.getECGdata(3);
                txArray1[9 * point + 6] = ecgFilteredCh3 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_H);
                txArray1[9 * point + 7] = ecgFilteredCh3 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_M);
                txArray1[9 * point + 8] = ecgFilteredCh3 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_L);
            }
            point++;
        } else {
            point++;
            if (point >= bufferSize) {
                point = 0;
                bleSetDataReady();
            }
        }
        diffTime = esp_timer_get_time() - startTime;
        startTime = esp_timer_get_time();
        xSemaphoreTake(SemADS1293, portMAX_DELAY);
    }
}


int32_t FIR_Filter(uint32_t input, int8_t channel) {
    float output = 0;
    if (channel == 1) {
        ch1ReadsBuffer[ch1CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch1ReadsBuffer[(i + ch1CircleIndex) % BL] * B[i];
        }
        ch1CircleIndex = (ch1CircleIndex - 1);
        if (ch1CircleIndex > BL) ch1CircleIndex = BL - 1;
    }
    if (channel == 2) {
        ch2ReadsBuffer[ch2CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch2ReadsBuffer[(i + ch2CircleIndex) % BL] * B[i];
        }
        ch2CircleIndex = (ch2CircleIndex - 1);
        if (ch2CircleIndex > BL) ch2CircleIndex = BL - 1;
    }
    if (channel == 3) {
        ch3ReadsBuffer[ch3CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch3ReadsBuffer[(i + ch3CircleIndex) % BL] * B[i];
        }
        ch3CircleIndex = (ch3CircleIndex - 1);
        if (ch3CircleIndex > BL) ch3CircleIndex = BL - 1;
    }
    return (int32_t)output;
}