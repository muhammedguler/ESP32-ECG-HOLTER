#include "BleUart.h"
#include "CardControl.h"
#include "SdCard.h"
uint32_t chipId = 0;

void setup() {
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.begin(115200);
  CardControlTaskBegin();
  BleUartTasksBegin();
  SdCardInit();
}
void loop() {
  sprintf(txArray, "deneme %d deneme ", random(300));
  DataReady = true;
  delay(10);
}