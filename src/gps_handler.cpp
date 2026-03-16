#include "sys_log.h"
// ============================================================
// TrassarV3 - Implementacja GPS NEO-6M
// UART2, 9600 baud, NMEA -> TinyGPS++
// ============================================================

#include "gps_handler.h"

GpsHandler gpsHandler;

static HardwareSerial gpsSerial(2);  // UART2 na ESP32-S3

void GpsHandler::begin() {
    gpsDBG_BEGIN(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    lastDataMs = millis();
    DBG_PRINTLN("[GPS] Modul GPS zainicjalizowany (UART2)");
    DBG_PRINTF("[GPS] Piny: RX=%d TX=%d Baud=%d\n", PIN_GPS_RX, PIN_GPS_TX, GPS_BAUD);
}

void GpsHandler::update() {
    // Limit odczytow per wywolanie — zapobiega blokowaniu loop() przy burst GPS.
    // NEO-6M @ 9600 baud = max ~960 bajtow/s. Przy update() co ~1ms,
    // typowo dostepne 1-2 bajty. 256B = zapas na ~250ms burstow.
    bool gotData = false;
    int maxBytes = 256;
    while (gpsSerial.available() > 0 && maxBytes-- > 0) {
        gps.encode(gpsSerial.read());
        gotData = true;
    }
    if (gotData) {
        lastDataMs = millis();
    }
}
