#pragma once
// ============================================================
// TrassarV3 - Automatyczny raport HTML po sesji malowania
// Generowany na karte SD, dostepny do pobrania z panelu WWW
// ============================================================

#include "config.h"

class SessionReport {
public:
    // Generuj raport HTML po zakonczeniu sesji
    bool generateReport(const char* patCode, float distM, float areaM2,
                       unsigned long timeSec, float avgSpeedKmh,
                       bool hasGps, double lat, double lon,
                       float paintUsedL, float paintRemainingL);

    // Ostatnio wygenerowany plik raportu
    const char* getLastReportFile() const { return lastReportFile; }
    bool hasReport() const { return lastReportFile[0] != '\0'; }

private:
    char lastReportFile[64] = {};
};

extern SessionReport sessionReport;
