// ============================================================
// TrassarV3 - Fizyczne przyciski wzorców via MCP23017
// 15 przycisków na expanderze I2C → 15 wzorców P-1a...P-7d
//
// MCP23017 na I2C (SDA=GPIO17, SCL=GPIO18) — ta sama magistrala
// co RTC DS1307. Adres 0x20 (A0=A1=A2=GND).
//
// Mapowanie pinów MCP23017:
//   GPA0 = P-1a    GPA4 = P-1e    GPB0 = P-3b    GPB4 = P-7b
//   GPA1 = P-1b    GPA5 = P-2a    GPB1 = P-4     GPB5 = P-7c
//   GPA2 = P-1c    GPA6 = P-2b    GPB2 = P-6     GPB6 = P-7d
//   GPA3 = P-1d    GPA7 = P-3a    GPB3 = P-7a
//
// Przyciski podłączone: pin MCP → GND (wewnętrzne pull-up)
// Aktywny stan: LOW (0) = wciśnięty
// ============================================================

#include "pattern_buttons.h"
#include "patterns.h"
#include "painting_engine.h"
#include "buzzer.h"
#include "storage.h"
#include "event_log.h"
#include "sys_log.h"
#include <Wire.h>

// Licznik bledow I2C — po przekroczeniu progu wyłacza modul
static uint8_t i2cErrorCount = 0;
static const uint8_t I2C_ERROR_THRESHOLD = 15;  // 15 bledow z rzedu = modul offline (EMI od pomp)

PatternButtonHandler patternButtons;

// ============ Rejestry MCP23017 (IOCON.BANK=0, domyślne) ============
static const uint8_t MCP_IODIRA  = 0x00;  // Kierunek portu A (1=input)
static const uint8_t MCP_IODIRB  = 0x01;  // Kierunek portu B
static const uint8_t MCP_GPPUA   = 0x0C;  // Pull-up portu A
static const uint8_t MCP_GPPUB   = 0x0D;  // Pull-up portu B
static const uint8_t MCP_GPIOA   = 0x12;  // Odczyt portu A
static const uint8_t MCP_GPIOB   = 0x13;  // Odczyt portu B

// ============ Mapowanie bit → wzorzec ============
// Bit 0..7 = GPA0..GPA7, Bit 8..15 = GPB0..GPB7
PatternID PatternButtonHandler::bitToPattern(uint8_t bit) {
    static const PatternID map[15] = {
        PAT_P1A,   // bit 0  = GPA0
        PAT_P1B,   // bit 1  = GPA1
        PAT_P1C,   // bit 2  = GPA2
        PAT_P1D,   // bit 3  = GPA3
        PAT_P1E,   // bit 4  = GPA4
        PAT_P2A,   // bit 5  = GPA5
        PAT_P2B,   // bit 6  = GPA6
        PAT_P3A,   // bit 7  = GPA7
        PAT_P3B,   // bit 8  = GPB0
        PAT_P4,    // bit 9  = GPB1
        PAT_P6,    // bit 10 = GPB2
        PAT_P7A,   // bit 11 = GPB3
        PAT_P7B,   // bit 12 = GPB4
        PAT_P7C,   // bit 13 = GPB5
        PAT_P7D    // bit 14 = GPB6
    };
    if (bit < 15) return map[bit];
    return PAT_P1A;  // fallback
}

// ============ I2C helpers ============

bool PatternButtonHandler::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

uint8_t PatternButtonHandler::readReg(uint8_t reg) {
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    Wire.write(reg);
    uint8_t err = Wire.endTransmission(false);
    if (err != 0) {
        i2cErrorCount++;
        if (i2cErrorCount >= I2C_ERROR_THRESHOLD && ready) {
            ready = false;
            LOG_ERROR("PAT_BTN", "MCP23017 odlaczony (I2C err=%d, %d bledow)", err, i2cErrorCount);
            eventLog.logf("PAT_BTN", "MCP23017 offline — I2C err=%d", err);
        }
        return 0xFF;
    }
    Wire.requestFrom((uint8_t)MCP23017_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        i2cErrorCount = 0;  // Reset licznika bledow przy udanym odczycie
        return Wire.read();
    }
    i2cErrorCount++;
    return 0xFF;
}

uint16_t PatternButtonHandler::readAllPins() {
    uint8_t a = readReg(MCP_GPIOA);
    if (!ready) return 0xFFFF;  // I2C offline — zwroc "wszystkie zwolnione"
    uint8_t b = readReg(MCP_GPIOB);
    if (!ready) return 0xFFFF;
    return ((uint16_t)b << 8) | a;
}

// ============ Inicjalizacja ============

void PatternButtonHandler::begin() {
    // Wire juz zainicjalizowany przez RTC (begin w rtc_handler)
    // Sprawdz czy MCP23017 odpowiada na I2C
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    uint8_t err = Wire.endTransmission();

    if (err != 0) {
        DBG_PRINTF("[PAT_BTN] MCP23017 niedostepny na 0x%02X (err=%d)\n",
                      MCP23017_I2C_ADDR, err);
        ready = false;
        return;
    }

    // Konfiguracja portów A i B jako wejścia (0xFF = wszystkie input)
    writeReg(MCP_IODIRA, 0xFF);
    writeReg(MCP_IODIRB, 0xFF);

    // Włączenie wewnętrznych pull-up na obu portach
    writeReg(MCP_GPPUA, 0xFF);
    writeReg(MCP_GPPUB, 0x7F);  // GPB0-GPB6 pull-up (GPB7 nieużywany)

    // Pierwszy odczyt — stabilizacja
    lastRaw = readAllPins();
    debouncedState = lastRaw;
    lastDebouncedState = lastRaw;
    lastReadMs = millis();

    ready = true;
    DBG_PRINTF("[PAT_BTN] MCP23017 OK na 0x%02X — 15 przyciskow wzorcow\n",
                  MCP23017_I2C_ADDR);
}

// ============ Cykliczna aktualizacja ============

void PatternButtonHandler::update() {
    if (!ready) {
        // Proba reconnectu co 5s
        unsigned long now = millis();
        if (now - lastReadMs < 5000) return;
        lastReadMs = now;
        Wire.beginTransmission(MCP23017_I2C_ADDR);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            ready = true;
            i2cErrorCount = 0;
            LOG_INFO("PAT_BTN", "MCP23017 ponownie dostepny na 0x%02X", MCP23017_I2C_ADDR);
            eventLog.log("PAT_BTN", "MCP23017 reconnected");
            // Reinicjalizacja odczytu
            lastRaw = readAllPins();
            debouncedState = lastRaw;
            lastDebouncedState = lastRaw;
        }
        return;
    }

    unsigned long now = millis();
    if (now - lastReadMs < MCP23017_SCAN_MS) return;
    lastReadMs = now;

    // Odczyt 16 bitów (PortA + PortB)
    uint16_t raw = readAllPins();

    // Prosty debounce: akceptuj zmiane tylko jesli dwa kolejne odczyty sa identyczne
    if (raw == lastRaw) {
        // Stabilny odczyt — zaktualizuj stan zdebouncowany
        debouncedState = raw;
    }
    lastRaw = raw;

    // Wykryj zbocze opadające (1→0 = wciśnięcie) dla bitów 0..14
    uint16_t pressed = lastDebouncedState & ~debouncedState;
    lastDebouncedState = debouncedState;

    if (pressed == 0) return;

    // Maska: tylko bity 0..14 (15 przycisków)
    pressed &= MCP23017_BUTTON_MASK;

    // Znajdz pierwszy wciśnięty przycisk (priorytet: najniższy bit)
    for (uint8_t i = 0; i < MCP23017_NUM_BUTTONS; i++) {
        if (pressed & (1 << i)) {
            PatternID pat = bitToPattern(i);

            // Zmień wzorzec (z uwzględnieniem Smart/Instant)
            if (g_state.machineState == STATE_PAINTING) {
                paintEngine.setPattern(pat);
            } else {
                patternMgr.setPattern(pat);
                storage.saveLastPattern(pat);
            }

            g_state.displayNeedsUpdate = true;

            // Potwierdzenie dźwiękowe
            buzzer.beep(BUZ_CONFIRM_FREQ, BUZ_CONFIRM_DURATION_MS);

            const PatternDef& def = patternMgr.getPattern(pat);
            DBG_PRINTF("[PAT_BTN] Przycisk %d → wzorzec %s (%s)\n",
                          i, def.code, def.name);

            eventLog.logf("PAT_BTN", "Przycisk %d -> wzorzec %s", i, def.code);

            break;  // Obsluz tylko jeden przycisk na cykl
        }
    }
}
