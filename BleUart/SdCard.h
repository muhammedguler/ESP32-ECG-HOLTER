#ifndef SdCard
#define SdCard

#include "defines.h"
#include "FS.h"
#include "SD_MMC.h"
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "RTC.h"
#include "ECG.h"
#include "arduino_base64.hpp"

void SdCardTasksBegin(void);
void SdCardTasks(void* parameters);
void SdCardSetDataReady(void);


#endif
