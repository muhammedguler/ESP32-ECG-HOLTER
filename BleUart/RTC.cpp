#include "RTC.h"


MCP7940_Class MCP7940;  // Create an instance of the MCP7940
char TxBuffer[64];

void RTCTaskBegin(void) {

  xTaskCreatePinnedToCore(
    RTCTask, "RTCTask",
    1024 * 4,
    NULL, 3,
    NULL, ARDUINO_RUNNING_CORE);
}


void RTCTask(void* Parameters) {
  while (!MCP7940.begin(RtcSDA, RtcSCL)) {                                // Initialize RTC communications
    Serial.println(F("Unable to find MCP7940N. Checking again in 3s."));  // Show error text
    delay(3000);                                                          // wait three seconds
  }                                                                       // of loop until device is located
  Serial.println(F("MCP7940N initialized."));
  while (!MCP7940.deviceStatus()) {  // Turn oscillator on if necessary
    Serial.println(F("Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart();                       // Start oscillator and return state
    if (!deviceStatus) {                                             // If it didn't start
      Serial.println(F("Oscillator did not start, trying again."));  // Show error and
      delay(1000);                                                   // wait for a second
    }                                                                // of if-then oscillator didn't start
  }                                                                  // of while the oscillator is off
  MCP7940.adjust();                                                  // Set to library compile Date/Time
  Serial.println(F("Enabling battery backup mode"));
  MCP7940.setBattery(true);     // enable battery backup mode
  if (!MCP7940.getBattery()) {  // Check if successful
    Serial.println(F("Couldn't set Battery Backup, is this a MCP7940N?"));
  }  // if-then battery mode couldn't be set

  while (true) {
    delay(499);
    DateTime now = MCP7940.now();  // get the current time
    memset(TxBuffer, 0, 64);
    sprintf(TxBuffer, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    Serial.println(TxBuffer);
  }
}
