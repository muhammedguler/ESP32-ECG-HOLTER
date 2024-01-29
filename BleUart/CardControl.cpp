#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "CardControl.h"

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

bool Button1Pressed = false, Button2Pressed = false, AdsAlarmOccured = false;



void CardControlTaskBegin(void) {
  //init Digital IO's
  pinMode(ChgStat, INPUT_PULLUP);
  pinMode(ChgInok, INPUT_PULLUP);
  pinMode(Buton1, INPUT_PULLUP);
  pinMode(Buton2, INPUT_PULLUP);
  pinMode(AdsALARM, INPUT_PULLUP);

  attachInterrupt(Buton1, isrButon1, FALLING);

  attachInterrupt(Buton2, isrButon2, FALLING);
  attachInterrupt(AdsALARM, isrAdsALARM, FALLING);
  // Initialize pins as LEDC channels
  // resolution 1-16 bits, freq limits depend on resolution
  ledcAttach(RedLED, 12000, 8);  // 12 kHz PWM, 8-bit resolution
  ledcAttach(GreenLED, 12000, 8);
  ledcAttach(BlueLED, 12000, 8);

  pinMode(SwShdn, OUTPUT);
  digitalWrite(SwShdn, LOW);
  BuzzerInit(Buzzer);

  //ESP32: Set the resolution to 9-12 bits (default is 12 bits)
  analogContinuousSetWidth(12);

  // Setup ADC Continuous with following input:
  // array of pins, count of the pins, how many conversions per pin in one cycle will happen, sampling frequency, callback function
  analogContinuous(ADC_Pins, 2, CONVERSIONS_PER_PIN, 2000, &adcComplete);

  // Start ADC Continuous conversions
  analogContinuousStart();
  xTaskCreatePinnedToCore(
    CardControlTask, "CardControlTask",
    1024 * 4,
    NULL, 3,
    NULL, ARDUINO_RUNNING_CORE);
}
void CardControlTask(void* Parameters) {

  delay(100);
  Serial.println("playing StarWars2");
  /*Starwars();
  delay(100);
  GameOfThrones();
  delay(100);
  HappyBirthday();
  delay(100);
  HarryPotter();
  delay(100);
  Pirate();
  delay(100);
  mario();
  delay(100);
  McGyver();
  delay(100);
  StarWars2();
  delay(100);
  BonBruteTruand();
  delay(100);
  IndianaJones();
  delay(100);
  twentyCentFox();
  delay(100);
  looney();
  delay(100);
  Entertainement();
  delay(100);
  BarbieGirl();
  delay(100);
  Greensleaves();
  delay(100);
  Bond();*/Bond();
  Serial.println("playing over");


  float NtcRes = 0.0, temperature = 0.0;
  while (true) {
    delay(1);
    if (Button1Pressed) {
      Button1Pressed = false;
    }
    if (Button2Pressed) {
      Button2Pressed = false;
    }
    if (AdsAlarmOccured) {
      AdsAlarmOccured = false;
    }

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
}

void ARDUINO_ISR_ATTR isrAdsALARM() {
  AdsAlarmOccured = true;
}
void ARDUINO_ISR_ATTR isrButon1() {
  Button1Pressed = true;
}
void ARDUINO_ISR_ATTR isrButon2() {
  Button2Pressed = true;
}
void ARDUINO_ISR_ATTR adcComplete() {
  ADC_ConversionDone = true;
}
