#include "touch_ui_module.h"
#include "../perf/debug_macros.h"

#include "audio_beep.h"  // audio_set_volume, play_test_voice

namespace {
constexpr int kObdBadgeFlushX = 360;
constexpr int kObdBadgeFlushY = 0;
constexpr int kObdBadgeFlushW = 72;
constexpr int kObdBadgeFlushH = 36;
}

void TouchUiModule::begin(V1Display* disp,
               TouchHandler* touch,
               SettingsManager* settingsMgr,
               const Callbacks& cbs) {
    display_ = disp;
    touchHandler_ = touch;
    settings_ = settingsMgr;
    callbacks_ = cbs;
}

bool TouchUiModule::process(unsigned long nowMs, bool bootPressed) {
    if (!display_ || !touchHandler_ || !settings_) return false;

    if (bootPressed && !bootWasPressed_) {
        bootPressStart_ = nowMs;
        wifiToggleFired_ = false;
    }

    if (bootPressed) {
        const unsigned long held = nowMs - bootPressStart_;

        const bool shouldArmObdPair = (settingsPage_ == 0) &&
                                      held >= OBD_PAIR_LONG_PRESS_MS &&
                                      canArmObdPairGesture(nowMs);
        if (shouldArmObdPair != obdPairGestureArmed_) {
            obdPairGestureArmed_ = shouldArmObdPair;
            updateObdIndicatorAttention(obdPairGestureArmed_, nowMs);
        }

        if (!wifiToggleFired_ && !obdPairGestureArmed_ && held >= AP_TOGGLE_LONG_PRESS_MS) {
            wifiToggleFired_ = true;
            if (callbacks_.isWifiSetupActive && callbacks_.isWifiSetupActive(callbacks_.isWifiSetupActiveCtx)) {
                if (callbacks_.stopWifiSetup) callbacks_.stopWifiSetup(callbacks_.stopWifiSetupCtx);
            } else {
                if (callbacks_.startWifi) callbacks_.startWifi(callbacks_.startWifiCtx);
            }
            if (callbacks_.drawWifiIndicator) callbacks_.drawWifiIndicator(callbacks_.drawWifiIndicatorCtx);
            display_->flush();
        }
    }

    if (!bootPressed && bootWasPressed_) {
        unsigned long pressDuration = nowMs - bootPressStart_;
        const bool triggerObdPair = obdPairGestureArmed_ && pressDuration >= OBD_PAIR_LONG_PRESS_MS;
        const bool wifiAlreadyToggled = wifiToggleFired_;

        if (obdPairGestureArmed_) {
            obdPairGestureArmed_ = false;
            updateObdIndicatorAttention(false, nowMs);
        }
        wifiToggleFired_ = false;

        if (triggerObdPair) {
            if (callbacks_.requestObdManualPairScan) {
                (void)callbacks_.requestObdManualPairScan(nowMs, callbacks_.requestObdManualPairScanCtx);
            }
            display_->refreshObdIndicator(nowMs);
            display_->flushRegion(kObdBadgeFlushX, kObdBadgeFlushY, kObdBadgeFlushW, kObdBadgeFlushH);
        } else if (!wifiAlreadyToggled && pressDuration >= BOOT_DEBOUNCE_MS) {
            // Cycle through pages: 0 → 1 (sliders) → 2 (toggles) → 0 (exit)
            if (settingsPage_ == 0) {
                enterSlidersPage();
            } else if (settingsPage_ == 1) {
                enterTogglesPage();
            } else {
                exitAndSave();
            }
        }
    }

    bootWasPressed_ = bootPressed;

    if (settingsPage_ == 1) {
        bool touched = handleSliderTouch(nowMs);
        if (!touched && lastVolumeChangeMs_ > 0 &&
            (nowMs - lastVolumeChangeMs_) >= VOLUME_TEST_DEBOUNCE_MS) {
            play_test_voice();
            lastVolumeChangeMs_ = 0;
        }
        return true;
    }

    if (settingsPage_ == 2) {
        handleToggleTouch();
        return true;
    }

    return false;
}

// ─── Page 1: sliders ────────────────────────────────────────────────────────

void TouchUiModule::enterSlidersPage() {
    const V1Settings& s = settings_->get();
    settingsPage_ = 1;
    brightnessAdjustValue_ = s.brightness;
    volumeAdjustValue_ = s.voiceVolume;
    activeSlider_ = 0;
    lastVolumeChangeMs_ = 0;
    display_->showSettingsSliders(brightnessAdjustValue_, volumeAdjustValue_);
}

void TouchUiModule::exitAndSave() {
    settingsPage_ = 0;
    settings_->updateBrightness(brightnessAdjustValue_);
    settings_->updateVoiceVolume(volumeAdjustValue_);
    settings_->save();
    audio_set_volume(volumeAdjustValue_);
    display_->hideBrightnessSlider();
    if (callbacks_.restoreDisplay) callbacks_.restoreDisplay(callbacks_.restoreDisplayCtx);
}

bool TouchUiModule::handleSliderTouch(unsigned long nowMs) {
    int16_t touchX, touchY;
    if (!touchHandler_->getTouchPoint(touchX, touchY)) return false;

    const int sliderX = 40;
    const int sliderWidth = SCREEN_WIDTH - 80;

    int touchedSlider = display_->getActiveSliderFromTouch(touchY);
    if (touchedSlider < 0 || touchX < sliderX || touchX > sliderX + sliderWidth) return true;

    activeSlider_ = touchedSlider;

    if (activeSlider_ == 0) {
        int newLevel = 255 - (((touchX - sliderX) * 175) / sliderWidth);
        if (newLevel < 80) newLevel = 80;
        if (newLevel > 255) newLevel = 255;
        if (newLevel != brightnessAdjustValue_) {
            brightnessAdjustValue_ = newLevel;
            if ((nowMs - lastSliderRedrawMs_) >= SLIDER_REDRAW_MIN_MS) {
                display_->updateSettingsSliders(brightnessAdjustValue_, volumeAdjustValue_, activeSlider_);
                lastSliderRedrawMs_ = nowMs;
            }
        }
    } else if (activeSlider_ == 1) {
        int newVolume = 100 - (((touchX - sliderX) * 100) / sliderWidth);
        if (newVolume < 0) newVolume = 0;
        if (newVolume > 100) newVolume = 100;
        if (newVolume != volumeAdjustValue_) {
            volumeAdjustValue_ = newVolume;
            audio_set_volume(volumeAdjustValue_);
            if ((nowMs - lastSliderRedrawMs_) >= SLIDER_REDRAW_MIN_MS) {
                display_->updateSettingsSliders(brightnessAdjustValue_, volumeAdjustValue_, activeSlider_);
                lastSliderRedrawMs_ = nowMs;
            }
            lastVolumeChangeMs_ = nowMs;
        }
    }

    return true;
}

// ─── Page 2: toggles ────────────────────────────────────────────────────────

void TouchUiModule::enterTogglesPage() {
    settingsPage_ = 2;

    const V1Settings& s = settings_->get();
    bool wifiOn   = callbacks_.isWifiSetupActive
                    ? callbacks_.isWifiSetupActive(callbacks_.isWifiSetupActiveCtx)
                    : false;
    bool proxyOn  = s.proxyBLE;
    bool muteZero = callbacks_.getMuteToZero
                    ? callbacks_.getMuteToZero(callbacks_.getMuteToZeroCtx)
                    : false;

    display_->showTogglesPage(wifiOn, proxyOn, muteZero);
}

bool TouchUiModule::handleToggleTouch() {
    int16_t touchX, touchY;
    if (!touchHandler_->getTouchPoint(touchX, touchY)) return false;

    // Three equal-width buttons across 640px.
    // Touch X is mirrored vs. display X on this panel (rotation=1),
    // so the rightmost visual button (Mute) maps to the lowest touchX values.
    // Button 2 (Mute):     touchX 0..212   → visual right
    // Button 1 (BLE Proxy): touchX 213..426 → visual center
    // Button 0 (WiFi AP):   touchX 427..640 → visual left
    int btn = -1;
    if (touchX < 213)       btn = 2;  // Mute (right side visually)
    else if (touchX < 427)  btn = 1;  // BLE Proxy (center)
    else                    btn = 0;  // WiFi AP (left side visually)

    if (btn == 0) {
        // Toggle WiFi AP
        if (callbacks_.isWifiSetupActive && callbacks_.isWifiSetupActive(callbacks_.isWifiSetupActiveCtx)) {
            if (callbacks_.stopWifiSetup) callbacks_.stopWifiSetup(callbacks_.stopWifiSetupCtx);
        } else {
            if (callbacks_.startWifi) callbacks_.startWifi(callbacks_.startWifiCtx);
        }
    } else if (btn == 1) {
        // Toggle BLE proxy
        if (callbacks_.isProxyBleEnabled && callbacks_.setProxyBleEnabled) {
            bool current = callbacks_.isProxyBleEnabled(callbacks_.isProxyBleEnabledCtx);
            callbacks_.setProxyBleEnabled(!current, callbacks_.setProxyBleEnabledCtx);
        }
    } else if (btn == 2) {
        // Toggle Mute→0 for active slot
        if (callbacks_.getMuteToZero && callbacks_.setMuteToZero) {
            bool current = callbacks_.getMuteToZero(callbacks_.getMuteToZeroCtx);
            callbacks_.setMuteToZero(!current, callbacks_.setMuteToZeroCtx);
        }
    }

    // Redraw with updated state
    const V1Settings& s = settings_->get();
    bool wifiOn   = callbacks_.isWifiSetupActive
                    ? callbacks_.isWifiSetupActive(callbacks_.isWifiSetupActiveCtx)
                    : false;
    bool proxyOn  = s.proxyBLE;
    bool muteZero = callbacks_.getMuteToZero
                    ? callbacks_.getMuteToZero(callbacks_.getMuteToZeroCtx)
                    : false;
    display_->showTogglesPage(wifiOn, proxyOn, muteZero);
    return true;
}

// ─── OBD pair gesture helpers ────────────────────────────────────────────────

bool TouchUiModule::canArmObdPairGesture(unsigned long nowMs) const {
    if (!callbacks_.readObdStatus || !callbacks_.isObdPairGestureSafe) return false;
    const ObdRuntimeStatus status = callbacks_.readObdStatus(nowMs, callbacks_.readObdStatusCtx);
    return status.enabled && !status.connected && !status.scanInProgress &&
           !status.manualScanPending &&
           callbacks_.isObdPairGestureSafe(nowMs, callbacks_.isObdPairGestureSafeCtx);
}

void TouchUiModule::updateObdIndicatorAttention(bool attention, unsigned long nowMs) {
    display_->setObdAttention(attention);
    display_->refreshObdIndicator(nowMs);
    display_->flushRegion(kObdBadgeFlushX, kObdBadgeFlushY, kObdBadgeFlushW, kObdBadgeFlushH);
}
