#ifndef ECG_ADS
#define ECG_ADS

#include "defines.h"
#include <SPI.h>
#include "ads1293.h"
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "BleUart.h"
#include "CardControl.h"
#include "RTC.h"
#include "SdCard.h"
#include "Arduino.h"

#define bufferSize 56

extern uint8_t sendBuffer;

extern bool bleDataReady, SdCardDataReady;
extern SemaphoreHandle_t SemADS1293;

void ARDUINO_ISR_ATTR isrAdsALARM();
void ARDUINO_ISR_ATTR isrADS1293Complete();

void ADS1293TaskBegin(void);
void ADS1293Task(void* Parameters);
int32_t FIR_Filter(uint32_t input, int8_t channel);

#endif