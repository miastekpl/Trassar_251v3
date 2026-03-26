#include "sys_log.h"
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
    DBG_PRINTLN("[LOG] Event log gotowy");
}

void EventLog::log(const char* category, const char* message) {
    // Zawsze drukuj na Serial niezaleznie od SD
    DBG_PRINTF("[%s] %s\n", category, message);

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
        DBG_PRINTF("[WARN][LOG] Blad zapisu do %s\n", path);
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

void EventLog::cleanupOldLogs() {
    // Wywolywane periodycznie — usuwanie logow starszych niz 7 dni
    // Zabezpieczenie: max raz na 10 minut
    unsigned long now = millis();
    if (lastCleanupMs != 0 && (now - lastCleanupMs < 600000UL)) return;
    lastCleanupMs = now;

    if (!ready || !reportLogger.isReady()) return;
    if (!SD_LOCK()) return;

    if (!SD.exists("/logs")) {
        SD_UNLOCK();
        return;
    }

    // Zbierz liste plikow i usun najstarsze jesli > EVENT_LOG_MAX_FILES
    File dir = SD.open("/logs");
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        SD_UNLOCK();
        return;
    }

    // Prosta strategia: zlicz pliki, jesli > limit usun najstarsze
    // Nazwy plikow to RRRRMMDD.log/.old — sortowanie leksykograficzne = chronologiczne
    struct LogFile {
        char name[20];
    };
    LogFile files[32];
    int count = 0;

    File entry = dir.openNextFile();
    while (entry && count < 32) {
        if (!entry.isDirectory()) {
            const char* n = entry.name();
            if (n && strlen(n) > 0 && strlen(n) < sizeof(files[0].name)) {
                strncpy(files[count].name, n, sizeof(files[0].name) - 1);
                files[count].name[sizeof(files[0].name) - 1] = '\0';
                count++;
            }
        }
        entry.close();
        // Fix #20: yield co 10 plikow — zapobiega kumulacji czasu SD przy wolnej karcie
        if (count % 10 == 0) yield();
        entry = dir.openNextFile();
    }
    dir.close();

    if (count <= EVENT_LOG_MAX_FILES) {
        SD_UNLOCK();
        return;
    }

    // Sortuj leksykograficznie (= chronologicznie dzieki formatowi RRRRMMDD)
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(files[i].name, files[j].name) > 0) {
                LogFile tmp = files[i];
                files[i] = files[j];
                files[j] = tmp;
            }
        }
    }

    // Usun najstarsze pliki az zostanie <= EVENT_LOG_MAX_FILES
    int toDelete = count - EVENT_LOG_MAX_FILES;
    for (int i = 0; i < toDelete; i++) {
        char path[40];
        snprintf(path, sizeof(path), "/logs/%s", files[i].name);
        SD.remove(path);
        DBG_PRINTF("[LOG] Usuwam stary log: %s\n", path);
    }

    SD_UNLOCK();
}
