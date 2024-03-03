#include "BleUart.h"
#include "CardControl.h"
#include "SdCard.h"
#include "RTC.h"
#include "ECG.h"

extern int64_t startTime, diffTime;
void setup() {
    Serial.begin(115200);
    BleUartTasksBegin();
    CardControlTaskBegin();
    SdCardTasksBegin();
    RTCTaskBegin();
    ADS1293TasksBegin();
}
void loop() {
    //sprintf(txArray, "deneme %d deneme ", random(300));
    //DataReady = true;
/*    delay(100);
    if (recordStatus)
        Serial.println(1000000.0 / diffTime);*/
}