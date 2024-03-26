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
#define SdCmd 37
#define SdCLK 38
#define SdDat0 39
#define SdDat1 40
#define SdDat2 35
#define SdDat3 36
#define SdDet 45
*/



//Uncomment and set up if you want to use custom pins for the SPI communication
int sck = SdCLK;
int miso = SdDat0;
int mosi = SdCmd;
int cs = SdDat3;
char contex[200] = { 0 }, FileName[50] = { 0 }, FolderName[50] = { 0 };
SemaphoreHandle_t SemSDCard;
char output0[(9 * bufferSize + 2 - ((9 * bufferSize + 2) % 3)) / 3 * 4 + 1];
char output1[(9 * bufferSize + 2 - ((9 * bufferSize + 2) % 3)) / 3 * 4 + 1];

extern MCP7940_Class MCP7940;  // Create an instance of the MCP7940

void SdCardSetDataReady(void) {
    xSemaphoreGive(SemSDCard);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    //Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        //Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        //Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    if (!file) {
        //Serial.println("Failed to open file");
        return;
    }
    while (file) {
        if (file.isDirectory()) {
            //Serial.print("  DIR : ");
            //Serial.print(file.name());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            //Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            //Serial.print("  FILE: ");
            //Serial.print(file.name());
            //Serial.print("  SIZE: ");
            //Serial.print(file.size());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            //Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}


void createDir(fs::FS &fs, const char *path) {
    //Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        //Serial.println("Dir created");
    } else {
        //Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path) {
    //Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        //Serial.println("Dir removed");
    } else {
        //Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path) {
    //Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        //Serial.println("Failed to open file for reading");
        return;
    }

    //Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    //Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        //Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        //Serial.println("File written");
    } else {
        //Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message1, const char *message2) {
    //Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        //Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message1)) {
        //Serial.println("Message appended");
    } else {
        //Serial.println("Append failed");
    }
    if (file.print(message2)) {
        //Serial.println("Message appended");
    } else {
        //Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    //Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        //Serial.println("File renamed");
    } else {
        //Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
    //Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        //Serial.println("File deleted");
    } else {
        //Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        //Serial.printf("%u bytes read for %lu ms\n", flen, end);
        file.close();
    } else {
        //Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if (!file) {
        //Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    //Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
    file.close();
}

void SdCardTasksBegin(void) {
    SemSDCard = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        SdCardTasks, "SdCardTasks",
        1024 * 4,
        NULL, 9,
        NULL, ARDUINO_RUNNING_CORE);
}


void SdCardTasks(void *parameters) {
    SPI.begin(sck, miso, mosi, cs);
    //if(!SD.begin(cs)){ //Change to this function to manually change CS pin
    if (!SD.begin(cs, SPI, 4 * 1000 * 1000)) {
        //Serial.println("Card Mount Failed");
        while (true) {
            delay(1000);
        }
    }
    delay(5000);
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        //Serial.println("No SD card attached");
        while (true) {
            delay(1000);
        }
    }
    /*
    //Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        //Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        //Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        //Serial.println("SDHC");
    } else {
        //Serial.println("UNKNOWN");
    }*/

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    //Serial.printf("SD Card Size: %lluMB\n", cardSize);
    /*delay(1000);
    listDir(SD, "/", 1);
    delay(1000);*/
    now = MCP7940.now();  // get the current time
    sprintf(FolderName, "/%04d%02d%02d", now.year(), now.month(), now.day());
    createDir(SD, FolderName);
    sprintf(FileName, "/%04d%02d%02d/%02d-%02d-%02d.txt", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    sprintf(contex, "Hamle ECG Holter\n\rLog File for ECG Data\n\rDevice started at  %04d / %02d / %02d  %02d : %02d : %02d \n\rBase64 BLEData:\n\r");
    writeFile(SD, FileName, contex);

    while (true) {
        xSemaphoreTake(SemSDCard, portMAX_DELAY);
        if (sendBuffer == 0) {
            base64::encode(txArray0, 9 * bufferSize, output0);
        }
        if (sendBuffer == 1) {
            base64::encode(txArray1, 9 * bufferSize, output1);
            appendFile(SD, FileName, output0, output1);
        }
        //readFile(SD, FileName);
    }
}
