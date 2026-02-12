// ============================================================
// TrassarV3 - Logger raportow na karte SD
// Pliki CSV: /reports/RRRRMMDD.csv
// Format: data,godzina,wzorzec,dystans_m,powierzchnia_m2
// ============================================================

#include "report_logger.h"
#include "rtc_handler.h"

ReportLogger reportLogger;

bool ReportLogger::begin() {
    sdReady = SD.begin(PIN_SD_CS);
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
