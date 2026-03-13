// ============================================================
// TrassarV3 - Log zdarzen na karte SD
// v2.21.0 - Zapis zdarzen systemowych do /logs/RRRRMMDD.log
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

    // Guard rekurencji — jesli operacja SD zawiedzie i error handler
    // probuje zalogowac blad, unikamy nieskonczonej rekurencji → WDT reset
    static bool logInProgress = false;
    if (logInProgress) return;

    if (!ready || !reportLogger.isReady()) return;

    logInProgress = true;

    DateTime now = rtcModule.now();

    // Sciezka: /logs/RRRRMMDD.log
    char path[28];
    snprintf(path, sizeof(path), "/logs/%04d%02d%02d.log",
             now.year(), now.month(), now.day());

    // Zapis linii: HH:MM:SS [KAT] wiadomosc
    char line[200];
    snprintf(line, sizeof(line), "%02d:%02d:%02d [%s] %s\n",
             now.hour(), now.minute(), now.second(),
             category, message);

    if (!SD_LOCK()) { logInProgress = false; return; }

    if (!SD.exists("/logs")) {
        if (!SD.mkdir("/logs")) {
            SD_UNLOCK();
            logInProgress = false;
            return;
        }
    }

    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        SD_UNLOCK();
        logInProgress = false;
        return;
    }

    if (f.size() > EVENT_LOG_MAX_SIZE) {
        f.close();
        // Rotacja: usun stary .old, przemianuj biezacy na .old
        char oldPath[32];
        snprintf(oldPath, sizeof(oldPath), "/logs/%04d%02d%02d.old",
                 now.year(), now.month(), now.day());
        SD.remove(oldPath);
        SD.rename(path, oldPath);
        // Otworz nowy plik
        f = SD.open(path, FILE_WRITE);
        if (!f) {
            SD_UNLOCK();
            logInProgress = false;
            return;
        }
        f.println("--- Log rotated (64KB limit) ---");
    }

    size_t written = f.print(line);
    f.close();
    SD_UNLOCK();

    logInProgress = false;

    if (written == 0) {
        Serial.printf("[WARN][LOG] Blad zapisu do %s\n", path);
    }
}

void EventLog::logf(const char* category, const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log(category, buf);
}
