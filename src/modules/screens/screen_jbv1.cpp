#include "screen_jbv1.h"
#include "display.h"
#include "modules/services/jbv1_client.h"
#include "main_globals.h"

// RGB565 gray (others from display_driver.h macros)
static constexpr uint16_t SCR_GRAY = 0x7BEF;

void drawJBV1Screen(V1Display& disp) {
    auto* canvas = disp.canvas();
    canvas->fillScreen(COLOR_BLACK);

    if (!g_jbv1.valid()) {
        canvas->setTextSize(2);
        canvas->setTextColor(SCR_GRAY);
        // Approximate centering for 640x172 canvas
        canvas->setCursor(220, 76);
        canvas->print("NO JBV1 DATA");
        disp.flush();
        return;
    }

    const int delta = g_jbv1.speed - g_jbv1.speedLimit;
    uint16_t speedColor = COLOR_WHITE;
    if (delta >= 10) {
        speedColor = COLOR_RED;
    } else if (delta >= 1) {
        speedColor = COLOR_YELLOW;
    }

    // Left column: speed, PSL, delta
    canvas->setTextColor(speedColor);
    canvas->setTextSize(4);
    canvas->setCursor(10, 10);
    canvas->print(g_jbv1.speed);
    canvas->print(" MPH");

    canvas->setTextColor(COLOR_WHITE);
    canvas->setTextSize(3);
    canvas->setCursor(10, 55);
    canvas->print("PSL ");
    canvas->print(g_jbv1.speedLimit);

    // Delta
    canvas->setTextSize(3);
    canvas->setCursor(10, 95);
    if (delta > 0) {
        canvas->setTextColor(speedColor);
        canvas->print("+");
        canvas->print(delta);
    } else {
        canvas->setTextColor(COLOR_WHITE);
        canvas->print(delta);
    }

    // Right column: heading, satellites, GPS accuracy
    canvas->setTextColor(COLOR_WHITE);
    canvas->setTextSize(4);
    canvas->setCursor(340, 10);
    canvas->print(g_jbv1.heading);

    canvas->setTextSize(3);
    canvas->setCursor(340, 55);
    canvas->print(g_jbv1.satellites);
    canvas->print(" SAT");

    canvas->setCursor(340, 95);
    canvas->print("+/-");
    canvas->print(g_jbv1.gpsAccuracy);
    canvas->print("m");

    disp.flush();
}
