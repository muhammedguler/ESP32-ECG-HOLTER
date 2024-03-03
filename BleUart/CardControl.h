#ifndef CardControl
#define CardControl
#include "defines.h"
#include "songs.h"
#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "BleUart.h"
// Define how many conversion per pin will happen and reading the data will be and average of all conversions
#define CONVERSIONS_PER_PIN 5

#define nominal_resistance 10000  //Nominal resistance at 25⁰C
#define nominal_temeprature 25    // temperature for nominal resistance (almost always 25⁰ C)
#define samplingrate 5            // Number of samples
#define beta 3434                 //25/85 The beta coefficient or the B value of the thermistor
#define Rt 10000                  //Value of  resistor used for the voltage divider
#define R0 10000                  // value of rct in T0 [ohm]
#define T0 298.15                 // use T0 in Kelvin [K]

extern bool Button1Pressed, Button2Pressed;

extern uint8_t ADC_Pins[2];

extern bool recordStatus;
extern float CardTemperatureFloat, TempC;
extern uint32_t BatteryVoltage;
extern uint8_t CardTemperature, BatteryPercentage;
extern uint8_t ChgStatRead, ChgInokRead;
extern uint32_t buttonPressTime[2];

void ARDUINO_ISR_ATTR isrButon1();
void ARDUINO_ISR_ATTR isrButon2();
void ARDUINO_ISR_ATTR adcComplete();

void CardControlTask(void* Parameters);
void CardControlTaskBegin(void);
void AdcOperation(void);
void ledOperation(void);


#endif