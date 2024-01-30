#include "ECG.h"

int32_t ecgCh1 = 0;
int32_t ecgCh2 = 0;
int32_t ecgCh3 = 0;
ads1293 ADS1293(AdsDrdy, AdsCS);
SemaphoreHandle_t SemADS1293;

BaseType_t xHigherPriorityTaskWoken = pdFALSE;
bool AdsAlarmOccured = false, AdsD;

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
    2048 * 4,
    NULL, 2,
    NULL, ARDUINO_RUNNING_CORE);
}
void ADS1293Tasks(void* Parameters) {

  SPI.begin(AdsSCLK, AdsSDO, AdsSDI);
  SPI.setFrequency(10000000);
  ADS1293.setAds1293Pins();

  pinMode(AdsALARM, INPUT_PULLUP);
  attachInterrupt(AdsALARM, isrAdsALARM, FALLING);

  //pinMode(AdsDrdy, INPUT_PULLUP);
  attachInterrupt(AdsDrdy, isrADS1293Complete, FALLING);
  ADS1293.ads1293Begin3LeadECG();
  //ADS1293.ads1293Begin5LeadECG(Hz_400);
  delay(1);
  /*
  ADS1293.attachTestSignal(1, POSITIVE_TST_SIG);
    delay(1);

  ADS1293.attachTestSignal(2, ZERO_TST_SIG);
    delay(1);

  ADS1293.attachTestSignal(3, NEGATIVE_TST_SIG);*/
  delay(5000);
  int32_t offset = 1<<22;
  while (true) {
    //delay(1);

    ecgCh1 = ADS1293.getECGdata(1);
    //ecgCh2 = ADS1293.getECGdata(2);
    //ecgCh3 = ADS1293.getECGdata(3);
    Serial.print(ecgCh1-offset);
    /*Serial.print(",");
    Serial.print(ecgCh2-offset);
    Serial.print(",");
    Serial.print(ecgCh3-offset);*/
    Serial.println(",ECG");
    xSemaphoreTake(SemADS1293, portMAX_DELAY);
  }
}
