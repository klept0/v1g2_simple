#pragma once

#include <Arduino.h>

#include "touch_handler.h"
#include "display.h"
#include "display_mode.h"
#include "settings.h"
#include "ble_client.h"
#include "packet_parser.h"
class AutoPushModule;
class AlertPersistenceModule;
class QuietCoordinatorModule;
#ifndef UNIT_TEST
class DashboardModule;
#endif

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
               QuietCoordinatorModule* quietCoordinator
#ifndef UNIT_TEST
               , DashboardModule* dashboardModule = nullptr
#endif
               );

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
#ifndef UNIT_TEST
    DashboardModule* dashboard_ = nullptr;
#endif

    unsigned long lastTapTime_ = 0;
    int tapCount_ = 0;

    static constexpr int SCREEN_CYCLE_TAP_COUNT   = 2;
    static constexpr int PROFILE_CHANGE_TAP_COUNT = 3;
    static constexpr unsigned long TAP_WINDOW_MS   = 600;
    static constexpr unsigned long TAP_DEBOUNCE_MS = 150;
};
