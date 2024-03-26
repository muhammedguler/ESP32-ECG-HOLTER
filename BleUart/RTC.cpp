#include "RTC.h"


MCP7940_Class MCP7940;                    // Create an instance of the MCP7940
const uint8_t SPRINTF_BUFFER_SIZE{ 32 };  // Buffer size for sprintf()

DateTime now;
char TxBuffer[64];
char inputBuffer[64];  // Buffer for sprintf()/sscanf()
static uint8_t inputBytes = 0;
void RTCTaskBegin(void) {
    xTaskCreatePinnedToCore(
        RTCTask, "RTCTask",
        2048 * 4,
        NULL, 1,
        NULL, ARDUINO_RUNNING_CORE);
}


void RTCTask(void* Parameters) {
    delay(3000);
    while (!MCP7940.begin(RtcSDA, RtcSCL)) {  // Initialize RTC communications
        //Serial.println(F("Unable to find MCP7940N. Checking again in 3s."));  // Show error text
        delay(3000);  // wait three seconds
    }                 // of loop until device is located
    //Serial.println(F("MCP7940N initialized."));
    if (MCP7940.getPowerFail()) {  // Check for a power failure
        //Serial.println(F("Power failure mode detected!\n"));
        //Serial.print(F("Power failed at   "));
        DateTime now = MCP7940.getPowerDown();                      // Read when the power failed
        sprintf(inputBuffer, "....-%02d-%02d %02d:%02d:..",         // Use sprintf() to pretty print
                now.month(), now.day(), now.hour(), now.minute());  // date/time with leading zeros
        //Serial.println(inputBuffer);
        //Serial.print(F("Power restored at "));
        now = MCP7940.getPowerUp();                                 // Read when the power restored
        sprintf(inputBuffer, "....-%02d-%02d %02d:%02d:..",         // Use sprintf() to pretty print
                now.month(), now.day(), now.hour(), now.minute());  // date/time with leading zeros
        //Serial.println(inputBuffer);
        MCP7940.clearPowerFail();  // Reset the power fail switch
    } else {
        while (!MCP7940.deviceStatus()) {  // Turn oscillator on if necessary
            //Serial.println(F("Oscillator is off, turning it on."));
            bool deviceStatus = MCP7940.deviceStart();  // Start oscillator and return state
            if (!deviceStatus) {                        // If it didn't start
                //Serial.println(F("Oscillator did not start, trying again."));  // Show error and
                delay(1000);  // wait for a second
            }                 // of if-then oscillator didn't start
        }                     // of while the oscillator is off
        MCP7940.adjust();     // Set to library compile Date/Time
        //Serial.println(F("Enabling battery backup mode"));
        MCP7940.setBattery(true);     // enable battery backup mode
        if (!MCP7940.getBattery()) {  // Check if successful
            //Serial.println(F("Couldn't set Battery Backup, is this a MCP7940N?"));
        }  // if-then battery mode couldn't be set
    }      // of if-then-else we have detected a priorpower failure

    now = MCP7940.now();  // get the current time
    //memset(TxBuffer, 0, 64);
    //sprintf(TxBuffer, "MCP7940.now() %04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    //Serial.println(TxBuffer);
    while (true) {
        delay(499);
        now = MCP7940.now();  // get the current time
        //memset(TxBuffer, 0, 64);
        //sprintf(TxBuffer, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
        ////Serial.println(TxBuffer);
        readCommand();
    }
}
/***************************************************************************************************
** Method readCommand(). This function checks the serial port to see if there has been any input. **
** If there is data it is read until a terminator is discovered and then the command is parsed    **
** and acted upon                                                                                 **
***************************************************************************************************/
void readCommand() {                                                              // Variable for buffer position
    while (Serial.available()) {                                                  // Loop while incoming serial data
        inputBuffer[inputBytes] = Serial.read();                                  // Get the next byte of data
        if (inputBuffer[inputBytes] != '\n' && inputBytes < SPRINTF_BUFFER_SIZE)  // keep on reading until a newline
            inputBytes++;                                                         // shows up or the buffer is full
        else {
            inputBuffer[inputBytes] = 0;                   // Add the termination character
            for (uint8_t i = 0; i < inputBytes; i++)       // Convert the whole input buffer
                inputBuffer[i] = toupper(inputBuffer[i]);  // to uppercase characters
            //Serial.print(F("\nCommand \""));
            Serial.write(inputBuffer);
            //Serial.print(F("\" received.\n"));
            /**********************************************************************************************
      ** Parse the single-line command and perform the appropriate action. The current list of **
      ** commands understood are as follows: **
      ** **
      ** SETDATE      - Set the device time **
      ** CALDATE      - Calibrate device time **
      ** **
      **********************************************************************************************/
            handleRtcCommand();
        }  // of if-then-else we've received full command
    }      // of if-then there is something in our input buffer
    if (bleDataReaded) {
        bleDataReaded = false;
        handleRtcCommand();
    }
}  // of method readCommand


void handleRtcCommand(void) {
    enum commands { SetDate,
                    CalDate,
                    Unknown_Command };          // of commands enumerated type
    commands command;                           // declare enumerated type
    char workBuffer[10];                        // Buffer to hold string compare
    sscanf(inputBuffer, "%s %*s", workBuffer);  // Parse the string for first word
    if (!strcmp(workBuffer, "SETDATE"))
        command = SetDate;  // Set command number when found
    else if (!strcmp(workBuffer, "CALDATE"))
        command = CalDate;  // Set command number when found
    else
        command = Unknown_Command;                            // Otherwise set to not found
    uint16_t tokens, year, month, day, hour, minute, second;  // Variables to hold parsed dt/tm
    switch (command) {                                        // Action depending upon command
        /*******************************************************************************************
        ** Set the device time and date                                                           **
        *******************************************************************************************/
        case SetDate:  // Set the RTC date/time
            tokens = sscanf(inputBuffer, "%*s %hu-%hu-%hu %hu:%hu:%hu;", &year, &month, &day, &hour,
                            &minute, &second);
            if (tokens != 6)  // Check to see if it was parsed
                //Serial.print(F("Unable to parse date/time\n"));
                else {
                    MCP7940.adjust(
                        DateTime(year, month, day, hour, minute, second));  // Adjust the RTC date/time
                    //Serial.print(F("Date has been set."));
                }   // of if-then-else the date could be parsed
            break;  //
        /*******************************************************************************************
        ** Calibrate the RTC and reset the time                                                   **
        *******************************************************************************************/
        case CalDate:  // Calibrate the RTC
            tokens = sscanf(inputBuffer,
                            "%*s %hu-%hu-%hu %hu:%hu:%hu;",                 // Use sscanf() to parse the date/
                            &year, &month, &day, &hour, &minute, &second);  // time into variables
            if (tokens != 6)                                                // Check to see if it was parsed
                //Serial.print(F("Unable to parse date/time\n"));
                else {
                    int8_t trim =
                        MCP7940.calibrate(DateTime(year, month, day,        // Calibrate the crystal and return
                                                   hour, minute, second));  // the new trim offset value
                    //Serial.print(F("Trim value set to "));
                    //Serial.print(trim * 2);  // Each trim tick is 2 cycles
                    //Serial.println(F(" clock cycles every minute"));
                }  // of if-then-else the date could be parsed
            break;
        /*******************************************************************************************
        ** Unknown command                                                                        **
        *******************************************************************************************/
        case Unknown_Command:  // Show options on bad command
        default:
            //Serial.println(F("Unknown command. Valid commands are:"));
            //Serial.println(F("SETDATE yyyy-mm-dd hh:mm:ss"));
            //Serial.println(F("CALDATE yyyy-mm-dd hh:mm:ss"));
    }                // of switch statement to execute commands
    inputBytes = 0;  // reset the counter
}
