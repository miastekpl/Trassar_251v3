#pragma once
// ============================================================
// TrassarV3 - Logger raportow na karte SD
// ============================================================

#include "config.h"
#include <SD.h>

class ReportLogger {
public:
    bool begin();
    void logSession(const char* patCode, float distanceM, float areaM2);
    bool isReady() const { return sdReady; }
    int  getReportCount();
    bool getLastReport(char* buf, size_t len);

private:
    bool sdReady = false;
};

extern ReportLogger reportLogger;
