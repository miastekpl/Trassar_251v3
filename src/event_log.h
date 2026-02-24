#pragma once
// ============================================================
// TrassarV3 - Log zdarzen na karte SD
// v2.20.0 - Plik dziennie: /logs/RRRRMMDD.log
// Format: HH:MM:SS [KATEGORIA] tresc
// ============================================================

#include "config.h"

// Max rozmiar pliku logu na jeden dzien [bajtow]
#define EVENT_LOG_MAX_SIZE  65536   // 64 KB

class EventLog {
public:
    void begin();
    void log(const char* category, const char* message);
    void logf(const char* category, const char* fmt, ...);

private:
    bool ready = false;
};

extern EventLog eventLog;
