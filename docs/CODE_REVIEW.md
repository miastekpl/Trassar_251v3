# TrassarV3 — Raport weryfikacji kodu v2.23.0

## Podsumowanie

Kod źródłowy (~9 600 linii, 45+ plików) jest dobrze napisany, spójny architektonicznie i gotowy do produkcji. Zidentyfikowano **20 potencjalnych problemów**, z czego **3 o wysokiej**, **9 średniej** i **8 niskiej** ważności.

**Ogólna ocena:** Gotowy do pracy na dedykowanych maszynach. Zalecane naprawienie problemów wysokiej ważności przed wdrożeniem w scenariuszach z intensywnym współbieżnym dostępem.

---

## Mocne strony architektury

- Architektura dual-core: WiFi non-blocking (Core 0) + czujniki real-time (Core 1)
- Szybki ISR enkodera: bezpośredni odczyt rejestru GPIO (~50 ns vs ~2 µs z digitalRead)
- Atomowy dostęp do stanu: FreeRTOS spinlock między rdzeniami
- Bogata biblioteka wzorców: 15 predefiniowanych + 1 własny z 3 slotami
- Kompleksowe logowanie: logi zdarzeń, raporty sesji, eksport statystyk
- Graceful degradation: POST sprawdza peryferia, ostrzega przy błędach
- Bezpieczeństwo: watchdog 3 s + gun keepalive 300 ms (dwie warstwy)
- Konfiguracja z przeglądarki: WiFi AP + panel HTML (bez instalowania aplikacji)

---

## Znalezione problemy

### Priorytet WYSOKI (3)

#### 1. SPI współdzielone bez pełnej ochrony mutex

**Lokalizacja:** main.cpp, display_manager.cpp, report_logger.cpp

**Problem:** Karta SD i wyświetlacz TFT współdzielą magistralę HSPI (GPIO 11/12/13). Core 1 może uzyskiwać dostęp do SD podczas gdy Core 0 renderuje TFT — potencjalna kolizja na magistrali SPI.

**Wpływ:** Przekłamania na wyświetlaczu, błędy zapisu SD.

**Zalecenie:** Upewnić się, że WSZYSTKIE operacje SD otoczone makrami `SD_LOCK()` / `SD_UNLOCK()`.

---

#### 2. Web API blokujące na mutex SD

**Lokalizacja:** web_server.cpp (handleReports, handleReportDownload)

**Problem:** Żądania HTTP blokują na `SD_LOCK()` z timeoutem 5 sekund. Jeśli operacja SD zawiesi się, broadcast WebSocket na Core 0 zostaje wstrzymany.

**Wpływ:** Panel WWW przestaje odpowiadać.

**Zalecenie:** Zwracać błąd HTTP 503 jeśli `SD_LOCK()` timeout zamiast czekać. Nie blokować Core 0 na operacjach I/O.

---

#### 3. Brak walidacji wejścia na wzorce własne

**Lokalizacja:** web_server.cpp (obsługa `save_custom_pattern`)

**Problem:** Użytkownik może ustawić lineLen/gapLen < 0, co powoduje błąd matematyczny w `shouldGunFire()` (fmod z ujemnym cyklem).

**Wpływ:** Nieprzewidywalne zachowanie pistoletów.

**Zalecenie:** Walidacja: `lineLen > 0 && gapLen > 0` przed zapisem do NVS. Odrzucać ujemne wartości z HTTP 400.

---

### Priorytet ŚREDNI (9)

#### 4. Race condition na `g_state`

**Problem:** Core 0 (web server) modyfikuje `g_state.machineState` z żądań HTTP bez pełnego pokrycia `STATE_LOCK()`. Możliwa niespójność stanu.

**Zalecenie:** Owinąć WSZYSTKIE modyfikacje `g_state` z Core 0 w `STATE_LOCK()` / `STATE_UNLOCK()`.

---

#### 5. GPIO 46 (joystick SW) jest strap pinem

**Problem:** Jeśli GPIO 46 jest LOW przy starcie ESP32-S3, może wpływać na tryb bootowania.

**Zalecenie:** Dokumentacja już ostrzega. Rozważyć dodanie opóźnienia w `joystick.begin()` po starcie.

---

#### 6. Brak thread-safety na `GunController::gunStates[]`

**Problem:** `guns.setGun()` wywoływane z Core 1, stan pistoletów czytany z Core 0 (API status JSON) bez synchronizacji.

**Zalecenie:** Dodać spinlock na tablicy `gunStates[]`.

---

#### 7. ISR enkodera potencjalnie agresywny

**Problem:** ISR wyzwalany na każdym CHANGE zarówno CLK jak i DT. Debounce 200 µs pomaga, ale przy wysokich prędkościach mogą być gubione impulsy.

**Zalecenie:** Zweryfikować dokładność pomiaru przy maksymalnej oczekiwanej prędkości (50 km/h).

---

#### 8. Szum ADC na joysticku

**Problem:** Osie analogowe (GPIO 19/20) podatne na szum. Kod blokuje zdarzenia osi na ekranach operacyjnych, ale sporadycznie mogą generować fałszywe `EVT_STOP_LONG`.

**Zalecenie:** Rozważyć histerezę programową w `joystick.update()`.

---

#### 9. NVS wzorca własnego bez wersjonowania struktury

**Problem:** `CustomPatternCfg` zapisywany jako blob binarny. Zmiana rozmiaru struktury w aktualizacji firmware spowoduje odczyt śmieci.

**Zalecenie:** Dodać wersjonowanie wewnątrz bloba wzorca lub migrację przy `NVS_DATA_VERSION`.

---

#### 10. Race condition przy przełączaniu wzorca

**Problem:** Flaga `patternChangePending` ustawiana z eventów webowych/przyciskowych, sprawdzana w `update()` na Core 1. Zmiana wzorca w połowie cyklu może użyć starej długości cyklu.

**Zalecenie:** Atomowa aktualizacja metadanych wzorca wewnątrz `STATE_LOCK()`.

---

#### 11. Rotacja logów zdarzeń tylko dziennie

**Problem:** Log rotuje dziennie, ale nie po rozmiarze. Maszyna logująca co 10 s przekroczy 64 KB w ~18 godzin.

**Zalecenie:** Sprawdzać rozmiar pliku i zamykać/rotować jeśli przekracza `EVENT_LOG_MAX_SIZE`.

---

#### 12. Watchdog nie izolowany per rdzeń

**Problem:** Oba rdzenie współdzielą ten sam WDT. Jeśli Core 1 resetuje WDT regularnie, ale Core 0 (WiFi) zawiesza się — WDT nie zadziała.

**Zalecenie:** Oddzielna konfiguracja WDT per rdzeń lub task-specific WDT.

---

### Priorytet NISKI (8)

| # | Problem | Opis |
|---|---------|------|
| 13 | WDT opóźniony na Core 0 | 5 s opóźnienie przed rejestracją WDT na Core 0 |
| 14 | NvsSession nie zawsze zamknięta | Zagnieżdżone NvsSession z różnymi flagami readOnly |
| 15 | Wyciek pamięci WebSocket | `wsServer.broadcastTXT()` alokuje na heapie; fragmentacja przy długiej pracy |
| 16 | GPS fix age za krótki | `hasFix()` wymaga < 3 s, ale GPX zapisuje co 5 s — mogą być luki |
| 17 | TFT init bez weryfikacji | `tft.init()` nie zwraca statusu — brak obsługi błędu |
| 18 | POST timeout niekonfigurowalny | 5 s hardcoded — problematyczne przy testach fabrycznych |
| 19 | Kolory UI hardcoded | 16-bitowe stałe kolorów bez walidacji |
| 20 | NVS silent failure | `prefs.getFloat()` zwraca domyślną wartość bez logowania błędu |

---

## Statystyki kodu

| Metryka | Wartość |
|---------|---------|
| Łączna liczba linii | ~9 600 |
| Pliki źródłowe (.cpp) | 26 |
| Pliki nagłówkowe (.h) | 26 |
| Enumy | 8 (MachineState, MachineMode, ScreenID, PatternID, GunID, GunMode, ButtonEvent, BuzzerSignal) |
| Ekrany interfejsu | 15 |
| Wzorce malowania | 16 (15 predef. + 1 własny) |
| Endpointy API | 11 |
| Sygnały buzzera | 10 |
| Timery w loop() | 12 |

---

## Rekomendowane naprawy (kolejność)

1. **Walidacja wzorca własnego** — lineLen/gapLen > 0 (1h)
2. **SD_LOCK na web API** — timeout → HTTP 503 zamiast blokady (2h)
3. **STATE_LOCK w web_server** — pełne pokrycie mutexem (2h)
4. **GunController spinlock** — thread-safety na stanie pistoletów (1h)
5. **NVS wersjonowanie blobów** — migracja CustomPatternCfg (2h)

---

*Raport wygenerowany na podstawie analizy kodu v2.23.0*
*Platforma: ESP32-S3 N16R8 | Framework: Arduino/PlatformIO*
