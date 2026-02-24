// ============================================================
// TrassarV3 - Backup NVS na karte SD
// v2.20.0 - Serializacja wszystkich ustawien NVS do JSON
//
// Plik: /backup/nvs_backup.json
// Zawartosc: kalibracja, statystyki lifetime, wzorce wlasne,
//            tryb pracy, progi predkosci, liczniki pistoletow
//
// Przy starcie: jesli NVS pusty (nvs_init=false) i backup istnieje
//               → automatyczny restore wszystkich ustawien
// ============================================================

#include "nvs_backup.h"
#include "storage.h"
#include "report_logger.h"
#include "event_log.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>

NvsBackup nvsBackup;

static const char* BACKUP_PATH = "/backup/nvs_backup.json";

// ============================================================
// Sprawdzenie czy NVS ma dane (flaga "nvs_init")
// ============================================================
bool NvsBackup::isNvsPopulated() {
    Preferences prefs;
    prefs.begin("trassar", true);
    bool init = prefs.getBool("nvs_init", false);
    prefs.end();
    return init;
}

void NvsBackup::markNvsPopulated() {
    Preferences prefs;
    prefs.begin("trassar", false);
    prefs.putBool("nvs_init", true);
    prefs.end();
}

// ============================================================
// begin() — sprawdz NVS, ewentualnie przywroc z backupu SD
// ============================================================
void NvsBackup::begin() {
    if (!isNvsPopulated()) {
        nvsWasEmpty = true;
        Serial.println("[BACKUP] NVS pusty/wyczyszczony");

        if (isBackupAvailable()) {
            Serial.println("[BACKUP] Znaleziono backup na SD — przywracanie...");
            if (restoreFromSD()) {
                markNvsPopulated();
                eventLog.log("BACKUP", "Auto-restore NVS z karty SD — OK");
            } else {
                eventLog.log("BACKUP", "Auto-restore NVS z karty SD — BLAD");
            }
        } else {
            Serial.println("[BACKUP] Brak backupu na SD — pierwsze uruchomienie");
            markNvsPopulated();
        }
    } else {
        Serial.println("[BACKUP] NVS OK (dane obecne)");
    }

    lastBackupMs = millis();
}

// ============================================================
// Czy plik backup istnieje na SD
// ============================================================
bool NvsBackup::isBackupAvailable() {
    if (!reportLogger.isReady()) return false;
    return SD.exists(BACKUP_PATH);
}

// ============================================================
// Backup NVS -> JSON -> SD
// ============================================================
bool NvsBackup::backupToSD() {
    if (!reportLogger.isReady()) return false;

    JsonDocument doc;

    // Metadata
    doc["v"] = NVS_DATA_VERSION;
    doc["fw"] = FW_VERSION;

    // Kalibracja
    bool calOk = false;
    float ppm = storage.loadCalibration(calOk);
    doc["cal_ppm"] = serialized(String(ppm, 1));
    doc["cal_ok"] = calOk;

    // Statystyki lifetime
    LifetimeStats lt = storage.loadLifetimeStats();
    doc["lt_dist"] = serialized(String(lt.totalDistance, 1));
    doc["lt_area"] = serialized(String(lt.totalArea, 2));
    doc["lt_time"] = lt.totalPaintTimeSec;

    // Ustawienia
    doc["pat"] = (uint8_t)storage.loadLastPattern();
    doc["spd"] = serialized(String(storage.loadMaxSpeed(), 1));
    doc["mode"] = (uint8_t)storage.loadMode();
    doc["sw"] = storage.loadSwitchMode();

    // Liczniki strzalow pistoletow
    uint32_t shots[NUM_GUNS];
    storage.loadGunShotCounts(shots);
    JsonArray shotsArr = doc["shots"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        shotsArr.add(shots[i]);
    }

    // Wzorce wlasne (3 sloty)
    JsonArray cpArr = doc["cp"].to<JsonArray>();
    for (int s = 0; s < NUM_CUSTOM_SLOTS; s++) {
        CustomPatternCfg cfg = storage.loadCustomPattern(s);
        JsonObject slot = cpArr.add<JsonObject>();
        slot["valid"] = cfg.valid;
        if (cfg.valid) {
            JsonArray gm = slot["gm"].to<JsonArray>();
            JsonArray ln = slot["ln"].to<JsonArray>();
            JsonArray gp = slot["gp"].to<JsonArray>();
            for (int i = 0; i < NUM_GUNS; i++) {
                gm.add(cfg.gunModes[i]);
                ln.add(serialized(String(cfg.lineLen[i], 1)));
                gp.add(serialized(String(cfg.gapLen[i], 1)));
            }
        }
    }

    // Zapis na SD
    if (!SD.exists("/backup")) {
        SD.mkdir("/backup");
    }

    File f = SD.open(BACKUP_PATH, FILE_WRITE);
    if (!f) {
        Serial.println("[BACKUP] Blad otwarcia pliku backup");
        return false;
    }

    size_t written = serializeJson(doc, f);
    f.close();

    if (written > 0) {
        lastBackupMs = millis();
        markNvsPopulated();
        Serial.printf("[BACKUP] Backup NVS zapisany (%u bajtow)\n", written);
        return true;
    }
    return false;
}

// ============================================================
// Restore NVS z JSON na SD
// ============================================================
bool NvsBackup::restoreFromSD() {
    if (!reportLogger.isReady() || !SD.exists(BACKUP_PATH)) return false;

    File f = SD.open(BACKUP_PATH, FILE_READ);
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[BACKUP] Blad parsowania JSON: %s\n", err.c_str());
        return false;
    }

    // Sprawdz wersje
    uint8_t ver = doc["v"] | 0;
    if (ver != NVS_DATA_VERSION) {
        Serial.printf("[BACKUP] Niekompatybilna wersja backupu: %d (oczekiwana: %d)\n",
                      ver, NVS_DATA_VERSION);
        return false;
    }

    // Kalibracja
    if (doc["cal_ok"].as<bool>()) {
        float ppm = doc["cal_ppm"].as<float>();
        if (ppm > 1.0f) {
            storage.saveCalibration(ppm);
        }
    }

    // Statystyki lifetime
    LifetimeStats lt;
    lt.totalDistance = doc["lt_dist"].as<float>();
    lt.totalArea = doc["lt_area"].as<float>();
    lt.totalPaintTimeSec = doc["lt_time"] | (uint32_t)0;
    if (lt.totalDistance > 0 || lt.totalArea > 0 || lt.totalPaintTimeSec > 0) {
        storage.saveLifetimeStats(lt);
    }

    // Ustawienia
    uint8_t pat = doc["pat"] | 0;
    if (pat < PAT_COUNT) {
        storage.saveLastPattern((PatternID)pat);
    }

    float spd = doc["spd"].as<float>();
    if (spd >= 5.0f && spd <= 30.0f) {
        storage.saveMaxSpeed(spd);
    }

    uint8_t mode = doc["mode"] | 0;
    if (mode <= MODE_MANUAL) {
        storage.saveMode((MachineMode)mode);
    }

    storage.saveSwitchMode(doc["sw"] | true);

    // Liczniki strzalow
    JsonArray shotsArr = doc["shots"].as<JsonArray>();
    if (shotsArr.size() == NUM_GUNS) {
        uint32_t shots[NUM_GUNS];
        for (int i = 0; i < NUM_GUNS; i++) {
            shots[i] = shotsArr[i] | (uint32_t)0;
        }
        storage.saveGunShotCounts(shots);
    }

    // Wzorce wlasne
    JsonArray cpArr = doc["cp"].as<JsonArray>();
    for (int s = 0; s < NUM_CUSTOM_SLOTS && s < (int)cpArr.size(); s++) {
        JsonObject slot = cpArr[s];
        if (slot["valid"].as<bool>()) {
            CustomPatternCfg cfg = {};
            cfg.valid = true;
            JsonArray gm = slot["gm"];
            JsonArray ln = slot["ln"];
            JsonArray gp = slot["gp"];
            for (int i = 0; i < NUM_GUNS; i++) {
                cfg.gunModes[i] = gm[i] | (uint8_t)0;
                cfg.lineLen[i] = ln[i].as<float>();
                cfg.gapLen[i] = gp[i].as<float>();
            }
            storage.saveCustomPattern(cfg, s);
        }
    }

    Serial.println("[BACKUP] Restore NVS z SD — wszystkie ustawienia przywrocone");
    return true;
}
