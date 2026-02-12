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

private:
    RTC_DS1307 rtc;
    bool rtcOk = false;
    DateTime currentTime;
    unsigned long lastUpdate = 0;

    char timeBuf[12];
    char dateBuf[12];
    char dateTimeBuf[24];
};

extern RTCHandler rtcModule;
