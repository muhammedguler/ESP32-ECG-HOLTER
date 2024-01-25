#include "CardControl.h"
#include "pitches.h"

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};
uint8_t ADC_Pins[] = { VTemp, VBat };
// Flag which will be set in ISR when conversion is done
uint8_t ADC_ConversionDone = false;
// Result structure for ADC Continuous reading
adc_continuos_data_t* ADC_Result = NULL;

float BatteryVoltage = 0.0, CardTemperature = 0.0;

#define nominal_resistance 10000  //Nominal resistance at 25⁰C
#define nominal_temeprature 25    // temperature for nominal resistance (almost always 25⁰ C)
#define samplingrate 5            // Number of samples
#define beta 3434                 //25/85 The beta coefficient or the B value of the thermistor (usually 3000-4000) check the datasheet for the accurate value.
#define Rref 10000                //Value of  resistor used for the voltage divider


// ISR Function that will be triggered when ADC conversion is done
void ARDUINO_ISR_ATTR adcComplete() {
  ADC_ConversionDone = true;
}

void CardControlTaskBegin(void) {
  //init Digital IO's
  pinMode(ChgStat, INPUT_PULLUP);
  pinMode(ChgInok, INPUT_PULLUP);
  pinMode(Buton1, INPUT_PULLUP);
  pinMode(Buton2, INPUT_PULLUP);
  pinMode(AdsALARM, INPUT_PULLUP);

  // Initialize pins as LEDC channels
  // resolution 1-16 bits, freq limits depend on resolution
  ledcAttach(RedLED, 12000, 8);  // 12 kHz PWM, 8-bit resolution
  ledcAttach(GreenLED, 12000, 8);
  ledcAttach(BlueLED, 12000, 8);

  //ESP32: Set the resolution to 9-12 bits (default is 12 bits)
  analogContinuousSetWidth(12);

  // Setup ADC Continuous with following input:
  // array of pins, count of the pins, how many conversions per pin in one cycle will happen, sampling frequency, callback function
  analogContinuous(ADC_Pins, 2, CONVERSIONS_PER_PIN, 20000, &adcComplete);

  // Start ADC Continuous conversions
  analogContinuousStart();
  xTaskCreatePinnedToCore(
    CardControlTask, "CardControlTask",
    1024 * 4,
    NULL, 3,
    NULL, ARDUINO_RUNNING_CORE);
}
void CardControlTask(void* Parameters) {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(Buzzer, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(Buzzer);
  }
  float NtcRes = 0.0, temperature = 0.0;
  while (true) {
    delay(1);

    
    if (ADC_ConversionDone) {
      ADC_ConversionDone = false;
      if (analogContinuousRead(&ADC_Result, 0)) {
        BatteryVoltage = ADC_Result[0].avg_read_mvolts / 500.0;

        // Calculate NTC resistance
        NtcRes = 4095.0 / ADC_Result[0].avg_read_raw - 1;
        NtcRes = Rref / NtcRes;
        temperature = NtcRes / nominal_resistance;            // (R/Ro)
        temperature = log(temperature);                       // ln(R/Ro)
        temperature /= beta;                                  // 1/B * ln(R/Ro)
        temperature += 1.0 / (nominal_temeprature + 273.15);  // + (1/To)
        temperature = 1.0 / temperature;                      // Invert
        CardTemperature = temperature - 273.15;               // convert absolute temp to C

      }
    }
  }
