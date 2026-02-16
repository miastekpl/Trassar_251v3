// ============================================================
// TrassarV3 - Trwały zapis do NVS (Preferences)
// ============================================================

#include "storage.h"
#include <Preferences.h>

StorageManager storage;

static Preferences prefs;

void StorageManager::begin() {
    Serial.println("[NVS] Inicjalizacja pamieci trwalej");
}

void StorageManager::saveCalibration(float pulsesPerMeter) {
    prefs.begin("trassar", false);
    prefs.putFloat("cal_ppm", pulsesPerMeter);
    prefs.putBool("cal_done", true);
    prefs.end();
    Serial.printf("[NVS] Zapisano kalibracje: %.1f imp/m\n", pulsesPerMeter);
}

float StorageManager::loadCalibration(bool& calibrated) {
    prefs.begin("trassar", true);
    calibrated = prefs.getBool("cal_done", false);
    float val = prefs.getFloat("cal_ppm", DEFAULT_PULSES_PER_METER);
    prefs.end();
    return val;
}

void StorageManager::saveLifetimeStats(const LifetimeStats& s) {
    prefs.begin("trassar", false);
    prefs.putFloat("lt_dist", s.totalDistance);
    prefs.putFloat("lt_area", s.totalArea);
    prefs.putUInt("lt_time", s.totalPaintTimeSec);
    prefs.end();
}

LifetimeStats StorageManager::loadLifetimeStats() {
    LifetimeStats s;
    prefs.begin("trassar", true);
    s.totalDistance = prefs.getFloat("lt_dist", 0);
    s.totalArea = prefs.getFloat("lt_area", 0);
    s.totalPaintTimeSec = prefs.getUInt("lt_time", 0);
    prefs.end();
    return s;
}

void StorageManager::saveLastPattern(PatternID pat) {
    prefs.begin("trassar", false);
    prefs.putUChar("last_pat", (uint8_t)pat);
    prefs.end();
}

PatternID StorageManager::loadLastPattern() {
    prefs.begin("trassar", true);
    uint8_t val = prefs.getUChar("last_pat", 0);
    prefs.end();
    if (val >= PAT_COUNT) val = 0;
    return (PatternID)val;
}

void StorageManager::saveMaxSpeed(float kmh) {
    prefs.begin("trassar", false);
    prefs.putFloat("max_spd", kmh);
    prefs.end();
    Serial.printf("[NVS] Zapisano max predkosc: %.1f km/h\n", kmh);
}

float StorageManager::loadMaxSpeed() {
    prefs.begin("trassar", true);
    float val = prefs.getFloat("max_spd", DEFAULT_MAX_PAINT_SPEED_KMH);
    prefs.end();
    return val;
}
