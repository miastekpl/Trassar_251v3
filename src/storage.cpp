// ============================================================
// TrassarV3 - Trwały zapis do NVS (Preferences)
// ============================================================

#include "storage.h"
#include "sys_log.h"
#include <Preferences.h>

StorageManager storage;

static Preferences prefs;
static bool nvsOpen = false;
static bool nvsReadOnly = false;

// RAII helper — otwiera NVS jesli nie jest juz otwarty
// Zamyka w destruktorze jesli sam otwarl
struct NvsSession {
    bool owned;
    NvsSession(bool readOnly) {
        if (nvsOpen) {
            // Juz otwarty (batch mode) — nie zamykaj
            // Jesli batch jest readOnly a my chcemy write → reopen
            if (!readOnly && nvsReadOnly) {
                prefs.end();
                prefs.begin("trassar", false);
                nvsReadOnly = false;
            }
            owned = false;
        } else {
            prefs.begin("trassar", readOnly);
            nvsOpen = true;
            nvsReadOnly = readOnly;
            owned = true;
        }
    }
    ~NvsSession() {
        if (owned) {
            prefs.end();
            nvsOpen = false;
        }
    }
};

void StorageManager::begin() {
    LOG_INFO("NVS", "Inicjalizacja pamieci trwalej");
    checkNvsVersion();

    // Weryfikacja checksumu NVS
    if (!verifyChecksum()) {
        LOG_WARN("NVS", "Checksum NVS nieprawidlowy — dane moga byc uszkodzone");
    } else {
        LOG_INFO("NVS", "Checksum NVS OK");
    }
}

void StorageManager::checkNvsVersion() {
    NvsSession s(false);
    uint8_t ver = prefs.getUChar("nvs_ver", 0);
    if (ver != NVS_DATA_VERSION) {
        Serial.printf("[NVS] Wersja NVS: %d -> %d (migracja)\n", ver, NVS_DATA_VERSION);
        if (ver < 2) {
            prefs.remove("cust_pat");
            prefs.remove("cust_p0");
            prefs.remove("cust_p1");
            prefs.remove("cust_p2");
            Serial.println("[NVS] Wyczyszczono wzorce wlasne (zmiana formatu)");
        }
        if (ver < 5) {
            // v5: CustomPatternCfg ma nowe pole structVersion — stare bloby
            // maja inny rozmiar/layout, loadCustomPattern() je wykryje i zignoruje
            Serial.println("[NVS] Migracja v5: wzorce wlasne beda zwalidowane przy ladowaniu");
        }
        prefs.putUChar("nvs_ver", NVS_DATA_VERSION);
    } else {
        Serial.printf("[NVS] Wersja NVS: %d (OK)\n", ver);
    }
}

void StorageManager::saveCalibration(float pulsesPerMeter) {
    NvsSession s(false);
    prefs.putFloat("cal_ppm", pulsesPerMeter);
    prefs.putBool("cal_done", true);
    LOG_INFO("NVS", "Zapisano kalibracje: %.1f imp/m", pulsesPerMeter);
    updateChecksum();
}

float StorageManager::loadCalibration(bool& calibrated) {
    NvsSession s(true);
    calibrated = prefs.getBool("cal_done", false);
    return prefs.getFloat("cal_ppm", DEFAULT_PULSES_PER_METER);
}

void StorageManager::saveLifetimeStats(const LifetimeStats& st) {
    NvsSession s(false);
    prefs.putFloat("lt_dist", st.totalDistance);
    prefs.putFloat("lt_area", st.totalArea);
    prefs.putUInt("lt_time", st.totalPaintTimeSec);
    updateChecksum();
}

LifetimeStats StorageManager::loadLifetimeStats() {
    NvsSession s(true);
    LifetimeStats st;
    st.totalDistance = prefs.getFloat("lt_dist", 0);
    st.totalArea = prefs.getFloat("lt_area", 0);
    st.totalPaintTimeSec = prefs.getUInt("lt_time", 0);
    return st;
}

void StorageManager::saveLastPattern(PatternID pat) {
    NvsSession s(false);
    prefs.putUChar("last_pat", (uint8_t)pat);
}

PatternID StorageManager::loadLastPattern() {
    NvsSession s(true);
    uint8_t val = prefs.getUChar("last_pat", 0);
    if (val >= PAT_COUNT) val = 0;
    return (PatternID)val;
}

void StorageManager::saveMaxSpeed(float kmh) {
    NvsSession s(false);
    prefs.putFloat("max_spd", kmh);
    Serial.printf("[NVS] Zapisano max predkosc: %.1f km/h\n", kmh);
}

float StorageManager::loadMaxSpeed() {
    NvsSession s(true);
    return prefs.getFloat("max_spd", DEFAULT_MAX_PAINT_SPEED_KMH);
}

void StorageManager::saveMinSpeed(float kmh) {
    NvsSession s(false);
    prefs.putFloat("min_spd", kmh);
    Serial.printf("[NVS] Zapisano min predkosc: %.1f km/h\n", kmh);
}

float StorageManager::loadMinSpeed() {
    NvsSession s(true);
    return prefs.getFloat("min_spd", DEFAULT_MIN_PAINT_SPEED_KMH);
}

void StorageManager::saveMode(MachineMode mode) {
    NvsSession s(false);
    prefs.putUChar("mode", (uint8_t)mode);
    Serial.printf("[NVS] Zapisano tryb: %d\n", mode);
}

MachineMode StorageManager::loadMode() {
    NvsSession s(true);
    uint8_t val = prefs.getUChar("mode", 0);
    if (val > MODE_MANUAL) val = 0;
    return (MachineMode)val;
}

void StorageManager::saveCustomPattern(const CustomPatternCfg& cfg, int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
    char key[12];
    snprintf(key, sizeof(key), "cust_p%d", slot);
    // Ustaw wersje struktury przed zapisem
    CustomPatternCfg versioned = cfg;
    versioned.structVersion = CUSTOM_PAT_STRUCT_VER;
    NvsSession s(false);
    prefs.putBytes(key, &versioned, sizeof(versioned));
    Serial.printf("[NVS] Zapisano wzorzec wlasny slot %d (ver=%d)\n", slot, CUSTOM_PAT_STRUCT_VER);
}

CustomPatternCfg StorageManager::loadCustomPattern(int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
    CustomPatternCfg cfg = {};
    char key[12];
    snprintf(key, sizeof(key), "cust_p%d", slot);
    NvsSession s(true);
    size_t len = prefs.getBytes(key, &cfg, sizeof(cfg));
    if (len != sizeof(cfg) || cfg.structVersion != CUSTOM_PAT_STRUCT_VER) {
        // Rozmiar lub wersja nie pasuje — blob pochodzi ze starszego firmware
        if (len > 0) {
            LOG_WARN("NVS", "Slot %d: niezgodna wersja struktury (len=%u ver=%u) — invalidated",
                     slot, (unsigned)len, cfg.structVersion);
        }
        cfg = {};
        cfg.valid = false;
    }
    return cfg;
}

void StorageManager::saveGunShotCounts(const uint32_t counts[NUM_GUNS]) {
    NvsSession s(false);
    prefs.putBytes("gun_shots", counts, sizeof(uint32_t) * NUM_GUNS);
}

void StorageManager::loadGunShotCounts(uint32_t counts[NUM_GUNS]) {
    NvsSession s(true);
    size_t len = prefs.getBytes("gun_shots", counts, sizeof(uint32_t) * NUM_GUNS);
    if (len != sizeof(uint32_t) * NUM_GUNS) {
        for (int i = 0; i < NUM_GUNS; i++) counts[i] = 0;
    }
}

void StorageManager::saveSwitchMode(bool smart) {
    NvsSession s(false);
    prefs.putBool("sw_smart", smart);
    Serial.printf("[NVS] Tryb przelaczania: %s\n", smart ? "SMART" : "INSTANT");
}

bool StorageManager::loadSwitchMode() {
    NvsSession s(true);
    return prefs.getBool("sw_smart", true);
}

void StorageManager::saveMTH(uint32_t totalSec) {
    NvsSession s(false);
    prefs.putUInt("mth_sec", totalSec);
}

uint32_t StorageManager::loadMTH() {
    NvsSession s(true);
    return prefs.getUInt("mth_sec", 0);
}

void StorageManager::saveNightMode(bool enabled) {
    NvsSession s(false);
    prefs.putBool("night_mode", enabled);
}

bool StorageManager::loadNightMode() {
    NvsSession s(true);
    return prefs.getBool("night_mode", false);
}

void StorageManager::saveTankCapacity(float liters) {
    NvsSession s(false);
    prefs.putFloat("tank_cap", liters);
    Serial.printf("[NVS] Zapisano pojemnosc zbiornika: %.0f L\n", liters);
}

float StorageManager::loadTankCapacity() {
    NvsSession s(true);
    return prefs.getFloat("tank_cap", 200.0f);
}

void StorageManager::saveConsumptionRate(float lPerM2) {
    NvsSession s(false);
    prefs.putFloat("cons_rate", lPerM2);
    Serial.printf("[NVS] Zapisano zuzycie farby: %.2f l/m2\n", lPerM2);
}

float StorageManager::loadConsumptionRate() {
    NvsSession s(true);
    return prefs.getFloat("cons_rate", 0.60f);
}

void StorageManager::saveAutoResume(bool enabled) {
    NvsSession s(false);
    prefs.putBool("auto_res", enabled);
    Serial.printf("[NVS] Auto-resume: %s\n", enabled ? "ON" : "OFF");
}

bool StorageManager::loadAutoResume() {
    NvsSession s(true);
    return prefs.getBool("auto_res", true);
}

void StorageManager::resetAllExceptCalibration() {
    // Zachowaj kalibracje i MTH przed czyszczeniem
    bool calibrated;
    float ppm = loadCalibration(calibrated);
    uint32_t mth = loadMTH();

    // Wyczysc cala przestrzen NVS
    NvsSession s(false);
    prefs.clear();

    // Przywroc wersje NVS, kalibracje i MTH
    prefs.putUChar("nvs_ver", NVS_DATA_VERSION);
    if (calibrated) {
        prefs.putFloat("cal_ppm", ppm);
        prefs.putBool("cal_done", true);
    }
    prefs.putUInt("mth_sec", mth);
    LOG_INFO("NVS", "Reset wszystkich danych (kalibracja + MTH zachowane)");

    updateChecksum();
}

// ============================================================
// Factory reset — usuwa WSZYSTKIE dane NVS
// ============================================================
void StorageManager::factoryReset() {
    NvsSession s(false);
    prefs.clear();
    prefs.putUChar("nvs_ver", NVS_DATA_VERSION);
    LOG_WARN("NVS", "FACTORY RESET — wszystkie dane usuniete");
    updateChecksum();
}

// ============================================================
// NVS Checksum — prosty CRC32 kluczowych wartosci
// ============================================================
// Wewnetrzna funkcja hashujaca — wymaga otwartej sesji NVS (nie otwiera wlasnej)
static uint32_t computeChecksumInternal() {
    // Prosty FNV-1a hash kluczowych danych NVS
    uint32_t hash = 2166136261u;  // FNV offset basis
    auto hashByte = [&hash](uint8_t b) {
        hash ^= b;
        hash *= 16777619u;  // FNV prime
    };
    auto hashFloat = [&hashByte](float f) {
        uint8_t* p = (uint8_t*)&f;
        for (int i = 0; i < 4; i++) hashByte(p[i]);
    };
    auto hashU32 = [&hashByte](uint32_t v) {
        uint8_t* p = (uint8_t*)&v;
        for (int i = 0; i < 4; i++) hashByte(p[i]);
    };

    // Kalibracja
    hashFloat(prefs.getFloat("cal_ppm", 0));
    hashByte(prefs.getBool("cal_done", false) ? 1 : 0);

    // Lifetime stats
    hashFloat(prefs.getFloat("lt_dist", 0));
    hashFloat(prefs.getFloat("lt_area", 0));
    hashU32(prefs.getUInt("lt_time", 0));

    // Ustawienia
    hashByte(prefs.getUChar("last_pat", 0));
    hashByte(prefs.getUChar("mode", 0));
    hashFloat(prefs.getFloat("max_spd", 0));
    hashFloat(prefs.getFloat("min_spd", 0));

    // MTH
    hashU32(prefs.getUInt("mth_sec", 0));

    return hash;
}

uint32_t StorageManager::computeChecksum() {
    NvsSession s(true);
    return computeChecksumInternal();
}

bool StorageManager::verifyChecksum() {
    NvsSession s(true);
    uint32_t stored = prefs.getUInt("nvs_crc", 0);
    if (stored == 0) {
        // Brak checksumu (stara wersja NVS) — wygeneruj
        updateChecksum();
        return true;
    }
    uint32_t computed = computeChecksumInternal();
    return (stored == computed);
}

void StorageManager::updateChecksum() {
    // Jedna sesja read-write: oblicz hash i zapisz — unika zagniezdzonego
    // prefs.end()/prefs.begin() ktore mogloby utracic niezapisane dane
    NvsSession s(false);
    uint32_t crc = computeChecksumInternal();
    prefs.putUInt("nvs_crc", crc);
}
