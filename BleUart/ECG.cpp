#include "ECG.h"

int32_t EcgData[3] = { 0 };

ads1293 ADS1293(AdsDrdy, AdsCS);
SemaphoreHandle_t  SemADS1293;

BaseType_t xHigherPriorityTaskWoken = pdFALSE;
bool AdsAlarmOccured = false,AdsD;

void ARDUINO_ISR_ATTR isrAdsALARM() {
  AdsAlarmOccured = true;
}
void ARDUINO_ISR_ATTR isrADS1293Complete() {
  xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(SemADS1293, &xHigherPriorityTaskWoken);
}

void ADS1293TasksBegin(void) {

  SemADS1293 = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(
    ADS1293Tasks, "ADS1293Tasks",
    1024 * 4,
    NULL, 3,
    NULL, ARDUINO_RUNNING_CORE);
}
void ADS1293Tasks(void* Parameters) {

  pinMode(AdsALARM, INPUT_PULLUP);
  attachInterrupt(AdsALARM, isrAdsALARM, FALLING);

  pinMode(AdsDrdy, INPUT_PULLUP);
  attachInterrupt(AdsDrdy, isrADS1293Complete, FALLING);

  SPI.begin(AdsSCLK, AdsSDO, AdsSDI);
  ADS1293.setAds1293Pins();
  ADS1293.ads1293Begin5LeadECG();

  while (true) {
    xSemaphoreTake(SemADS1293, portMAX_DELAY);
    EcgData[0] = ADS1293.getECGdata(1);
    EcgData[1] = ADS1293.getECGdata(2);
    EcgData[2] = ADS1293.getECGdata(3);
  }
}
