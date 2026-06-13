#include "screen_clock.h"
#include "display.h"
#include "time_service.h"
#include <time.h>

// Gray not in display_driver.h macros
static constexpr uint16_t SCR_GRAY = 0x7BEF;

static const char* dayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};
static const char* monthNames[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

void drawClockScreen(V1Display& disp) {
    auto* canvas = disp.canvas();
    canvas->fillScreen(COLOR_BLACK);

    int64_t epochMs = timeService.nowEpochMsOr0();
    if (epochMs == 0) {
        canvas->setTextSize(3);
        canvas->setTextColor(SCR_GRAY);
        canvas->setCursor(200, 70);
        canvas->print("--:-- --");
        disp.flush();
        return;
    }

    int32_t tzOffset = timeService.tzOffsetMinutes();
    int64_t localMs  = epochMs + (int64_t)tzOffset * 60000LL;
    time_t  t        = (time_t)(localMs / 1000LL);

    struct tm tm_local;
    gmtime_r(&t, &tm_local);

    int hour12 = tm_local.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = (tm_local.tm_hour < 12) ? "AM" : "PM";

    // Time in large text, approximately centered (640 wide, 172 tall)
    char timeBuf[12];
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d %s", hour12, tm_local.tm_min, ampm);

    canvas->setTextColor(COLOR_WHITE);
    canvas->setTextSize(4); // 32px tall chars
    // Rough centering: each char ~24px wide at size 4
    int16_t x = 160;
    canvas->setCursor(x, 30);
    canvas->print(timeBuf);

    // Day + date in smaller text
    char dateBuf[32];
    snprintf(dateBuf, sizeof(dateBuf), "%s  %s %d",
             dayNames[tm_local.tm_wday],
             monthNames[tm_local.tm_mon],
             tm_local.tm_mday);

    canvas->setTextColor(SCR_GRAY);
    canvas->setTextSize(2);
    canvas->setCursor(160, 110);
    canvas->print(dateBuf);

    disp.flush();
}
