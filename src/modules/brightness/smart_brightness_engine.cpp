#include "smart_brightness_engine.h"
#include "../../settings.h"

#ifndef UNIT_TEST
#include "../../display.h"
#endif

void SmartBrightnessEngine::begin(V1Display* display, SettingsManager* settings, uint32_t nowMs) {
    display_        = display;
    settings_       = settings;
    lastActivityMs_ = nowMs;
    dayBase_        = settings_ ? settings_->getBrtDay() : 200;
    currentLevel_   = 255;
    state_          = SmartBrightnessState::DayActive;
    prevHasAlerts_  = false;
    prevIsMuted_    = false;
}

void SmartBrightnessEngine::applyBrightness(uint8_t level) {
    if (level == currentLevel_) return;
    currentLevel_ = level;
#ifndef UNIT_TEST
    if (display_) display_->setBrightness(level);
#endif
}

void SmartBrightnessEngine::notifyActivity(uint32_t nowMs) {
    lastActivityMs_ = nowMs;
    // Wake from idle dim immediately
    if (state_ == SmartBrightnessState::IdleDim) {
        state_ = SmartBrightnessState::DayActive;
        applyBrightness(dayBase_);
    }
}

void SmartBrightnessEngine::setDayBase(uint8_t brightness, uint32_t nowMs) {
    dayBase_ = brightness;
    notifyActivity(nowMs);
    if (state_ == SmartBrightnessState::DayActive) {
        applyBrightness(dayBase_);
    }
}

void SmartBrightnessEngine::process(uint32_t nowMs, bool hasAlerts, bool isMuted) {
    if (!settings_) return;
    if (!settings_->isBrightEngEnabled()) {
        // Engine disabled — do nothing (firmware controls brightness externally)
        prevHasAlerts_ = hasAlerts;
        prevIsMuted_   = isMuted;
        return;
    }

    if (hasAlerts) {
        lastActivityMs_ = nowMs;  // alerts count as activity for idle dim timer

        if (isMuted) {
            if (state_ != SmartBrightnessState::AlertMuted) {
                state_ = SmartBrightnessState::AlertMuted;
                applyBrightness(settings_->getBrtMute());
            }
        } else {
            if (state_ != SmartBrightnessState::AlertBoosted) {
                state_ = SmartBrightnessState::AlertBoosted;
                applyBrightness(settings_->getBrtAlert());
            }
        }
    } else {
        if (prevHasAlerts_) {
            // Alert just cleared — restore to day base
            state_ = SmartBrightnessState::DayActive;
            applyBrightness(dayBase_);
        }

        if (state_ != SmartBrightnessState::IdleDim) {
            uint16_t timeoutSec = settings_->getBrtIdleSec();
            if (timeoutSec > 0) {
                uint32_t elapsedMs = nowMs - lastActivityMs_;
                if (elapsedMs >= static_cast<uint32_t>(timeoutSec) * 1000u) {
                    state_ = SmartBrightnessState::IdleDim;
                    applyBrightness(settings_->getBrtIdle());
                }
            }
        }
    }

    prevHasAlerts_ = hasAlerts;
    prevIsMuted_   = isMuted;
}
