#include "sys_log.h"
// ============================================================
// TrassarV3 - Implementacja modułu RTC DS1307
// ============================================================

#include "rtc_handler.h"

RTCHandler rtcModule;

bool RTCHandler::begin() {
    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);

    // Domyslny czas = kompilacja (fallback gdy RTC niedostepny lub zwraca smieci)
    compileTime = DateTime(F(__DATE__), F(__TIME__));
    currentTime = compileTime;

    if (!rtc.begin()) {
        DBG_PRINTLN("[RTC] DS1307 nie znaleziony!");
        rtcOk = false;
        timeValid = false;
        return false;
    }

    if (!rtc.isrunning()) {
        DBG_PRINTLN("[RTC] DS1307 nie dziala - ustawiam czas kompilacji");
        rtc.adjust(compileTime);
    }

    rtcOk = true;

    // Walidacja pierwszego odczytu — RTC moze zwrocic smieci po utracie baterii
    DateTime t = rtc.now();
    if (isDateTimeValid(t)) {
        currentTime = t;
        timeValid = true;
        DBG_PRINTLN("[RTC] DS1307 zainicjalizowany poprawnie");
    } else {
        DBG_PRINTF("[RTC] WARN: nierozsadny czas %04d-%02d-%02d %02d:%02d:%02d — ustawiam kompilacji\n",
                      t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
        rtc.adjust(compileTime);
        currentTime = compileTime;
        timeValid = true;
    }

    return true;
}

void RTCHandler::update() {
    if (!rtcOk) return;

    // Odczytuj RTC co 500ms
    unsigned long now = millis();
    if (now - lastUpdate >= 500) {
        lastUpdate = now;
        DateTime t = rtc.now();
        if (isDateTimeValid(t)) {
            currentTime = t;
            timeValid = true;
            consecutiveErrors = 0;
        } else {
            consecutiveErrors++;
            // Loguj pierwszy blad i co 60. (co ~30s przy 500ms polling)
            if (consecutiveErrors == 1 || consecutiveErrors % 60 == 0) {
                DBG_PRINTF("[RTC] WARN: nieprawidlowy odczyt #%u: %04d-%02d-%02d %02d:%02d:%02d\n",
                              consecutiveErrors,
                              t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
            }
            // Po 10 kolejnych bledach: RTC prawdopodobnie uszkodzony
            if (consecutiveErrors >= 10 && timeValid) {
                timeValid = false;
                DBG_PRINTLN("[RTC] BLAD: zbyt wiele blednych odczytow — czas niewiarygodny");
            }
        }
    }
}

bool RTCHandler::isDateTimeValid(const DateTime& t) const {
    return (t.year() >= 2024 && t.year() <= 2035 &&
            t.month() >= 1 && t.month() <= 12 &&
            t.day() >= 1 && t.day() <= 31 &&
            t.hour() <= 23 && t.minute() <= 59 && t.second() <= 59);
}

DateTime RTCHandler::now() {
    return currentTime;
}

const char* RTCHandler::getTimeStr() {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
             currentTime.hour(), currentTime.minute(), currentTime.second());
    return timeBuf;
}

const char* RTCHandler::getDateStr() {
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%04d",
             currentTime.day(), currentTime.month(), currentTime.year());
    return dateBuf;
}

const char* RTCHandler::getDateTimeStr() {
    snprintf(dateTimeBuf, sizeof(dateTimeBuf), "%02d.%02d.%04d %02d:%02d:%02d",
             currentTime.day(), currentTime.month(), currentTime.year(),
             currentTime.hour(), currentTime.minute(), currentTime.second());
    return dateTimeBuf;
}

void RTCHandler::setTime(int hour, int minute, int second) {
    if (!rtcOk) return;
    DateTime now = rtc.now();
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, second));
    currentTime = rtc.now();
}

void RTCHandler::setDate(int year, int month, int day) {
    if (!rtcOk) return;
    DateTime now = rtc.now();
    rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    currentTime = rtc.now();
}
