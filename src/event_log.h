#pragma once
// ============================================================
// TrassarV3 - Log zdarzen na karte SD
// v2.21.0 - Plik dziennie: /logs/RRRRMMDD.log
// Format: HH:MM:SS [KATEGORIA] tresc
// ============================================================

#include "config.h"

// Max rozmiar pliku logu na jeden dzien [bajtow]
#define EVENT_LOG_MAX_SIZE  65536   // 64 KB

// Max liczba plików logu na karcie SD (stare kasowane automatycznie)
#define EVENT_LOG_MAX_FILES  14   // 14 plików = 7 dni (.log + .old)

class EventLog {
public:
    void begin();
    void log(const char* category, const char* message);
    void logf(const char* category, const char* fmt, ...);

    // Usun stare pliki logu (wywolywane periodycznie z main loop)
    void cleanupOldLogs();

private:
    bool ready = false;
    unsigned long lastCleanupMs = 0;
};

extern EventLog eventLog;
