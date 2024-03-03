#ifndef SdCard
#define SdCard

#include "defines.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"

void SdCardTasksBegin(void);
void SdCardTasks(void* parameters);
void SdCardSetDataReady(void);
#endif
