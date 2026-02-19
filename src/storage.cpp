// ============================================================
// TrassarV3 - Trwały zapis do NVS (Preferences)
// ============================================================

#include "storage.h"
#include <Preferences.h>

StorageManager storage;

static Preferences prefs;

void StorageManager::begin() {
    Serial.println("[NVS] Inicjalizacja pamieci trwalej");
    checkNvsVersion();
}

void StorageManager::checkNvsVersion() {
    prefs.begin("trassar", false);
    uint8_t ver = prefs.getUChar("nvs_ver", 0);
    if (ver != NVS_DATA_VERSION) {
        Serial.printf("[NVS] Wersja NVS: %d -> %d (migracja)\n", ver, NVS_DATA_VERSION);
        // Kasuj dane niekompatybilne ze starszymi wersjami
        if (ver < 2) {
            // v1->v2: zmiana CustomPatternCfg (lineLen/gapLen -> tablice per-gun)
            // + dodanie slotow + gun shot counts
            prefs.remove("cust_pat");   // Stary jednosotowy wzorzec
            prefs.remove("cust_p0");
            prefs.remove("cust_p1");
            prefs.remove("cust_p2");
            Serial.println("[NVS] Wyczyszczono wzorce wlasne (zmiana formatu)");
        }
        prefs.putUChar("nvs_ver", NVS_DATA_VERSION);
    } else {
        Serial.printf("[NVS] Wersja NVS: %d (OK)\n", ver);
    }
    prefs.end();
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

void StorageManager::saveMode(MachineMode mode) {
    prefs.begin("trassar", false);
    prefs.putUChar("mode", (uint8_t)mode);
    prefs.end();
    Serial.printf("[NVS] Zapisano tryb: %d\n", mode);
}

MachineMode StorageManager::loadMode() {
    prefs.begin("trassar", true);
    uint8_t val = prefs.getUChar("mode", 0);
    prefs.end();
    if (val > MODE_MANUAL) val = 0;
    return (MachineMode)val;
}

void StorageManager::saveCustomPattern(const CustomPatternCfg& cfg, int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
    char key[12];
    snprintf(key, sizeof(key), "cust_p%d", slot);
    prefs.begin("trassar", false);
    prefs.putBytes(key, &cfg, sizeof(cfg));
    prefs.end();
    Serial.printf("[NVS] Zapisano wzorzec wlasny slot %d\n", slot);
}

CustomPatternCfg StorageManager::loadCustomPattern(int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
    CustomPatternCfg cfg = {};
    char key[12];
    snprintf(key, sizeof(key), "cust_p%d", slot);
    prefs.begin("trassar", true);
    size_t len = prefs.getBytes(key, &cfg, sizeof(cfg));
    prefs.end();
    if (len != sizeof(cfg)) {
        cfg.valid = false;
    }
    return cfg;
}

void StorageManager::saveGunShotCounts(const uint32_t counts[NUM_GUNS]) {
    prefs.begin("trassar", false);
    prefs.putBytes("gun_shots", counts, sizeof(uint32_t) * NUM_GUNS);
    prefs.end();
}

void StorageManager::loadGunShotCounts(uint32_t counts[NUM_GUNS]) {
    prefs.begin("trassar", true);
    size_t len = prefs.getBytes("gun_shots", counts, sizeof(uint32_t) * NUM_GUNS);
    prefs.end();
    if (len != sizeof(uint32_t) * NUM_GUNS) {
        for (int i = 0; i < NUM_GUNS; i++) counts[i] = 0;
    }
}

void StorageManager::saveSwitchMode(bool smart) {
    prefs.begin("trassar", false);
    prefs.putBool("sw_smart", smart);
    prefs.end();
    Serial.printf("[NVS] Tryb przelaczania: %s\n", smart ? "SMART" : "INSTANT");
}

bool StorageManager::loadSwitchMode() {
    prefs.begin("trassar", true);
    bool val = prefs.getBool("sw_smart", true);  // Domyslnie smart
    prefs.end();
    return val;
}
