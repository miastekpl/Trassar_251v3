#pragma once
// ============================================================
// TrassarV3 - Logger raportow na karte SD
// ============================================================

#include "config.h"
#include <SPI.h>
#include <SD.h>

class ReportLogger {
public:
    bool begin();
    void logSession(const char* patCode, float distanceM, float areaM2, double lat = 0, double lng = 0);
    bool isReady() const { return sdReady; }
    int  getReportCount();
    bool getLastReport(char* buf, size_t len);

    // Cache listy raportow (odswiezany z Core 1, czytany z Core 0)
    void refreshReportCache();
    String getReportListJson() const {
        taskENTER_CRITICAL(&cacheMux);
        String copy = cachedReportList;
        taskEXIT_CRITICAL(&cacheMux);
        return copy;
    }

    // Spinlock chroniacy cachedReportList (Core 0 czyta, Core 1 pisze)
    mutable portMUX_TYPE cacheMux = portMUX_INITIALIZER_UNLOCKED;

private:
    bool sdReady = false;
    String cachedReportList = "[]";
    volatile bool cacheValid = false;
};

extern ReportLogger reportLogger;
