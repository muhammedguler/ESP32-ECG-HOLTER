#include "SdCard.h"

/* SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
*/
// SD kart pin tanımlamaları
int clk = SdCLK;
int cmd = SdCmd;
int d0 = SdDat0;
int d1 = SdDat1;
int d2 = SdDat2;
int d3 = SdDat3;  
// SD kart içine yazılacak veriler için bufferler
char contex[200] = { 0 }, FileName[50] = { 0 }, FolderName[50] = { 0 };
// SD kart'a veri yazma operasyonlarını yöneten semafor
SemaphoreHandle_t SemSDCard;
// SD karta yazılacak verin base64 formatında depolanacağı bufferler
char output0[(9 * bufferSize + 2 - ((9 * bufferSize + 2) % 3)) / 3 * 4 + 1];
char output1[(9 * bufferSize + 2 - ((9 * bufferSize + 2) % 3)) / 3 * 4 + 1];
// RTC entegresi sınıfı
extern MCP7940_Class MCP7940;  // Create an instance of the MCP7940
// SD kart kurulu değilse SdCardSetDataReady fonksiyonunu işlevsizleştiren bayrak
bool isSdInit = false;
// SD kart 
void SdCardSetDataReady(void) {
    /*!
   * @brief     SdCardSetDataReady() fonksiyonu SD karta yazılmaya hazır veri olduğunda semaforu aktifleştirmek için kullanılır 
   * @return    none
   */
    if (isSdInit)
        xSemaphoreGive(SemSDCard);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    /*!
    * @brief     listDir() fonksiyonu SD kart içerisindeki dosya ve klasörlerin seri porta listelenmesini sağlar
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] dirname listelenecek dizinin adı
    * @param[in] levels dizin içerisinde kaç klasör içinin listeleneceğinin seçimi
    * @return    none
    */
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    if (!file) {
        Serial.println("Failed to open file");
        return;
    }
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.print(file.name());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}


void createDir(fs::FS &fs, const char *path) {
    /*!
    * @brief     createDir() fonksiyonu SD kart içerisindeki klasör oluşturmak için kullanılır
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path oluşturulacak klasör yolu
    * @return    none
    */
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path) {
    /*!
    * @brief     removeDir() fonksiyonu SD kart içerisinden klasör silmek için kullanılır
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path silinecek klasör yolu
    * @return    none
    */
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path) {
/*!
    * @brief     readFile() fonksiyonu SD kart içerisindeki dosyanın içeriğini seri porta yazılmasını sağlar
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path okunacak dosya yolu
    * @return    none
    */
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
/*!
    * @brief     writeFile() fonksiyonu SD kart içerisine dosya yazılmasını sağlar 
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path yazılacak dosyanın yolu
    * @param[in] message dosyaya yazılacak mesajın içeriği
    * @return    none
    */
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message1, const char *message2) {
/*!
    * @brief     appendFile() fonksiyonu SD kart içerisindekie dosyaya veri eklenmesini sağlar 
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path yazılacak dosyanın yolu
    * @param[in] message1 dosyaya yazılacak mesajın içeriği
    * @param[in] message2 dosyaya yazılacak mesajın içeriği
    * @return    none
    */
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message1)) {
        Serial.println("Message1 appended");
    } else {
        Serial.println("Append1 failed");
    }
    if (file.print(message2)) {
        Serial.println("Message2 appended");
    } else {
        Serial.println("Append2 failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
/*!
    * @brief     renameFile() fonksiyonu SD kart içerisindekie dosyanın adının değiştirilmesini sağlar 
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path1 ismi değiştirilecek dosyanın yolu
    * @param[in] path2 isim değiştirildikten sonra dosyanın yolu
    * @return    none
    */
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
/*!
    * @brief     deleteFile() fonksiyonu SD kart içerisindekie dosyanın silinmesini sağlar 
    * @param[in] fs dosya sistemi pointeri 
    * @param[in] path silinecek dosyanın yolu
    * @return    none
    */
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void SdCardTasksBegin(void) {
/*!
    * @brief     SdCardTasksBegin() fonksiyonu SD işlemleri için thread ve semafor kurulumunu içerir 
    * @return    none
    */
    SemSDCard = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        SdCardTasks, "SdCardTasks",
        1024 * 4,
        NULL, 2,
        NULL, ARDUINO_RUNNING_CORE);
}


void SdCardTasks(void *parameters) {
/*!
    * @brief        SdCardTasks() fonksiyonu SD işlemleri için oluşturulan thread
    * @details      SD kartın 4 bit modda kurulmasını, cihazın açıldığı günün adı ile kayıt klasörünün oluşturulmasını, 
    cihazın açıldığı saat adi ile kayıt dosyasının oluşturulmasını ve
    kayıt aktif olduğu sürece BLE üzerinden telefona gönerilen verilerin base64 formatında depolanmasını sağlar
    * @param[in]    parameters thread kurulumunda aktarılan parametreler 
    * @return       none
    */
    delay(500);
    if (!SD_MMC.setPins(clk, cmd, d0, d1, d2, d3)) {
        Serial.println("Pin change failed!");
        while (true) {
            delay(1000);
        }
    }
    if (!SD_MMC.begin()) {
        Serial.println("Card Mount Failed");
        while (true) {
            delay(1000);
        }
    }
    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("No SD_MMC card attached");
        while (true) {
            delay(1000);
        }
    }

    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    isSdInit = true;
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluGB\n", cardSize);
    delay(2000);
    listDir(SD_MMC, "/", 1);
    delay(2000);
    now = MCP7940.now();  // get the current time
    sprintf(FolderName, "/%04d%02d%02d", now.year(), now.month(), now.day());
    createDir(SD_MMC, FolderName);
    sprintf(FileName, "/%04d%02d%02d/%02d-%02d-%02d.txt", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    sprintf(contex, "Hamle ECG Holter\n\rLog File for ECG Data\n\rDevice started at  %04d / %02d / %02d  %02d : %02d : %02d \n\rBase64 BLEData:\n\r");
    writeFile(SD_MMC, FileName, contex);

    while (true) {
        xSemaphoreTake(SemSDCard, portMAX_DELAY);
        if (sendBuffer == 0) {
            base64::encode(txArray0, 9 * bufferSize, output0);
        }
        if (sendBuffer == 1) {
            base64::encode(txArray1, 9 * bufferSize, output1);
            appendFile(SD_MMC, FileName, output0, output1);
        }
    }
}
