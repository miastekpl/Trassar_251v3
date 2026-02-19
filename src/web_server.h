#pragma once
// ============================================================
// TrassarV3 - Modul serwera WWW (WiFi AP)
// Dziala na Core 0 jako osobny task FreeRTOS
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

class TrassarWebServer {
public:
    void begin();       // Inicjalizacja WiFi + start tasku na Core 0
    void update();      // Wywolywane przez task na Core 0 (nie z loop!)
    String getIPAddress();
    int getConnectedClients();

    // Stack high-water mark tasku WWW (diagnostyka)
    uint32_t getTaskStackHWM() const;

private:
    WebServer server{WEB_SERVER_PORT};
    TaskHandle_t webTaskHandle = nullptr;

    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleStats();
    void handleReports();
    void handleReportDownload();
    void handleControl();
    void handleNotFound();

    String buildHtmlPage();
    String getStateJson();
    String getStatsJson();
    String getReportsJson();

    static void webTaskFunc(void* param);
};

extern TrassarWebServer webServer;
