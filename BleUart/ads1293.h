#ifndef ADS1293_h_
#define ADS1293_h_

#define WREG 0x7f
#define RREG 0x80

//    REGISTER NAME        ADDRESS        DEFAULT    DESCRIPTION
//    Operation Mode Registers
#define CONFIG 0x00  //    0x02    Main Configuration
//    Input Channel Selection Registers
#define FLEX_CH1_CN 0x01   //    0x00    Flex Routing Switch Control for Channel 1
#define FLEX_CH2_CN 0x02   //    0x00    Flex Routing Switch Control for Channel 2
#define FLEX_CH3_CN 0x03   //    0x00    Flex Routing Switch Control for Channel 3
#define FLEX_PACE_CN 0x04  //    0x00    Flex Routing Switch Control for Pace Channel
#define FLEX_VBAT_CN 0x05  //    0x00    Flex Routing Switch for Battery Monitoring
//    Lead-off Detect Control Registers
#define LOD_CN 0x06       //    0x08    Lead-Off Detect Control
#define LOD_EN 0x07       //    0x00    Lead-Off Detect Enable
#define LOD_CURRENT 0x08  //    0x00    Lead-Off Detect Current
#define LOD_AC_CN 0x09    //    0x00    AC Lead-Off Detect Control
//    Common-Mode Detection and Right-Leg Drive Feedback Control Registers
#define CMDET_EN 0x0A  //    0x00    Common-Mode Detect Enable
#define CMDET_CN 0x0B  //    0x00    Common-Mode Detect Control
#define RLD_CN 0x0C    //    0x00    Right-Leg Drive Control
//    Wilson Control Registers
#define WILSON_EN1 0x0D  //    0x00    Wilson Reference Input one Selection
#define WILSON_EN2 0x0E  //    0x00    Wilson Reference Input two Selection
#define WILSON_EN3 0x0F  //    0x00    Wilson Reference Input three Selection
#define WILSON_CN 0x10   //    0x00    Wilson Reference Control
//    Reference Registers
#define REF_CN 0x11  //    0x00    Internal Reference Voltage Control
//    OSC Control Registers
#define OSC_CN 0x12  //    0x00    Clock Source and Output Clock Control
//    AFE Control Registers
#define AFE_RES 0x13       //    0x00    Analog Front End Frequency and Resolution
#define AFE_SHDN_CN 0x14   //    0x00    Analog Front End Shutdown Control
#define AFE_FAULT_CN 0x15  //    0x00    Analog Front End Fault Detection Control
//    RESERVED              0x16        0x00    —
#define AFE_PACE_CN 0x17  //    0x01    Analog Pace Channel Output Routing Control
//    Error Status Registers
#define ERROR_LOD 0x18     //    —        Lead-Off Detect Error Status
#define ERROR_STATUS 0x19  //    —        Other Error Status
#define ERROR_RANGE1 0x1A  //    —        Channel 1 AFE Out-of-Range Status
#define ERROR_RANGE2 0x1B  //    —        Channel 2 AFE Out-of-Range Status
#define ERROR_RANGE3 0x1C  //    —        Channel 3 AFE Out-of-Range Status
#define ERROR_SYNC 0x1D    //    —        Synchronization Error
#define ERROR_MISC 0x1E    //    0x00    Miscellaneous Errors
//    Digital Registers
#define DIGO_STRENGTH 0x1F  //    0x03    Digital Output Drive Strength
#define R2_RATE 0x21        //    0x08    R2 Decimation Rate
#define R3_RATE_CH1 0x22    //    0x80    R3 Decimation Rate for Channel 1
#define R3_RATE_CH2 0x23    //    0x80    R3 Decimation Rate for Channel 2
#define R3_RATE_CH3 0x24    //    0x80    R3 Decimation Rate for Channel 3
#define R1_RATE 0x25        //    0x00    R1 Decimation Rate
#define DIS_EFILTER 0x26    //    0x00    ECG Filter Disable
#define DRDYB_SRC 0x27      //    0x00    Data Ready Pin Source
#define SYNCB_CN 0x28       //    0x40    SYNCB In/Out Pin Control
#define MASK_DRDYB 0x29     //    0x00    Optional Mask Control for DRDYB Output
#define MASK_ERR 0x2A       //    0x00    Mask Error on ALARMB Pin
//    Reserved              0x2B        0x00    —
//    Reserved              0x2C        0x00    —
//    Reserved              0x2D        0x09    —
#define ALARM_FILTER 0x2E  //    0x33    Digital Filter for Analog Alarm Signals
#define CH_CNFG 0x2F       //    0x00    Configure Channel for Loop Read Back Mode
//    Pace and ECG Data Read Back Registers
#define DATA_STATUS 0x30      //    —        ECG and Pace Data Ready Status
#define DATA_CH1_PACE_H 0x31  //    —        Channel 1 Pace Data
#define DATA_CH1_PACE_L 0x32
#define DATA_CH2_PACE_H 0x33  //    —        Channel 2 Pace Data
#define DATA_CH2_PACE_L 0x34
#define DATA_CH3_PACE_H 0x35  //    —        Channel 3 Pace Data
#define DATA_CH3_PACE_L 0x36
#define DATA_CH1_ECG_H 0x37  //    —        Channel 1 ECG Data
#define DATA_CH1_ECG_M 0x38
#define DATA_CH1_ECG_L 0x39
#define DATA_CH2_ECG_H 0x3A  //    —        Channel 2 ECG Data
#define DATA_CH2_ECG_M 0x3B
#define DATA_CH2_ECG_L 0x3C
#define DATA_CH3_ECG_H 0x3D  //    —        Channel 3 ECG Data
#define DATA_CH3_ECG_M 0x3E
#define DATA_CH3_ECG_L 0x3F
#define REVID 0x40      //    0x01    Revision ID
#define DATA_LOOP 0x50  //    —        Loop Read-Back Address

#define POSITIVE_TST_SIG 0x01
#define NEGATIVE_TST_SIG 0x02
#define ZERO_TST_SIG 0x03
#define ERROR -1
enum SampleFreq {
    Hz_25 = 127,
    Hz_33 = 119,
    Hz_40 = 111,
    Hz_50 = 126,
    Hz_66 = 118,
    Hz_80 = 110,
    Hz_100 = 125,
    Hz_133 = 117,
    Hz_160 = 109,
    Hz_200 = 124,
    Hz_266 = 123,
    Hz_320 = 108,
    Hz_355 = 115,
    Hz_400 = 122,
    Hz_426 = 107,
    Hz_533 = 121,
    Hz_640 = 106,
    Hz_711 = 113,
    Hz_800 = 120,
    Hz_853 = 105,
    Hz_1066 = 112,
    Hz_1280 = 104,
    Hz_1422 = 81,
    Hz_1600 = 96,
    Hz_1706 = 73,
    Hz_2133 = 80,
    Hz_2560 = 72,
    Hz_2844 = 17,
    Hz_3200 = 64,
    Hz_3413 = 9,
    Hz_4266 = 16,
    Hz_5120 = 8,
    Hz_6400 = 0
};
class ads1293 {
  public:
    uint8_t drdyPin;
    uint8_t csPin;

    void ads1293Begin3LeadECG();
    void ads1293Begin5LeadECG(enum SampleFreq Freq);
    int32_t getECGdata(uint8_t channel);
    bool readSensorID();
    void setAds1293Pins();
    void disableCh1();
    uint8_t ads1293ReadRegister(uint8_t rdAddress);
    uint8_t readErrorStatus(uint8_t rdAddress);
    uint8_t readLeadErrorStatus();
    bool attachTestSignal(uint8_t channel, uint8_t pol);
    void setSamplingRate(enum SampleFreq);
    void disableFilterAllChannels();
    void disableFilter(uint8_t channel);
    uint8_t readErrorStatus();

    void ads1293WriteRegister(uint8_t wrAddress, uint8_t data);
    ads1293(uint8_t drdy, uint8_t chipSelect) {
        csPin = chipSelect;
        drdyPin = drdy;
    }

  private:
    void configDCleadoffDetect(uint8_t level, bool active);
    void configACleadoffDetect(uint8_t level, bool active);
};

#endif
