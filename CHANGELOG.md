# Changelog - TrassarV3

Wszystkie istotne zmiany w projekcie są dokumentowane w tym pliku.

Format oparty na [Keep a Changelog](https://keepachangelog.com/pl/1.0.0/).
Wersjonowanie zgodne z [Semantic Versioning](https://semver.org/lang/pl/).

---

## [1.0.1] - 2025-02-12

### Naprawiono
- **KRYTYCZNE:** Zmiana pinów przycisków z GPIO 35/36/37 na GPIO 38/39/40 - piny 33-37 są zajęte przez Octal PSRAM modułu N16R8 (powodowały Guru Meditation Error: StoreProhibited)
- Zamiana `analogWrite` na ESP32 LEDC API (`ledcSetup`/`ledcAttachPin`/`ledcWrite`) dla podświetlenia wyświetlacza - kompatybilność z Arduino-ESP32 2.x
- Usunięcie flagi `ARDUINO_USB_CDC_ON_BOOT` - Serial teraz trafia na UART0 (ten sam port co crash dump)
- Dodanie `USE_HSPI_PORT=1` dla TFT_eSPI - izolacja magistrali SPI3 od potencjalnych konfliktów z PSRAM
- Obniżenie częstotliwości SPI z 40MHz do 27MHz dla większej stabilności
- Dodanie opóźnienia 100ms przed inicjalizacją wyświetlacza (stabilizacja zasilania)
- Zwiększenie opóźnienia startowego do 1000ms (stabilizacja UART)

### Zmieniono
- Przyciski BS-33B: Start/Pauza=GPIO38, Stop=GPIO39, Selektor=GPIO40
- Zaktualizowana dokumentacja podłączeń i instrukcja obsługi

---

## [1.0.0] - 2025-02-12

### Dodano
- Pierwsza wersja firmware TrassarV3
- Obsługa wyświetlacza ILI9341 2.8" (240x320) przez SPI
- Interfejs graficzny z systemem menu:
  - Ekran główny z czasem, datą i statusem maszyny
  - Menu główne z 4 opcjami
  - Ustawienia malowania (prędkość, liczba przejść)
  - Ustawienia czasu i daty (edycja przez enkoder)
  - Informacje WiFi (SSID, IP, hasło, klienci)
  - Informacje systemowe (firmware, RAM, uptime)
  - Ekran malowania z postępem
- Serwer WWW jako Access Point WiFi:
  - SSID: TrassarV3, hasło: 12345678
  - Responsywny panel sterowania HTML
  - API REST (JSON) do statusu i sterowania
  - Auto-odświeżanie statusu co 1 sekundę
- Obsługa 3 przycisków monostabilnych BS-33B:
  - Start/Pauza - uruchamianie i pauzowanie malowania
  - Stop - zatrzymanie procesu, długie naciśnięcie = wejście w menu
  - Selektor - nawigacja po menu (krótko = następna opcja, długo = wejście)
- Obsługa enkodera obrotowego z przyciskiem:
  - Obrót = przewijanie menu / regulacja wartości
  - Przycisk = potwierdzenie wyboru
- Zegar RTC DS1307:
  - Wyświetlanie czasu i daty
  - Edycja czasu i daty przez menu
  - Automatyczne ustawienie czasu kompilacji przy pierwszym uruchomieniu
- Konfiguracja pinów dla ESP32-S3 N16R8
- Pełna dokumentacja: instrukcja obsługi, schemat podłączeń, API WWW

### Platforma
- ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM)
- Framework: Arduino (PlatformIO)
- Biblioteki: TFT_eSPI, RTClib, ArduinoJson
