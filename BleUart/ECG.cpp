#include "ECG.h"
#include "fdacoefs.h"
// ham EKG verisi değişkenleri
int32_t ecgCh1 = 0;
int32_t ecgCh2 = 0;
int32_t ecgCh3 = 0;
// filtrelenmiş EKG verisi değişkenleri
int32_t ecgFilteredCh1 = 0;
int32_t ecgFilteredCh2 = 0;
int32_t ecgFilteredCh3 = 0;
// filtreleme için kullanılan halka buffer'de son yazılan verinini indesi
uint32_t ch1CircleIndex = 0, ch2CircleIndex = 0, ch3CircleIndex = 0;
// filtreleme için kullanılan halka buffer
uint32_t ch1ReadsBuffer[BL], ch2ReadsBuffer[BL], ch3ReadsBuffer[BL];
//hassas zamanlama için timer kesmesi kurulumu
hw_timer_t* My_timer = NULL;
// ADS1293 sınıfı örneğinin oluşturulması
ads1293 ADS1293(AdsDrdy, AdsCS);
//EKG okuma işlemleri için kullanılacak semafor tanımlaması
SemaphoreHandle_t SemADS1293;
// BLE giden veri bufferinde son yazılan verinin indesi
uint16_t point = 0;
// kesme içerisinden semafor etkinleştirmek için kullanılan değişken
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
// alarm durum göstergesi
bool AdsAlarmOccured = false;
// gönerme ve depolama buffer seçimi için bayraklar
uint8_t sendBuffer = 1, writeBuffer = 0;

void ARDUINO_ISR_ATTR isrAdsALARM() {
    /*!
    * @brief     isrAdsALARM() fonksiyonu ADS1293 entegresinde alarm oluştuğunda tetiklenen fonksiyon.
    * @return    none
    */
    AdsAlarmOccured = true;
}
void ARDUINO_ISR_ATTR isrADS1293Complete() {
    /*!
    * @brief     isrADS1293Complete() fonksiyonu ADS1293 entegresinde çevrim tamamlandığında tetiklenen fonksiyon.
    * @details   (kart tasarım hatası sebebiyle stabil çalışmıyor bu yüzden kullanılmıyor.)
    * @return    none
    */
    xHigherPriorityTaskWoken = pdFALSE;
    //xSemaphoreGiveFromISR(SemADS1293, &xHigherPriorityTaskWoken);
}
void IRAM_ATTR onTimer() {
    /*!
    * @brief     onTimer() fonksiyonu ADS1293'ten 400hz örnekleme frekansı ile okuma yapılması için kurulan timer'in kesmesi
    * @return    none
    */
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(SemADS1293, &xHigherPriorityTaskWoken);
}
void ADS1293TaskBegin(void) {
    /*!
    * @brief     ADS1293TasksBegin() fonksiyonu EKG okuma işlemleri için thread ve semafor kurulumunu içerir 
    * @return    none
    */
    SemADS1293 = xSemaphoreCreateBinary();
    //xSemaphoreGive(SemADS1293);
    xTaskCreatePinnedToCore(
        ADS1293Task, "ADS1293Tasks",
        4096 * 4,
        NULL, 7,
        NULL, ARDUINO_RUNNING_CORE);
}
void ADS1293Task(void* Parameters) {
    /*!
    * @brief        ADS1293Tasks() fonksiyonu EKG okuma işlemleri için oluşturulan thread
    * @details      ADS1293'ün pin kurulumlarını, 400sps ve 5 prob modunda okuma ayarlarını içerir.
    Ayrıca EKG verisinin okunmasını, filtrelenmesini ve gönderme bufferlarına yazılmasını sağlar. 
    * @param[in]    parameters thread kurulumunda aktarılan parametreler 
    * @return       none
    */
    ADS1293.setAds1293Pins();

    pinMode(AdsALARM, INPUT_PULLUP);
    attachInterrupt(AdsALARM, isrAdsALARM, FALLING);

    //pinMode(AdsDrdy, INPUT_PULLUP);
    attachInterrupt(AdsDrdy, isrADS1293Complete, FALLING);
    //ADS1293.ads1293Begin3LeadECG();
    ADS1293.ads1293Begin5LeadECG(Hz_400);
    //delay(1);

    Serial.printf("Free Heap: %d \n", xPortGetFreeHeapSize());
    delay(5000);
    //Serial.printf("%04d%02d%02d%02d%02d%02d\n\r", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    point = 0;
    //esp_timer_init();

    My_timer = timerBegin(1000 * 1000);
    timerAttachInterrupt(My_timer, &onTimer);
    timerAlarm(My_timer, 2500, true, 0);

    while (true) {

        ecgCh1 = ADS1293.getECGdata(1);
        ecgCh2 = ADS1293.getECGdata(2);
        ecgCh3 = ADS1293.getECGdata(3);

        ecgFilteredCh1 = FIR_Filter(ecgCh1, 1) + 4179304;
        ecgFilteredCh2 = FIR_Filter(ecgCh2, 2) + 4194304;
        ecgFilteredCh3 = FIR_Filter(ecgCh3, 3) + 4214304;
        /*
        Serial.print("ECG , ");
        Serial.print(ecgFilteredCh1);
        Serial.print(" , ");
        Serial.print(ecgFilteredCh2);
        Serial.print(" , ");
        Serial.print(ecgFilteredCh3);
        Serial.print(" , ");
        Serial.print(ecgCh1);
        Serial.print(" , ");
        Serial.print(ecgCh2);
        Serial.print(" , ");
        Serial.println(ecgCh3);*/

        if (recordStatus) {
            if (point >= bufferSize) {
                point = 2;
                sendBuffer = 1 - sendBuffer;
                writeBuffer = 1 - writeBuffer;
                bleSetDataReady();
                SdCardSetDataReady();
            }
            if (writeBuffer == 0) {
                txArray0[9 * point] = ecgFilteredCh1 >> 16;       //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_H);
                txArray0[9 * point + 1] = ecgFilteredCh1 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_M);
                txArray0[9 * point + 2] = ecgFilteredCh1 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_L);  //ecgCh2 = ADS1293.getECGdata(2);
                txArray0[9 * point + 3] = ecgFilteredCh2 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_H);
                txArray0[9 * point + 4] = ecgFilteredCh2 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_M);
                txArray0[9 * point + 5] = ecgFilteredCh2 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_L);  //ecgCh3 = ADS1293.getECGdata(3);
                txArray0[9 * point + 6] = ecgFilteredCh3 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_H);
                txArray0[9 * point + 7] = ecgFilteredCh3 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_M);
                txArray0[9 * point + 8] = ecgFilteredCh3 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_L);
            }
            if (writeBuffer == 1) {
                txArray1[9 * point] = ecgFilteredCh1 >> 16;       //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_H);
                txArray1[9 * point + 1] = ecgFilteredCh1 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_M);
                txArray1[9 * point + 2] = ecgFilteredCh1 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH1_ECG_L);  //ecgCh2 = ADS1293.getECGdata(2);
                txArray1[9 * point + 3] = ecgFilteredCh2 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_H);
                txArray1[9 * point + 4] = ecgFilteredCh2 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_M);
                txArray1[9 * point + 5] = ecgFilteredCh2 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH2_ECG_L);  //ecgCh3 = ADS1293.getECGdata(3);
                txArray1[9 * point + 6] = ecgFilteredCh3 >> 16;   //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_H);
                txArray1[9 * point + 7] = ecgFilteredCh3 >> 8;    //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_M);
                txArray1[9 * point + 8] = ecgFilteredCh3 & 0xFF;  //ADS1293.ads1293ReadRegister(DATA_CH3_ECG_L);
            }
            point++;
        } else {
            point++;
            if (point >= bufferSize) {
                point = 0;
                bleSetDataReady();
            }
        }
        xSemaphoreTake(SemADS1293, portMAX_DELAY);
    }
}


int32_t FIR_Filter(uint32_t input, int8_t channel) {
    /*!
    * @brief        FIR_Filter() fonksiyonu okunan ham EKG verisinin filtrelenmesini sağlar
    * @details      Ham EKG verisinden DC ofseti ve 50Hz, 100Hz, 150Hz ve 200Hz de bulunan 
    şebeke gürültüsü harmoniklerinin etkilerini elimine etmek için kullanılan filtre fonksiyonu.
    4000. dereceden least-squares metodu kullanılarakoluşturulan FIR filtre kodu 
    * @param[in] input filtrelenecek ham ADS1293 verisi 
    * @param[in] channel filtrelenecek ADS1293 kanalı
    * @return       filtrelenmiş EKG verisi
    */
    float output = 0;
    if (channel == 1) {
        ch1ReadsBuffer[ch1CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch1ReadsBuffer[(i + ch1CircleIndex) % BL] * B[i];
        }
        ch1CircleIndex = (ch1CircleIndex - 1);
        if (ch1CircleIndex > BL) ch1CircleIndex = BL - 1;
    }
    if (channel == 2) {
        ch2ReadsBuffer[ch2CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch2ReadsBuffer[(i + ch2CircleIndex) % BL] * B[i];
        }
        ch2CircleIndex = (ch2CircleIndex - 1);
        if (ch2CircleIndex > BL) ch2CircleIndex = BL - 1;
    }
    if (channel == 3) {
        ch3ReadsBuffer[ch3CircleIndex] = input;
        for (int i = 0; i < BL; i++) {
            output += ch3ReadsBuffer[(i + ch3CircleIndex) % BL] * B[i];
        }
        ch3CircleIndex = (ch3CircleIndex - 1);
        if (ch3CircleIndex > BL) ch3CircleIndex = BL - 1;
    }
    return (int32_t)output;
}