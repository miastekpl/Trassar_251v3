// ============================================================
// TrassarV3 - Logger raportow na karte SD
// Pliki CSV: /reports/RRRRMMDD.csv
// Format: data,godzina,wzorzec,dystans_m,powierzchnia_m2
// ============================================================

#include "report_logger.h"
#include "rtc_handler.h"

ReportLogger reportLogger;

// SD i TFT wspoldziela magistrale HSPI - musimy uzyc tego samego portu
static SPIClass sdSPI(HSPI);

bool ReportLogger::begin() {
    // Inicjalizuj HSPI dla SD (te same piny co TFT: SCK=12, MISO=13, MOSI=11)
    sdSPI.begin(12, 13, 11, PIN_SD_CS);
    sdReady = SD.begin(PIN_SD_CS, sdSPI, 4000000);
    if (sdReady) {
        if (!SD.exists("/reports")) {
            SD.mkdir("/reports");
        }
        Serial.println("[SD] Karta SD gotowa");
    } else {
        Serial.println("[SD] UWAGA: Brak karty SD");
    }
    return sdReady;
}

void ReportLogger::logSession(const char* patCode, float distanceM, float areaM2) {
    if (!sdReady) return;

    DateTime now = rtcModule.now();
    char fname[32];
    snprintf(fname, sizeof(fname), "/reports/%04d%02d%02d.csv",
             now.year(), now.month(), now.day());

    bool newFile = !SD.exists(fname);
    File f = SD.open(fname, FILE_APPEND);
    if (!f) return;

    if (newFile) {
        f.println("data,godzina,wzorzec,dystans_m,powierzchnia_m2");
    }

    char line[128];
    snprintf(line, sizeof(line), "%04d-%02d-%02d,%02d:%02d:%02d,%s,%.1f,%.2f",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second(),
             patCode, distanceM, areaM2);
    f.println(line);
    f.close();

    Serial.printf("[SD] Raport: %s\n", line);
}

int ReportLogger::getReportCount() {
    if (!sdReady) return 0;

    File dir = SD.open("/reports");
    if (!dir) return 0;

    int count = 0;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) count++;
        entry.close();
    }
    dir.close();
    return count;
}

void ReportLogger::refreshReportCache() {
    if (!sdReady) { cachedReportList = "[]"; cacheValid = true; return; }

    File dir = SD.open("/reports");
    if (!dir) { cachedReportList = "[]"; cacheValid = true; return; }

    // Zbierz nazwy plikow (max 50 najnowszych)
    struct FileInfo { char name[32]; size_t size; };
    FileInfo files[50];
    int count = 0;

    while (count < 50) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            strncpy(files[count].name, entry.name(), sizeof(files[count].name) - 1);
            files[count].name[sizeof(files[count].name) - 1] = 0;
            files[count].size = entry.size();
            count++;
        }
        entry.close();
    }
    dir.close();

    // Sortuj malejaco (najnowsze pliki pierwsze - nazwy RRRRMMDD sortuja chronologicznie)
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(files[j].name, files[i].name) > 0) {
                FileInfo tmp = files[i];
                files[i] = files[j];
                files[j] = tmp;
            }
        }
    }

    // Buduj JSON
    String json = "[";
    for (int i = 0; i < count; i++) {
        if (i > 0) json += ",";
        json += "{\"file\":\"";
        json += files[i].name;
        json += "\",\"size\":";
        json += String(files[i].size);
        json += "}";
    }
    json += "]";

    cachedReportList = json;
    cacheValid = true;
}

bool ReportLogger::getLastReport(char* buf, size_t len) {
    if (!sdReady) { buf[0] = 0; return false; }

    File dir = SD.open("/reports");
    if (!dir) { buf[0] = 0; return false; }

    // Znajdz najnowszy plik (nazwy sortuja sie chronologicznie)
    char latestName[32] = {};
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            if (strcmp(name, latestName) > 0) {
                strncpy(latestName, name, sizeof(latestName) - 1);
            }
        }
        entry.close();
    }
    dir.close();

    if (latestName[0] == 0) { buf[0] = 0; return false; }

    char path[48];
    snprintf(path, sizeof(path), "/reports/%s", latestName);
    File f = SD.open(path, FILE_READ);
    if (!f) { buf[0] = 0; return false; }

    // Odczytaj ostatnia linie (pomijajac naglowek)
    char line[128] = {};
    while (f.available()) {
        size_t r = f.readBytesUntil('\n', line, sizeof(line) - 1);
        line[r] = 0;
    }
    f.close();

    strncpy(buf, line, len - 1);
    buf[len - 1] = 0;
    return (buf[0] != 0);
}
