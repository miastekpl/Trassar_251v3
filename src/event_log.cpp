// ============================================================
// TrassarV3 - Log zdarzen na karte SD
// v2.20.0 - Zapis zdarzen systemowych do /logs/RRRRMMDD.log
//
// Format linii:  HH:MM:SS [KATEGORIA] tresc zdarzenia
// Jeden plik na dzien, max 64KB (potem stop do nastepnego dnia)
// ============================================================

#include "event_log.h"
#include "rtc_handler.h"
#include "report_logger.h"
#include <SD.h>
#include <stdarg.h>

EventLog eventLog;

void EventLog::begin() {
    ready = true;
    Serial.println("[LOG] Event log gotowy");
}

void EventLog::log(const char* category, const char* message) {
    // Zawsze drukuj na Serial niezaleznie od SD
    Serial.printf("[%s] %s\n", category, message);

    if (!ready || !reportLogger.isReady()) return;

    DateTime now = rtcModule.now();

    // Sciezka: /logs/RRRRMMDD.log
    char path[28];
    snprintf(path, sizeof(path), "/logs/%04d%02d%02d.log",
             now.year(), now.month(), now.day());

    // Utworz katalog przy pierwszym zapisie
    if (!SD.exists("/logs")) {
        SD.mkdir("/logs");
    }

    File f = SD.open(path, FILE_APPEND);
    if (!f) return;

    // Limit rozmiaru pliku
    if (f.size() > EVENT_LOG_MAX_SIZE) {
        f.close();
        return;
    }

    // Zapis linii: HH:MM:SS [KAT] wiadomosc
    char line[200];
    snprintf(line, sizeof(line), "%02d:%02d:%02d [%s] %s\n",
             now.hour(), now.minute(), now.second(),
             category, message);
    f.print(line);
    f.close();
}

void EventLog::logf(const char* category, const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log(category, buf);
}
