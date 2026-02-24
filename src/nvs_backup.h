#pragma once
// ============================================================
// TrassarV3 - Backup NVS na karte SD
// v2.20.0 - Serializacja ustawien do JSON, auto-restore
//
// Co 30 min: zapis /backup/nvs_backup.json
// Przy starcie: jesli NVS pusty → auto-restore z SD
// ============================================================

#include "config.h"

// Interwal automatycznego backupu [ms] (30 minut)
#define NVS_BACKUP_INTERVAL_MS  1800000UL

class NvsBackup {
public:
    void begin();               // Sprawdz NVS, ewentualnie restore
    bool backupToSD();          // Serializuj NVS -> JSON -> SD
    bool restoreFromSD();       // Odczytaj JSON z SD -> NVS
    bool isBackupAvailable();   // Czy plik backup istnieje na SD

    unsigned long getLastBackupMs() const { return lastBackupMs; }

private:
    unsigned long lastBackupMs = 0;
    bool nvsWasEmpty = false;

    bool isNvsPopulated();      // Czy NVS ma dane (flaga nvs_init)
    void markNvsPopulated();    // Ustaw flage po backup/restore
};

extern NvsBackup nvsBackup;
