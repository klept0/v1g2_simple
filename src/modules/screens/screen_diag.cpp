#include "screen_diag.h"
#include "display.h"

static constexpr uint16_t SCR_GRAY  = 0x7BEF;

void drawDiagScreen(V1Display& disp, const DisplayBleContext& ble) {
    auto* canvas = disp.canvas();
    canvas->fillScreen(COLOR_BLACK);

    // Header
    canvas->setTextSize(1);
    canvas->setTextColor(SCR_GRAY);
    canvas->setCursor(4, 4);
    canvas->print("DIAGNOSTICS");

    // BT status
    canvas->setTextSize(2);
    if (ble.v1Connected) {
        canvas->setTextColor(COLOR_GREEN);
        canvas->setCursor(4, 20);
        canvas->print("BT: Connected");
    } else {
        canvas->setTextColor(COLOR_RED);
        canvas->setCursor(4, 20);
        canvas->print("BT: Disconnected");
    }

    // RSSI
    canvas->setTextColor(COLOR_WHITE);
    canvas->setCursor(4, 50);
    canvas->print("RSSI  ");
    canvas->print(ble.v1Rssi);
    canvas->print(" dBm");

    // Packets per second
    canvas->setCursor(4, 80);
    canvas->print("PKT   ");
    canvas->print(ble.packetsPerSecond);
    canvas->print("/s");

    // Last packet age
    canvas->setCursor(4, 110);
    canvas->print("AGE   ");
    canvas->print(ble.lastPacketAgeMs);
    canvas->print(" ms");

    // Firmware
    canvas->setTextColor(SCR_GRAY);
    canvas->setTextSize(1);
    canvas->setCursor(4, 158);
    canvas->print("v1g2_simple");

    disp.flush();
}
