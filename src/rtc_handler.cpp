// ============================================================
// TrassarV3 - Implementacja modułu RTC DS1307
// ============================================================

#include "rtc_handler.h"

RTCHandler rtcModule;

bool RTCHandler::begin() {
    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);

    if (!rtc.begin()) {
        Serial.println("[RTC] DS1307 nie znaleziony!");
        rtcOk = false;
        return false;
    }

    if (!rtc.isrunning()) {
        Serial.println("[RTC] DS1307 nie dziala - ustawiam czas kompilacji");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    rtcOk = true;
    currentTime = rtc.now();
    Serial.println("[RTC] DS1307 zainicjalizowany poprawnie");
    return true;
}

void RTCHandler::update() {
    if (!rtcOk) return;

    // Odczytuj RTC co 500ms
    unsigned long now = millis();
    if (now - lastUpdate >= 500) {
        lastUpdate = now;
        DateTime t = rtc.now();
        // Walidacja zakresu — I2C moze zwrocic smieci (rok 2165, data 0)
        if (t.year() >= 2024 && t.year() <= 2035 &&
            t.month() >= 1 && t.month() <= 12 &&
            t.day() >= 1 && t.day() <= 31 &&
            t.hour() <= 23 && t.minute() <= 59 && t.second() <= 59) {
            currentTime = t;
        } else {
            // Smieci z I2C — zachowaj poprzedni czas, loguj raz
            static bool rtcGarbageLogged = false;
            if (!rtcGarbageLogged) {
                Serial.printf("[RTC] WARN: nieprawidlowy odczyt %04d-%02d-%02d %02d:%02d:%02d — ignorowany\n",
                              t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
                rtcGarbageLogged = true;
            }
        }
    }
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
