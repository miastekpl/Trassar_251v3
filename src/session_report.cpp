#include "sys_log.h"
// ============================================================
// TrassarV3 - Automatyczny raport HTML po sesji malowania
// ============================================================

#include "session_report.h"
#include "report_logger.h"
#include "rtc_handler.h"
#include "event_log.h"
#include "statistics.h"
#include "patterns.h"
#include <SD.h>
#include <esp_task_wdt.h>

SessionReport sessionReport;

bool SessionReport::generateReport(const char* patCode, float distM, float areaM2,
                                   unsigned long timeSec, float avgSpeedKmh,
                                   bool hasGps, double lat, double lon,
                                   float paintUsedL, float paintRemainingL) {
    if (!reportLogger.isReady()) return false;

    if (!SD_LOCK()) return false;

    // Utworz katalog raportow
    if (!SD.exists("/html_reports")) {
        SD.mkdir("/html_reports");
    }

    // Nazwa pliku: raport_RRRRMMDD_HHMMSS.html
    const char* dt = rtcModule.getDateTimeStr();
    char fname[64];
    // dt format: "RRRR-MM-DD HH:MM:SS"
    char datePart[16] = {};
    char timePart[16] = {};
    int di = 0, ti = 0;
    bool inTime = false;
    for (int i = 0; dt[i]; i++) {
        char c = dt[i];
        if (c == ' ') { inTime = true; continue; }
        if (c == '-' || c == ':') continue;
        if (!inTime) datePart[di++] = c;
        else timePart[ti++] = c;
    }
    snprintf(fname, sizeof(fname), "/html_reports/raport_%s_%s.html", datePart, timePart);

    File f = SD.open(fname, FILE_WRITE);
    if (!f) {
        SD_UNLOCK();
        return false;
    }

    // Formatowanie czasu
    unsigned long h = timeSec / 3600;
    unsigned long m = (timeSec % 3600) / 60;
    unsigned long s = timeSec % 60;
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%lu:%02lu:%02lu", h, m, s);

    // Formatowanie dystansu
    char distStr[32];
    if (distM >= 1000.0f) {
        snprintf(distStr, sizeof(distStr), "%.2f km", distM / 1000.0f);
    } else {
        snprintf(distStr, sizeof(distStr), "%.1f m", distM);
    }

    // Generuj HTML
    f.print(F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              "<title>Raport TrassarV3</title>"
              "<style>"
              "body{font-family:Arial,sans-serif;max-width:800px;margin:20px auto;padding:0 15px;background:#f5f5f5}"
              "h1{color:#1a5276;border-bottom:3px solid #2ecc71;padding-bottom:10px}"
              "h2{color:#2c3e50;margin-top:30px}"
              ".card{background:white;border-radius:8px;padding:20px;margin:15px 0;box-shadow:0 2px 4px rgba(0,0,0,0.1)}"
              "table{width:100%;border-collapse:collapse}"
              "td{padding:8px 12px;border-bottom:1px solid #eee}"
              "td:first-child{color:#7f8c8d;width:40%}"
              "td:last-child{font-weight:bold;color:#2c3e50}"
              ".accent{color:#27ae60}"
              ".warn{color:#e67e22}"
              "footer{text-align:center;color:#95a5a6;margin-top:30px;font-size:0.9em}"
              "</style></head><body>"));

    esp_task_wdt_reset();  // Fix #26: WDT reset po duzym bloku CSS (yield() nie resetuje WDT)
    f.print(F("<h1>Raport sesji malowania</h1>"));
    f.printf("<p>Data: <strong>%s</strong></p>", dt);

    f.print(F("<div class='card'><h2>Parametry sesji</h2><table>"));
    f.printf("<tr><td>Wzorzec</td><td class='accent'>%s</td></tr>", patCode);
    f.printf("<tr><td>Dystans</td><td>%s</td></tr>", distStr);
    f.printf("<tr><td>Powierzchnia</td><td>%.2f m&sup2;</td></tr>", areaM2);
    f.printf("<tr><td>Czas malowania</td><td>%s</td></tr>", timeStr);
    f.printf("<tr><td>Srednia predkosc</td><td>%.1f km/h</td></tr>", avgSpeedKmh);
    f.print(F("</table></div>"));

    // Rozbicie per wzorzec (jesli bylo wiecej niz 1 wzorzec)
    int patCount = stats.getPatternEntryCount();
    if (patCount > 1) {
        f.print(F("<div class='card'><h2>Wzorce w sesji</h2><table>"
                  "<tr><td><strong>Wzorzec</strong></td><td><strong>Dystans / Powierzchnia</strong></td></tr>"));
        for (int i = 0; i < patCount; i++) {
            const auto& pe = stats.getPatternEntry(i);
            const PatternDef& pd = patternMgr.getPattern(pe.pattern);
            char dBuf[32];
            if (pe.distance >= 1000.0f)
                snprintf(dBuf, sizeof(dBuf), "%.2f km", pe.distance / 1000.0f);
            else
                snprintf(dBuf, sizeof(dBuf), "%.1f m", pe.distance);
            f.printf("<tr><td class='accent'>%s</td><td>%s / %.2f m&sup2;</td></tr>",
                     pd.code, dBuf, pe.area);
        }
        f.print(F("</table></div>"));
    }

    esp_task_wdt_reset();  // Fix #26: WDT reset miedzy sekcjami raportu

    // Sekcja zuzycia farby
    f.print(F("<div class='card'><h2>Zuzycie farby</h2><table>"));
    f.printf("<tr><td>Zuzyto (sesja)</td><td>%.1f L</td></tr>", paintUsedL);
    f.printf("<tr><td>Pozostalo w zbiorniku</td><td class='%s'>%.1f L</td></tr>",
             paintRemainingL < 20.0f ? "warn" : "accent", paintRemainingL);
    f.print(F("</table></div>"));

    // GPS
    if (hasGps && (lat != 0 || lon != 0)) {
        f.print(F("<div class='card'><h2>Lokalizacja GPS</h2><table>"));
        f.printf("<tr><td>Wspolrzedne</td><td>%.6f, %.6f</td></tr>", lat, lon);
        f.print(F("</table></div>"));
    }

    f.printf("<footer>TrassarV3 v%s | Wygenerowano automatycznie</footer>", FW_VERSION);
    f.print(F("</body></html>"));
    f.close();
    SD_UNLOCK();

    // Zapamietaj nazwe pliku (bez sciezki /html_reports/)
    strncpy(lastReportFile, fname + 14, sizeof(lastReportFile) - 1);  // pomiń "/html_reports/"

    DBG_PRINTF("[REPORT] Raport HTML: %s\n", fname);
    eventLog.logf("REPORT", "Wygenerowano raport HTML: %s", lastReportFile);

    return true;
}
