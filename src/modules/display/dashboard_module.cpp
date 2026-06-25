#include "dashboard_module.h"
#include "display.h"
#include "packet_parser.h"
#include "settings.h"
#include "ble_client.h"
#include "battery_manager.h"
#include "wifi_manager.h"
#include "modules/obd/obd_runtime_module.h"
#include "modules/phone/phone_companion_module.h"
#include <string.h>
#include <stdio.h>

void DashboardModule::begin(V1Display* display,
                            PacketParser* parser,
                            SettingsManager* settings,
                            V1BLEClient* bleClient,
                            ObdRuntimeModule* obd,
                            PhoneCompanionModule* phone) {
    display_  = display;
    parser_   = parser;
    settings_ = settings;
    ble_      = bleClient;
    obd_      = obd;
    phone_    = phone;
}

DashboardData DashboardModule::snapshot(uint32_t nowMs) const {
    DashboardData d;

    if (!settings_ || !ble_) return d;

    const V1Settings& s = settings_->get();

    // Speed from OBD
    if (obd_) {
        const ObdRuntimeStatus obdStatus = obd_->snapshot(nowMs);
        d.obdConnected = obdStatus.connected;
        if (obdStatus.speedValid && obdStatus.speedAgeMs < 3000) {
            d.speedValid = true;
            d.speedMph   = obdStatus.speedMph;
        }
    }

    // BLE connection state + RSSI
    d.v1Connected          = ble_->isConnected();
    d.proxyEnabled         = s.proxyBLE;
    d.proxyClientConnected = ble_->isProxyClientConnected();
    if (d.v1Connected) {
        const int rssi = ble_->getConnectionRssi();
        if (rssi != 0) {
            d.v1RssiValid = true;
            d.v1Rssi      = static_cast<int8_t>(rssi);
        }
    }

    // WiFi AP
    d.wifiApActive = wifiManager.isSetupModeActive();

    // Battery
    d.hasBattery  = batteryManager.hasBattery();
    d.batteryPct  = batteryManager.getPercentage();

    // Active profile slot
    d.activeSlot = s.activeSlot;
    const String& name = (s.activeSlot == 0) ? s.slot0Name
                       : (s.activeSlot == 1) ? s.slot1Name
                                             : s.slot2Name;
    strncpy(d.slotName, name.c_str(), sizeof(d.slotName) - 1);
    d.slotName[sizeof(d.slotName) - 1] = '\0';

    // Mode character from last display state
    if (parser_) {
        const DisplayState ds = parser_->getDisplayState();
        d.modeChar = ds.bogeyCounterChar;
        d.muted    = ds.muted;

        // Last alert: prefer a live alert; fall back to cache
        if (parser_->hasAlerts()) {
            AlertData pri;
            if (parser_->getRenderablePriorityAlert(pri)) {
                d.hasLastAlert   = true;
                d.lastBand       = pri.band;
                d.lastFreqMhz    = pri.frequency;
                d.lastDirection  = pri.direction;
                d.lastAlertAgeMs = 0;  // currently active
                lastAlertSeenMs_ = nowMs;
                // Update the persistent cache
                lastAlertCache_.hasLastAlert  = true;
                lastAlertCache_.lastBand      = pri.band;
                lastAlertCache_.lastFreqMhz   = pri.frequency;
                lastAlertCache_.lastDirection = pri.direction;
            }
        } else if (lastAlertCache_.hasLastAlert) {
            d.hasLastAlert  = lastAlertCache_.hasLastAlert;
            d.lastBand      = lastAlertCache_.lastBand;
            d.lastFreqMhz   = lastAlertCache_.lastFreqMhz;
            d.lastDirection = lastAlertCache_.lastDirection;
            if (lastAlertSeenMs_ != UINT32_MAX) {
                d.lastAlertAgeMs = nowMs - lastAlertSeenMs_;
            }
        }
    }

    // Phone companion data
    if (phone_ && phone_->isValid(nowMs)) {
        const PhoneCompanionData& pd = phone_->data();
        d.phoneDataValid  = true;
        d.phoneAgeMs      = phone_->ageMs(nowMs);
        d.hasHeading      = pd.hasHeading;
        d.heading         = pd.heading;
        d.hasGpsAccuracy  = pd.hasGpsAccuracy;
        d.gpsAccuracyM    = pd.gpsAccuracyM;
        d.hasPhoneBattery = pd.hasPhoneBattery;
        d.phoneBattery    = pd.phoneBattery;
        if (pd.hasRoadName) {
            d.hasRoadName = true;
            strncpy(d.roadName, pd.roadName, sizeof(d.roadName) - 1);
        }
    }

    return d;
}

void DashboardModule::cycleNext() {
    const auto next = static_cast<uint8_t>(activeScreen_) + 1;
    activeScreen_ = static_cast<IdleScreen>(
        next > static_cast<uint8_t>(IdleScreen::Stealth) ? 0 : next);
}

TuningData DashboardModule::tuningSnapshot(uint32_t nowMs) const {
    TuningData d;
    if (!settings_ || !ble_ || !parser_) return d;

    const V1Settings& s = settings_->get();
    d.activeSlot  = s.activeSlot;
    d.v1Connected = ble_->isConnected();

    const String& name = (s.activeSlot == 0) ? s.slot0Name
                       : (s.activeSlot == 1) ? s.slot1Name
                                             : s.slot2Name;
    strncpy(d.slotName, name.c_str(), sizeof(d.slotName) - 1);
    d.slotName[sizeof(d.slotName) - 1] = '\0';

    const DisplayState ds = parser_->getDisplayState();
    d.muted = ds.muted;

    if (parser_->hasAlerts()) {
        AlertData pri;
        if (parser_->getRenderablePriorityAlert(pri)) {
            d.hasAlert   = true;
            d.band       = pri.band;
            d.freqMHz    = pri.frequency;
            d.signalBars = pri.frontStrength;  // 0-6; scale to 0-8 display range
            d.direction  = pri.direction;
            d.durationMs = alertOnsetMs_ > 0 ? (nowMs - alertOnsetMs_) : 0;
        }
    }

    // Copy frequency history snapshot (most recent first)
    d.freqHistCount = freqHistCount_;
    for (uint8_t i = 0; i < freqHistCount_ && i < 3; ++i) {
        // freqHistHead_ points to the *next* write slot; walk backwards
        const uint8_t idx = (freqHistHead_ + 3 - 1 - i) % 3;
        d.freqHistory[i] = freqHistory_[idx];
    }
    return d;
}

StealthData DashboardModule::stealthSnapshot(uint32_t nowMs) const {
    StealthData d;
    if (!ble_ || !parser_) return d;

    if (obd_) {
        const ObdRuntimeStatus obdStatus = obd_->snapshot(nowMs);
        if (obdStatus.speedValid && obdStatus.speedAgeMs < 3000) {
            d.speedValid = true;
            d.speedMph   = obdStatus.speedMph;
        }
    }

    const DisplayState ds = parser_->getDisplayState();
    d.muted = ds.muted;

    if (parser_->hasAlerts()) {
        AlertData pri;
        if (parser_->getRenderablePriorityAlert(pri)) {
            d.hasAlert  = true;
            d.band      = pri.band;
            d.direction = pri.direction;
            d.freqMHz   = pri.frequency;
        }
    }

    d.flashFrame = static_cast<int32_t>(nowMs - stealthFlashUntilMs_) < 0;
    return d;
}

bool DashboardModule::renderIfActive(uint32_t nowMs) {
    if (activeScreen_ == IdleScreen::Off || !display_) return false;
    if (nowMs - lastRedrawMs_ < REDRAW_INTERVAL_MS) return false;

    // Track alert onset / clear transitions.
    const bool hasAlert = parser_ && parser_->hasAlerts();
    const bool alertOnset = hasAlert && !prevHasAlert_;
    const bool alertClear = !hasAlert && prevHasAlert_;

    if (alertOnset) {
        alertOnsetMs_ = nowMs;
        alertFiredOnIdleScreen_ = true;

        // Arm stealth flash for one render cycle (~250ms).
        stealthFlashUntilMs_ = nowMs + REDRAW_INTERVAL_MS;

        // Record frequency into history ring (de-dup consecutive same freq).
        if (parser_) {
            AlertData pri;
            if (parser_->getRenderablePriorityAlert(pri) && pri.frequency != lastRecordedFreq_) {
                freqHistory_[freqHistHead_] = pri.frequency;
                freqHistHead_ = (freqHistHead_ + 1) % 3;
                if (freqHistCount_ < 3) ++freqHistCount_;
                lastRecordedFreq_ = pri.frequency;
            }
        }
    }

    if (!hasAlert) alertOnsetMs_ = 0;

    // Auto-return to Off when the alert that triggered the idle screen clears.
    if (alertClear && alertFiredOnIdleScreen_) {
        alertFiredOnIdleScreen_ = false;
        activeScreen_ = IdleScreen::Off;
        prevHasAlert_ = false;
        return false;  // nothing to render — caller will draw normal radar view
    }

    prevHasAlert_ = hasAlert;
    lastRedrawMs_ = nowMs;

    switch (activeScreen_) {
        case IdleScreen::Dashboard: {
            const DashboardData data = snapshot(nowMs);
            display_->showDashboard(data);
            break;
        }
        case IdleScreen::Tuning: {
            const TuningData data = tuningSnapshot(nowMs);
            display_->showTuning(data);
            break;
        }
        case IdleScreen::Stealth: {
            const StealthData data = stealthSnapshot(nowMs);
            display_->showStealth(data);
            break;
        }
        default:
            break;
    }
    return true;
}
