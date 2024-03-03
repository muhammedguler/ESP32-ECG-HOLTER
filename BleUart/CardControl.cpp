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


float Vout = 0.0;  // Vout in A0
float Rout = 0.0;  // Rout in A0
// use the datasheet to get this data.
float Rinf = 0.0;   // initial parameters [ohm]
float TempK = 0.0;  // variable output
float TempC = 0.0;  // variable output
float CardTemperatureFloat = 0.0;
uint32_t BatteryVoltage;
uint8_t CardTemperature = 25, BatteryPercentage = 50;
bool Button1Pressed = false, Button2Pressed = false;
uint32_t buttonPressTime[2];
uint32_t buttonRelaseTime[2];
uint8_t ChgStatRead, ChgInokRead;
uint32_t recordTime[2];
bool recordStatus = false;
bool deviceClosing = false;

void CardControlTaskBegin(void) {
    //init Digital IO's
    pinMode(SwShdn, OUTPUT);
    digitalWrite(SwShdn, LOW);

    pinMode(ChgStat, INPUT_PULLUP);
    pinMode(ChgInok, INPUT_PULLUP);

    pinMode(Buton1, INPUT_PULLUP);
    attachInterrupt(Buton1, isrButon1, FALLING);

    pinMode(Buton2, INPUT_PULLUP);
    attachInterrupt(Buton2, isrButon2, FALLING);
    // Initialize pins as LEDC channels
    // resolution 1-16 bits, freq limits depend on resolution
    ledcAttach(RedLED, 12000, 8);  // 12 kHz PWM, 8-bit resolution
    ledcAttach(GreenLED, 12000, 8);
    ledcAttach(BlueLED, 12000, 8);

    ledcWrite(RedLED, 255);
    ledcWrite(GreenLED, 255);
    ledcWrite(BlueLED, 255);

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
        NULL, 1,
        NULL, ARDUINO_RUNNING_CORE);
}
void CardControlTask(void* Parameters) {
    Rinf = R0 * exp(-beta / T0);
    HarryStart();
    while (true) {
        delay(1);
        AdcOperation();
        ledOperation();
        ChgStatRead = 1 - digitalRead(ChgStat);  // flas şarj oluyor 1 dolu 0
        ChgInokRead = 1 - digitalRead(ChgInok);  // şarj aleti bağlı

        //if (millis() % 2500 == 0)
        ///   Serial.println("CS " + String(ChgStatRead) + " CI " + String(ChgInokRead) + " BV " + String(BatteryVoltage) + " BP " + String(BatteryPercentage) + " T " + String(CardTemperature));
        if (Button1Pressed) {
            if (digitalRead(Buton1) == 0) {
                if (millis() - buttonPressTime[0] > 500)
                    buttonRelaseTime[0] = millis();
                if (millis() - buttonPressTime[0] > 5000)
                    deviceClosing = true;
            }
            if ((digitalRead(Buton1) == 1) && (millis() - buttonRelaseTime[0] < 10)) {
                Button1Pressed = false;
                recordTime[recordStatus] = millis();
                recordStatus = !recordStatus;
            }
        }
        if ((recordStatus == false) && (millis() - recordTime[0] > 3600000))
            deviceClosing = true;
    }
}

void ARDUINO_ISR_ATTR isrButon1() {
    Button1Pressed = true;
    buttonPressTime[0] = millis();
}
void ARDUINO_ISR_ATTR isrButon2() {
    Button2Pressed = true;
    buttonPressTime[1] = millis();
}
void ARDUINO_ISR_ATTR adcComplete() {
    ADC_ConversionDone = true;
}

void AdcOperation(void) {
    if (ADC_ConversionDone) {
        ADC_ConversionDone = false;
        if (analogContinuousRead(&ADC_Result, 0)) {
            BatteryVoltage = ADC_Result[0].avg_read_mvolts * 2;
            if (BatteryVoltage < 3000)  //low battery
                deviceClosing = true;
            BatteryPercentage = map(BatteryVoltage, 3000, 4200, 0, 250);
            Vout = ((float)(ADC_Result[0].avg_read_raw) / 4095.0);  // calc for ntc
            Rout = ((1 / Vout) * Rt) - Rt;                          //((Vi / Vo) / R2) - R2 = Rout
            //Rout = Rt * Vout / (1 - Vout);
            TempK = (beta / log(Rout / Rinf));  // calc for temperature
            TempC = TempK - 273.15;
            CardTemperature = (int)(TempC);
        }
    }
}
bool fade_ended = false;  // status of LED fade
bool fade_on = true;
uint8_t blefadeStatus = false;


void ledOperation() {
    if (deviceClosing) {
        ledcWrite(RedLED, 255);
        ledcWrite(GreenLED, 0);
        ledcWrite(BlueLED, 0);
        MarioGameOver();  //device will be close play closing song
        digitalWrite(SwShdn, HIGH);
    } else if (ChgInokRead) {
        ledcWrite(RedLED, 255 - BatteryPercentage);
        ledcWrite(GreenLED, BatteryPercentage);
        ledcWrite(BlueLED, 0);
    } else {
        if (deviceConnected) {
            if ((millis() % 6000 < 1500) && (fade_on == true)) {
                fade_on = false;
                ledcWrite(RedLED, 0);
                ledcWrite(GreenLED, 0);
                ledcFade(BlueLED, 0, 255, 1500);
            } else if ((millis() % 6000 > 1500) && (millis() % 6000 < 3000) && (fade_on == false)) {
                fade_on = true;
                ledcWrite(RedLED, 0);
                ledcWrite(GreenLED, 0);
                ledcFade(BlueLED, 255, 0, 1500);
            }
        }
        if (recordStatus) {
            if ((millis() % 6000 > 3000) && (millis() % 6000 < 4500) && (fade_on == true)) {
                fade_on = false;
                ledcWrite(RedLED, 0);
                ledcFade(GreenLED, 0, 255, 1500);
                ledcWrite(BlueLED, 0);
            } else if ((millis() % 6000 > 4500) && (fade_on == false)) {
                fade_on = true;
                ledcWrite(RedLED, 0);
                ledcFade(GreenLED, 255, 0, 1500);
                ledcWrite(BlueLED, 0);
            }
        } else if (recordTime[1] > 100) {
            if ((millis() % 6000 > 3000) && (millis() % 6000 < 4500) && (fade_on == true)) {
                fade_on = false;
                ledcFade(RedLED, 0, 255, 1500);
                ledcFade(GreenLED, 0, 255, 1500);
                ledcWrite(BlueLED, 0);
            } else if ((millis() % 6000 > 4500) && (fade_on == false)) {
                fade_on = true;
                ledcFade(RedLED, 255, 0, 1500);
                ledcFade(GreenLED, 255, 0, 1500);
                ledcWrite(BlueLED, 0);
            }
        }
    }
}