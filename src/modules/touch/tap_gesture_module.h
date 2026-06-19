#pragma once

#include <Arduino.h>

#include "touch_handler.h"
#include "display.h"
#include "display_mode.h"
#include "settings.h"
#include "ble_client.h"
#include "packet_parser.h"
#include "modules/screens/screen_manager.h"

class AutoPushModule;
class AlertPersistenceModule;
class QuietCoordinatorModule;

class TapGestureModule {
public:
    void begin(TouchHandler* touchHandler,
               SettingsManager* settingsMgr,
               V1Display* display,
               V1BLEClient* bleClient,
               PacketParser* parser,
               AutoPushModule* autoPushModule,
               AlertPersistenceModule* alertPersistenceModule,
               DisplayMode* displayModePtr,
               QuietCoordinatorModule* quietCoordinator);

    void process(unsigned long nowMs);

private:
    TouchHandler* touch_ = nullptr;
    SettingsManager* settings_ = nullptr;
    V1Display* display_ = nullptr;
    V1BLEClient* ble_ = nullptr;
    PacketParser* parser_ = nullptr;
    AutoPushModule* autoPush_ = nullptr;
    AlertPersistenceModule* alertPersistence_ = nullptr;
    DisplayMode* displayMode_ = nullptr;
    QuietCoordinatorModule* quiet_ = nullptr;

    unsigned long lastTapTime_ = 0;
    int tapCount_ = 0;
    static constexpr int PROFILE_CHANGE_TAP_COUNT = 3;
    static constexpr unsigned long TAP_WINDOW_MS = 600;
    static constexpr unsigned long TAP_DEBOUNCE_MS = 150;

    // Zone-based screen navigation: left/right 25% of 640px display
    static constexpr int16_t NAV_LEFT_ZONE_PX  = 160;  // x < 160 → previous screen
    static constexpr int16_t NAV_RIGHT_ZONE_PX = 480;  // x > 480 → next screen
};
