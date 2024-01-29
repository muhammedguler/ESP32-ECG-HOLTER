#ifndef RTC_MCP
#define RTC_MCP


#include "defines.h"
#include <MCP7940.h>  // Include the MCP7940 RTC library
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"

void RTCTask(void* Parameters);
void RTCTaskBegin(void);

#endif