#pragma once
// ============================================================
// TrassarV3 - Modul serwera WWW (WiFi AP) + WebSocket
// v2.21.0 - WebSocket push, GeoJSON endpoint, GPS track API
// Dziala na Core 0 jako osobny task FreeRTOS
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "config.h"

class TrassarWebServer {
public:
    void begin();       // Inicjalizacja WiFi + WS + start tasku na Core 0
    void update();      // Wywolywane przez task na Core 0 (nie z loop!)
    String getIPAddress();
    int getConnectedClients();
    bool isLittleFsReady() const { return littleFsReady; }

    // Haslo WiFi (generowane z MAC adresu)
    const char* getPassword() const { return wifiPassword; }

    // Stack high-water mark tasku WWW (diagnostyka)
    uint32_t getTaskStackHWM() const;

    // Fix #15/#26/#28: Software watchdog Core 0 — monitorowane z Core 1
    volatile unsigned long core0AliveMs = 0;  // Timestamp ostatniej aktywnosci tasku
    volatile uint8_t hangCount = 0;           // Licznik wykrytych zawieszen
    static const uint8_t MAX_HANGS_BEFORE_REBOOT = 6;  // Fix #26: 3->6 (wiecej tolerancji)
    volatile unsigned long lastRepairMs = 0;  // Fix #26: Timestamp ostatniej naprawy (cooldown)
    volatile uint8_t totalSelfRepairs = 0;    // Fix #28: Laczna liczba selfRepair w sesji
    static const uint8_t MAX_SELF_REPAIRS_BEFORE_REBOOT = 8;  // Fix #29: 5→8 (decay resetuje licznik)
    bool isCore0Alive(unsigned long now, unsigned long timeoutMs = 5000) const {
        return (now - core0AliveMs) < timeoutMs;
    }

    // Fix #16: Rozlaczenie WiFi nie moze powodowac restartu tasku
    void disconnectAllWsClients();  // Rozlacz wszystkie WS klienty (przy WiFi disconnect)

    // Fix #22: Flaga samonaprawy — task sam reinicjalizuje serwery z Core 0
    volatile bool selfRepairRequested = false;

private:
    volatile bool wifiStationConnected = false;  // Flaga: jest podlaczony klient WiFi
    volatile bool wsDisconnectRequested = false;  // Fix #18: Flaga rozlaczenia WS (thread-safe)
    WebServer server{WEB_SERVER_PORT};
    WebSocketsServer wsServer{WS_PORT};
    TaskHandle_t webTaskHandle = nullptr;
    unsigned long lastWsBroadcast = 0;
    bool littleFsReady = false;
    char wifiPassword[16] = {};  // Haslo WiFi generowane z MAC

    void generatePassword();
    void selfRepairServers();  // Fix #22: reinicjalizacja z Core 0

    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleStats();
    void handleReports();
    void handleReportDownload();
    void handleControl();
    void handleGeoJson();
    void handleTrackList();
    void handleTrackDownload();
    void handleHtmlReports();
    void handleHtmlReportDownload();
    void handleNotFound();
    bool handleStaticFile(const String& path);

    String buildHtmlPage();
    String getStateJson();
    String getStatsJson();
    String getReportsJson();

    static void webTaskFunc(void* param);
};

extern TrassarWebServer webServer;
