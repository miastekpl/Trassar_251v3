#pragma once
// ============================================================
// TrassarV3 - Modul serwera WWW (WiFi AP)
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

class TrassarWebServer {
public:
    void begin();
    void update();
    String getIPAddress();
    int getConnectedClients();

private:
    WebServer server{WEB_SERVER_PORT};

    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleControl();
    void handleNotFound();

    String buildHtmlPage();
    String getStateJson();
};

extern TrassarWebServer webServer;
