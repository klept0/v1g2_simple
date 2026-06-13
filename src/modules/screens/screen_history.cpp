#include "screen_history.h"
#include "display.h"
#include "modules/history/history_manager.h"
#include "main_globals.h"

static constexpr uint16_t SCR_GRAY  = 0x7BEF;

void drawHistoryScreen(V1Display& disp) {
    auto* canvas = disp.canvas();
    canvas->fillScreen(COLOR_BLACK);

    // Header
    canvas->setTextSize(1);
    canvas->setTextColor(SCR_GRAY);
    canvas->setCursor(4, 4);
    canvas->print("HISTORY");

    if (historyManager.count() == 0) {
        canvas->setTextSize(2);
        canvas->setTextColor(COLOR_WHITE);  // from display_driver.h macro
        canvas->setCursor(200, 76);
        canvas->print("No encounters");
        disp.flush();
        return;
    }

    const uint32_t nowMs = millis();
    const int rowH = 28;
    const int startY = 20;
    const int maxRows = 5;
    const int total = historyManager.count() < maxRows ? historyManager.count() : maxRows;

    canvas->setTextSize(2);
    for (int i = 0; i < total; i++) {
        const Encounter& enc = historyManager.get(i);
        int y = startY + i * rowH;

        // Time ago
        uint32_t agoMs = (nowMs > enc.endMs) ? (nowMs - enc.endMs) : 0;
        uint32_t agoMin = agoMs / 60000;

        char line[48];
        if (enc.frequency[0] != '\0') {
            snprintf(line, sizeof(line), "%-6s %s  %lum ago", enc.band, enc.frequency, (unsigned long)agoMin);
        } else {
            snprintf(line, sizeof(line), "%-6s            %lum ago", enc.band, (unsigned long)agoMin);
        }

        canvas->setTextColor(COLOR_WHITE);
        canvas->setCursor(4, y);
        canvas->print(line);
    }

    disp.flush();
}
