#pragma once
// ============================================================
// TrassarV3 - Moduł zegara RTC DS1307
// ============================================================

#include <Wire.h>
#include <RTClib.h>
#include "config.h"

class RTCHandler {
public:
    bool begin();
    void update();

    const char* getTimeStr();    // HH:MM:SS
    const char* getDateStr();    // DD.MM.YYYY
    const char* getDateTimeStr(); // DD.MM.YYYY HH:MM:SS

    DateTime now();

    void setTime(int hour, int minute, int second);
    void setDate(int year, int month, int day);

    bool isRunning() const { return rtcOk; }

    // Czy czas jest wiarygodny (poprawny zakres + brak ciagu bledow I2C)
    bool isTimeReliable() const { return timeValid; }

private:
    RTC_DS1307 rtc;
    bool rtcOk = false;
    bool timeValid = false;         // Czy czas przeszedl walidacje
    DateTime currentTime;
    DateTime compileTime;           // Fallback: czas kompilacji firmware
    unsigned long lastUpdate = 0;
    uint16_t consecutiveErrors = 0; // Licznik kolejnych blednych odczytow I2C

    char timeBuf[12];
    char dateBuf[16];
    char dateTimeBuf[32];

    // Sprawdza czy DateTime ma rozsadne wartosci (rok 2024-2035)
    bool isDateTimeValid(const DateTime& t) const;
};

extern RTCHandler rtcModule;
