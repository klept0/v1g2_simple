#include "dashboard_module.h"
#include "display.h"
#include "packet_parser.h"
#include "settings.h"
#include "ble_client.h"
#include "battery_manager.h"
#include "wifi_manager.h"
#include "modules/obd/obd_runtime_module.h"
#include <string.h>
#include <stdio.h>

void DashboardModule::begin(V1Display* display,
                            PacketParser* parser,
                            SettingsManager* settings,
                            V1BLEClient* bleClient,
                            ObdRuntimeModule* obd) {
    display_  = display;
    parser_   = parser;
    settings_ = settings;
    ble_      = bleClient;
    obd_      = obd;
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

    // BLE connection state
    d.v1Connected          = ble_->isConnected();
    d.proxyEnabled         = s.proxyBLE;
    d.proxyClientConnected = ble_->isProxyClientConnected();

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
                d.hasLastAlert  = true;
                d.lastBand      = pri.band;
                d.lastFreqMhz   = pri.frequency;
                d.lastDirection = pri.direction;
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
        }
    }

    return d;
}

bool DashboardModule::renderIfActive(uint32_t nowMs) {
    if (!active_ || !display_) return false;
    if (nowMs - lastRedrawMs_ < REDRAW_INTERVAL_MS) return false;

    lastRedrawMs_ = nowMs;
    const DashboardData data = snapshot(nowMs);
    display_->showDashboard(data);
    return true;
}
