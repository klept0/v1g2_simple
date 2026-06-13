#include "screen_manager.h"
#include "display.h"
#include "display_ble_context.h"
#include "modules/screens/screen_radar.h"
#include "modules/screens/screen_jbv1.h"
#include "modules/screens/screen_history.h"
#include "modules/screens/screen_diag.h"
#include "modules/screens/screen_clock.h"

ScreenManager screenManager;

void ScreenManager::next() {
    uint8_t n = (static_cast<uint8_t>(current_) + 1) % static_cast<uint8_t>(ScreenType::COUNT);
    current_ = static_cast<ScreenType>(n);
}

void ScreenManager::previous() {
    uint8_t cur = static_cast<uint8_t>(current_);
    uint8_t count = static_cast<uint8_t>(ScreenType::COUNT);
    uint8_t n = (cur == 0) ? (count - 1) : (cur - 1);
    current_ = static_cast<ScreenType>(n);
}

void ScreenManager::set(ScreenType s) {
    if (static_cast<uint8_t>(s) < static_cast<uint8_t>(ScreenType::COUNT)) {
        current_ = s;
    }
}

void ScreenManager::forceRadar() {
    current_ = ScreenType::RADAR;
}

void ScreenManager::tick(bool hasAlert, uint32_t /*nowMs*/) {
    if (hasAlert && !lastHadAlert_) {
        forceRadar();
    }
    lastHadAlert_ = hasAlert;
}

void ScreenManager::render(V1Display& disp, const DisplayBleContext& ble) {
    switch (current_) {
        case ScreenType::RADAR:
            // No-op: radar is rendered by existing pipeline
            break;
        case ScreenType::JBV1:
            drawJBV1Screen(disp);
            break;
        case ScreenType::HISTORY:
            drawHistoryScreen(disp);
            break;
        case ScreenType::DIAG:
            drawDiagScreen(disp, ble);
            break;
        case ScreenType::CLOCK:
            drawClockScreen(disp);
            break;
        default:
            break;
    }
}
