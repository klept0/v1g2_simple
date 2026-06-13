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

    // Swipe tracking
    int16_t swipeTouchStartX_   = 0;
    int16_t swipeCurrentX_      = 0;
    bool    swipeTracking_      = false;
    unsigned long swipeTouchStartMs_ = 0;
    static constexpr int SWIPE_THRESHOLD_PX = 40;
    static constexpr unsigned long SWIPE_MAX_DURATION_MS = 800;
};
