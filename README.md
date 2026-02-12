# TrassarV3 - Sterownik maszyny malarskiej

Firmware dla sterownika automatycznej maszyny malarskiej oparty na platformie **ESP32-S3 N16R8**.

## Funkcje

- **Wyświetlacz TFT 2.8" ILI9341** (240x320, SPI) - interfejs graficzny z menu
- **Serwer WWW** - zdalny panel sterowania przez WiFi (Access Point)
- **Zegar RTC DS1307** - czas rzeczywisty z podtrzymaniem bateryjnym
- **Nawigacja** - 3 przyciski BS-33B + enkoder obrotowy
- **Sterowanie malowaniem** - start, pauza, stop, regulacja prędkości

## Komponenty sprzętowe

| Komponent | Opis |
|-----------|------|
| ESP32-S3 N16R8 | Płytka deweloperska, 16MB Flash, 8MB PSRAM |
| ILI9341 2.8" | Wyświetlacz LCD 240x320 SPI z panelem dotykowym |
| DS1307 | Zegar RTC z baterią |
| BS-33B x3 | Przyciski monostabilne (Start/Pauza, Stop, Selektor) |
| Enkoder obrotowy | Z przyciskiem, do nawigacji i regulacji wartości |

## Podłączenie WiFi

- **SSID:** `TrassarV3`
- **Hasło:** `12345678`
- **IP panelu:** `192.168.4.1`
- **URL:** `http://192.168.4.1`

## Kompilacja

Projekt wykorzystuje **PlatformIO**. Aby skompilować:

```bash
# Instalacja PlatformIO CLI (jeśli brak)
pip install platformio

# Kompilacja
pio run

# Upload na ESP32-S3
pio run --target upload

# Monitor szeregowy
pio device monitor
```

## Struktura projektu

```
TrassarV3/
├── platformio.ini          # Konfiguracja PlatformIO
├── src/
│   ├── main.cpp            # Główny plik programu
│   ├── config.h            # Definicje pinów i stałych
│   ├── display_manager.h/cpp   # Obsługa wyświetlacza ILI9341
│   ├── button_handler.h/cpp    # Obsługa przycisków i enkodera
│   ├── rtc_handler.h/cpp       # Obsługa zegara RTC DS1307
│   ├── web_server.h/cpp        # Serwer WWW (WiFi AP)
│   └── menu.h/cpp              # System menu
├── docs/
│   ├── INSTRUKCJA_OBSLUGI.md   # Instrukcja obsługi
│   ├── SCHEMAT_PODLACZEN.md    # Schemat podłączeń
│   └── API_WWW.md              # Dokumentacja API serwera
├── CHANGELOG.md            # Historia zmian
└── README.md               # Ten plik
```

## Dokumentacja

- [Instrukcja obsługi](docs/INSTRUKCJA_OBSLUGI.md)
- [Schemat podłączeń](docs/SCHEMAT_PODLACZEN.md)
- [API serwera WWW](docs/API_WWW.md)
- [Historia zmian](CHANGELOG.md)

## Wersja

Aktualna wersja firmware: **v1.0.0**

## Licencja

Projekt prywatny. Wszelkie prawa zastrzeżone.
