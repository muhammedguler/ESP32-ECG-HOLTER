#ifndef ECG_ADS
#define ECG_ADS

#include "defines.h"
#include <SPI.h>
#include "protocentral_ads1293.h"
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"

void ARDUINO_ISR_ATTR isrAdsALARM();
void ARDUINO_ISR_ATTR isrADS1293Complete();

void ADS1293TasksBegin(void);
void ADS1293Tasks(void* Parameters);

#endif