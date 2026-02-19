// ============================================================
// TrassarV3 - Implementacja GPS NEO-6M
// UART2, 9600 baud, NMEA -> TinyGPS++
// ============================================================

#include "gps_handler.h"

GpsHandler gpsHandler;

static HardwareSerial gpsSerial(2);  // UART2 na ESP32-S3

void GpsHandler::begin() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    Serial.println("[GPS] Modul GPS zainicjalizowany (UART2)");
    Serial.printf("[GPS] Piny: RX=%d TX=%d Baud=%d\n", PIN_GPS_RX, PIN_GPS_TX, GPS_BAUD);
}

void GpsHandler::update() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
}
