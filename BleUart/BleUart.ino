#include "BleUart.h"
#include "CardControl.h"
#include "SdCard.h"
#include "RTC.h"
#include "ECG.h"

void setup() {
    /*!
    * @brief        setup() fonksiyonu ilk açılışta tetiklenen ana fonksiyon
    * @details      Cihaz açıldığında BleUart, CardControl, SdCard, RTC, ADS1293 threadlerinin başlatıldığı yer.
    * @return       none
    */
    Serial.begin(115200);
    BleUartTasksBegin();
    CardControlTaskBegin();
    SdCardTasksBegin();
    RTCTaskBegin();
    ADS1293TaskBegin();
}
void loop() {
    /*!
    * @brief        loop() fonksiyonu multi thread programlama mantığında kullanılmıyor 
    * @details      tüm işlemler kendilerine ait threadler içerisinde yapıldığı için Loop fonksiyonu kullanılmıyor.
    * @return       none
    */
    delay(1000);
}