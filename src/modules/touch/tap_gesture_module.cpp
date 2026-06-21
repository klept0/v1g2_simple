#include "tap_gesture_module.h"
#include "../quiet/quiet_coordinator_module.h"
#include "../perf/debug_macros.h"
#ifndef UNIT_TEST
#include "modules/display/dashboard_module.h"
#include "modules/auto_push/auto_push_module.h"
#include "modules/alert_persistence/alert_persistence_module.h"
#endif

void TapGestureModule::begin(TouchHandler* touchHandler,
                             SettingsManager* settingsMgr,
                             V1Display* displayPtr,
                             V1BLEClient* bleClient,
                             PacketParser* parserPtr,
                             AutoPushModule* autoPushModule,
                             AlertPersistenceModule* alertPersistenceModule,
                             DisplayMode* displayModePtr,
                             QuietCoordinatorModule* quietCoordinator
#ifndef UNIT_TEST
                             , DashboardModule* dashboardModule
#endif
                             ) {
    touch_ = touchHandler;
    settings_ = settingsMgr;
    display_ = displayPtr;
    ble_ = bleClient;
    parser_ = parserPtr;
    autoPush_ = autoPushModule;
    alertPersistence_ = alertPersistenceModule;
    displayMode_ = displayModePtr;
    quiet_ = quietCoordinator;
#ifndef UNIT_TEST
    dashboard_ = dashboardModule;
#endif
}

void TapGestureModule::process(unsigned long nowMs) {
    if (!touch_ || !settings_ || !display_ || !ble_ || !parser_ || !autoPush_ || !alertPersistence_ || !displayMode_) {
        return;
    }

    // If a single tap was pending and the window expired with no follow-up,
    // fire the dashboard toggle now.
#ifndef UNIT_TEST
    if (pendingDashboardToggle_ && tapCount_ == 1 &&
        nowMs - lastTapTime_ > TAP_WINDOW_MS) {
        pendingDashboardToggle_ = false;
        if (dashboard_) {
            dashboard_->cycleNext();
            if (!dashboard_->isActive()) {
                display_->forceNextRedraw();
            }
            DBG_PRINTLN("Idle screen cycled via single tap");
        }
        tapCount_ = 0;
    }
#endif

    int16_t touchX, touchY;
    bool hasActiveAlert = parser_->hasAlerts();

    // getTouchPoint() returns true only on the rising edge of each tap.
    // The touch hardware (AXS15231B) applies 200ms debounce internally,
    // so each true return is a distinct user tap.
    if (!touch_->getTouchPoint(touchX, touchY)) {
        return;
    }

    auto performMuteToggle = [&](const char* reason) {
        if (!hasActiveAlert) {
            DBG_PRINTLN("MUTE BLOCKED: No active alert to mute");
            return;
        }

        DisplayState state = parser_->getDisplayState();
        bool currentMuted = state.muted;
        bool newMuted = !currentMuted;

        DBG_PRINTF("Mute: %s -> Sending: %s (%s)\n",
                      currentMuted ? "MUTED" : "UNMUTED",
                      newMuted ? "MUTE_ON" : "MUTE_OFF",
                      reason);

        const bool cmdSent = quiet_ && quiet_->sendMute(QuietOwner::TapGesture, newMuted);
        DBG_PRINTF("Mute command sent: %s\n", cmdSent ? "OK" : "FAIL");
    };

    auto performProfileCycle = [&]() {
        const V1Settings& s = settings_->get();
        int newSlot = (s.activeSlot + 1) % 3;
        settings_->setActiveSlot(newSlot);
        *displayMode_ = DisplayMode::IDLE;

        alertPersistence_->clearPersistence();

        const char* slotNames[] = {"Default", "Highway", "Comfort"};
        DBG_PRINTF("PROFILE CHANGE: Switched to '%s' (slot %d)\n", slotNames[newSlot], newSlot);

        display_->drawProfileIndicator(newSlot);

        if (ble_->isConnected() && s.autoPushEnabled) {
            DBG_PRINTLN("Pushing new profile to V1...");
            const auto queueResult = autoPush_->queueSlotPush(newSlot);
            if (queueResult != AutoPushModule::QueueResult::QUEUED) {
                DBG_PRINTF("Profile push skipped: %d\n", static_cast<int>(queueResult));
            }
        }
    };

    DBG_PRINTF("Tap at x=%d y=%d hasAlert=%d\n", touchX, touchY, hasActiveAlert);

    // Alert active: any tap mutes (and exits dashboard if active)
    if (hasActiveAlert) {
#ifndef UNIT_TEST
        if (dashboard_ && dashboard_->isActive()) {
            dashboard_->setActive(false);
            display_->forceNextRedraw();
        }
#endif
        performMuteToggle("tap");
        tapCount_ = 0;
        pendingDashboardToggle_ = false;
        return;
    }

    // No alert: count taps.
    // A single isolated tap (no follow-up within TAP_WINDOW_MS) toggles the
    // dashboard.  Triple-tap within the window cycles the profile as before.
    if (nowMs - lastTapTime_ >= TAP_DEBOUNCE_MS) {
        if (nowMs - lastTapTime_ <= TAP_WINDOW_MS) {
            // Follow-up tap — cancel any pending dashboard toggle
            pendingDashboardToggle_ = false;
            tapCount_++;
        } else {
            // New tap sequence; arm dashboard toggle and start counting
            tapCount_ = 1;
            pendingDashboardToggle_ = true;
        }
        lastTapTime_ = nowMs;

        DBG_PRINTF("Profile tap count: %d\n", tapCount_);

        if (tapCount_ >= PROFILE_CHANGE_TAP_COUNT) {
            pendingDashboardToggle_ = false;
            performProfileCycle();
            tapCount_ = 0;
        }
    }
}

// NOTE: process() is called every main loop iteration.  Between tap events
// we also check whether a pending single-tap dashboard toggle has timed out.
