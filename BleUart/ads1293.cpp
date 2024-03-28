#include "ads1293.h"

//uninitalised pointers to SPI objects
SPIClass* vspi = NULL;

int32_t ads1293::getECGdata(uint8_t channel) {
    /*!
   * @brief     getECGdata() fonksiyonu istenilen ADC'nin verisini okumak için kullanılır 
   * @param[in] channel kanal seçim değişkeni  1. in 1-2 2. in 3 3. in 5
   * @return    24 bit EKG verisi
   */
    uint8_t rawData[3];
    int32_t ecgData;

    if (channel < 1 || channel > 3) {
        return -1;  // return error, -1
    } else {
        channel -= 1;
    }

    rawData[0] = ads1293ReadRegister(0x37 + (channel * 3));
    rawData[1] = ads1293ReadRegister(0x38 + (channel * 3));
    rawData[2] = ads1293ReadRegister(0x39 + (channel * 3));

    uint32_t temp = rawData[0];
    temp = temp << 8;          // shift left
    temp = temp | rawData[1];  // or result with byte 1
    temp = temp << 8;          // shift left
    temp = temp | rawData[2];  // or result with byte 2Ü

    ecgData = (int32_t)(temp);
    return (ecgData);
}

void ads1293::setAds1293Pins() {
    /*!
   * @brief     setAds1293Pins() fonksiyonu ADS1293 entegresinin SPI ve kesme pinlerinin kurulumu için kullanılır.
   * @return    none
   */
    vspi = new SPIClass(FSPI);
    vspi->begin(AdsSCLK, AdsSDO, AdsSDI);
    vspi->setFrequency(10 * 1000 * 1000);
    pinMode(drdyPin, INPUT_PULLUP);
    pinMode(csPin, OUTPUT);
}

void ads1293::ads1293Begin3LeadECG(enum SampleFreq Freq) {
    /*!
   * @brief     ads1293Begin3LeadECG() fonksiyonu ADS1293 entegresini 3 problu modda kurmak için kullanılır. 
   * @param[in] Freq ADS1293 entegresinin örnekleme hızını ayarlamak için kullanılır.
   * @return    none
   */
    ads1293WriteRegister(CONFIG, 0x00);
    delay(1);
    ads1293WriteRegister(FLEX_CH1_CN, 0x11);
    delay(1);
    ads1293WriteRegister(FLEX_CH2_CN, 0x19);
    delay(1);
    ads1293WriteRegister(CMDET_EN, 0x07);
    delay(1);
    ads1293WriteRegister(RLD_CN, 0x04);
    delay(1);
    ads1293WriteRegister(OSC_CN, 0x04);
    delay(1);
    ads1293WriteRegister(AFE_SHDN_CN, 0x24);
    delay(1);
    /*ads1293WriteRegister(R2_RATE, 0x02);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH1, 0x02);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH2, 0x02);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH3, 0x02);*/
    setSamplingRate(Freq);
    delay(1);
    ads1293WriteRegister(DRDYB_SRC, 0x08);
    delay(1);
    ads1293WriteRegister(CH_CNFG, 0x30);
    delay(1);
    ads1293WriteRegister(CONFIG, 0x01);
    delay(1);
    configACleadoffDetect(0, false);
    delay(1);
    configDCleadoffDetect(0, false);
    delay(1);
}

void ads1293::ads1293Begin5LeadECG(enum SampleFreq Freq) {
    /*!
   * @brief     ads1293Begin5LeadECG() fonksiyonu ADS1293 entegresini 5 problu modda kurmak için kullanılır. 
   * @param[in] Freq ADS1293 entegresinin örnekleme hızını ayarlamak için kullanılır.
   * @return    none
   */
    ads1293WriteRegister(CONFIG, 0x00);
    delay(1);
    ads1293WriteRegister(FLEX_CH1_CN, 0x11);
    delay(1);
    ads1293WriteRegister(FLEX_CH2_CN, 0x19);
    delay(1);
    ads1293WriteRegister(FLEX_CH3_CN, 0x2e);
    delay(1);
    ads1293WriteRegister(CMDET_EN, 0x07);
    delay(1);
    ads1293WriteRegister(RLD_CN, 0x04);
    delay(1);
    ads1293WriteRegister(WILSON_EN1, 0x01);
    delay(1);
    ads1293WriteRegister(WILSON_EN2, 0x02);
    delay(1);
    ads1293WriteRegister(WILSON_EN3, 0x03);
    delay(1);
    ads1293WriteRegister(WILSON_CN, 0x01);
    delay(1);
    ads1293WriteRegister(OSC_CN, 0x04);
    delay(1);
    /*ads1293WriteRegister(R2_RATE, 0x02);
  delay(1);
  ads1293WriteRegister(R3_RATE_CH1, 0x02);
  delay(1);
  ads1293WriteRegister(R3_RATE_CH2, 0x02);
  delay(1);
  ads1293WriteRegister(R3_RATE_CH3, 0x02);*/
    setSamplingRate(Freq);
    delay(1);
    ads1293WriteRegister(DRDYB_SRC, 0x08);
    delay(1);
    ads1293WriteRegister(CH_CNFG, 0x70);
    delay(1); /*
    configACleadoffDetect(0, false);
    delay(1);
    configDCleadoffDetect(0, false);*/
    delay(1);
    ads1293WriteRegister(CONFIG, 0x01);
    delay(1);
}
void ads1293::ads1293WriteRegister(uint8_t wrAddress, uint8_t data) {
    /*!
    * @brief     ads1293WriteRegister() fonksiyonu ADS1293 entegresinin seçilen bellek adaresine istenilen verinin yazılmasını sağlar 
    * @param[in] wrAddress ADS1293 entegresinin bellek adresi
    * @param[in] data ilgili bellek adresine yazılacak veri
    * @return    none
    */
    uint8_t dataToSend = (wrAddress & WREG);
    digitalWrite(csPin, LOW);
    vspi->transfer(dataToSend);
    vspi->transfer(data);
    digitalWrite(csPin, HIGH);
}

uint8_t ads1293::ads1293ReadRegister(uint8_t rdAddress) {
    /*!
    * @brief     ads1293ReadRegister() fonksiyonu ADS1293 entegresinin seçilen bellek adresindeki verinin okunmasını sağlar 
    * @param[in] wrAddress ADS1293 entegresinin bellek adresi
    * @return    bellek adresindeki 8 bit veri
    */
    uint8_t rdData;
    uint8_t dataToSend = (rdAddress | RREG);
    digitalWrite(csPin, LOW);
    vspi->transfer(dataToSend);
    rdData = vspi->transfer(0);
    digitalWrite(csPin, HIGH);

    return (rdData);
}

void ads1293::configDCleadoffDetect(uint8_t level, bool active) {
    /*!
    * @brief     configDCleadoffDetect() fonksiyonu ADS1293 entegresinin DC prob çıkma kontrol(Lead Off Detection) fonksiyonunu kontrol etmek için kullanılır. 
    * @param[in] level ADS1293 entegresinin Lead off detection fonksiyonunun tetiklenme seviyesi
    * @param[in] active ADS1293 entegresinin Lead off detection fonksiyonunun durumu
    * @return    none
    */
    uint8_t regLOD_CN = 0x08;
    uint8_t regLOD_EN = 0x00;
    uint8_t regLOD_CURRENT = 0x00;
    uint8_t regLOD_AC_CN = 0x00;
    if (active)
        regLOD_CN = (level & 3) | 0x10;
    regLOD_EN = 0x1F;       // 5 tel aktif
    regLOD_CURRENT = 0x0F;  // 128nA
    ads1293WriteRegister(LOD_CN, regLOD_CN);
    delay(1);
    ads1293WriteRegister(LOD_EN, regLOD_EN);
    delay(1);
    ads1293WriteRegister(LOD_CURRENT, regLOD_CURRENT);
    delay(1);
}

void ads1293::configACleadoffDetect(uint8_t level, bool active) {
    /*!
    * @brief     configACleadoffDetect() fonksiyonu ADS1293 entegresinin AC prob çıkma kontrol(Lead Off Detection) fonksiyonunu kontrol etmek için kullanılır. 
    * @param[in] level ADS1293 entegresinin Lead off detection fonksiyonunun tetiklenme seviyesi
    * @param[in] active ADS1293 entegresinin Lead off detection fonksiyonunun durumu
    * @return    none
    */
    uint8_t regLOD_CN = 0x08;
    uint8_t regLOD_EN = 0x00;
    uint8_t regLOD_CURRENT = 0x00;
    uint8_t regLOD_AC_CN = 0x00;
    if (active)
        regLOD_CN = (level & 3) | 0x14;
    regLOD_EN = 0x1F;       // 5 tel aktif
    regLOD_CURRENT = 0x0F;  // 128nA
    regLOD_AC_CN = 0x0B;    // 1khz
    ads1293WriteRegister(LOD_CN, regLOD_CN);
    delay(1);
    ads1293WriteRegister(LOD_EN, regLOD_EN);
    delay(1);
    ads1293WriteRegister(LOD_CURRENT, regLOD_CURRENT);
    delay(1);
    ads1293WriteRegister(LOD_AC_CN, regLOD_AC_CN);
    delay(1);
}

void ads1293::setSamplingRate(enum SampleFreq Freq) {
    /*!
    * @brief     setSamplingRate() fonksiyonu ADS1293 entegresinin örnekleme frekansını ayarlamak için kullanılır.  
    * @param[in] Freq ADS1293 entegresinin örnekleme frekansı ayarıdır. enum SampleFreq içinde tanımlı olan değerleri alabilir. 
    * @return    none
    */
    uint8_t /*AFE_Val = 0,*/ R1 = 0, R2 = 1, R3 = 1;

    R3 = 1 << (Freq & 7);         // 1=>1/4, 2=>1/6, 4=>1/8...128=>1/128
    R2 = 1 << ((Freq >> 3) & 3);  // 1=>1/4, 2=>1/5, 4=>1/6, 8=>1/8
    //if (((Freq >> 5) & 1) == 0) R1 = 0x07;       // 1/2
    /*if (((Freq >> 6) & 1) == 0) AFE_Val = 0x3F;  //204800
  ads1293WriteRegister(AFE_RES, AFE_Val);
  delay(1);*/
    Serial.printf("R2 : %d  R3 : %d \n\r", R2, R3);

    ads1293WriteRegister(R2_RATE, R2);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH1, R3);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH2, R3);
    delay(1);
    ads1293WriteRegister(R3_RATE_CH3, R3);
    delay(1);
    ads1293WriteRegister(R1_RATE, R1);
    delay(1);
    ads1293WriteRegister(AFE_RES, 7);
    delay(1);
}

uint8_t ads1293::readErrorStatus() {
    /*!
    * @brief     readErrorStatus() fonksiyonu ADS1293 entegresinin hata durumlarını görüntülemek için kullanılır. 
    * @return    8 bit hata bayrakları
    */
    return (ads1293ReadRegister(ERROR_STATUS));
}
uint8_t ads1293::readLeadErrorStatus() {
    /*!
    * @brief     readLeadErrorStatus() fonksiyonu hangi elektrodun takılı olmadığını gösterir.  
    * @return    8 bit çıkık elektrod bayrakları
    */
    return (ads1293ReadRegister(ERROR_LOD));
}

bool ads1293::attachTestSignal(uint8_t channel, uint8_t pol) {
    /*!
    * @brief     attachTestSignal() fonksiyonu ADS1293 entegresinin istenilen kanalına test sinyali uygulamak için kullanılır. 
    * @param[in] channel kanal seçim değişkeni 1. in 1 - 2 2. in 3 3. in 5
    * @param[in] pol seçilen kanalı maksimum, minimum veya sıfır seviyesine setlemek için kullanılır.
    * @return    hata veya başarı durumu döner
    */
    if ((channel > 3) || (channel < 1)) {
        Serial.println("Wrong channel error!");
        return ERROR;
    }

    pol = (pol << 6);
    ads1293WriteRegister((channel), pol);
    delay(1);

    return true;
}
