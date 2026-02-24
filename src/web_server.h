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

    // Stack high-water mark tasku WWW (diagnostyka)
    uint32_t getTaskStackHWM() const;

private:
    WebServer server{WEB_SERVER_PORT};
    WebSocketsServer wsServer{WS_PORT};
    TaskHandle_t webTaskHandle = nullptr;
    unsigned long lastWsBroadcast = 0;

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
    void handleNotFound();

    String buildHtmlPage();
    String getStateJson();
    String getStatsJson();
    String getReportsJson();

    static void webTaskFunc(void* param);
};

extern TrassarWebServer webServer;
