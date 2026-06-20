#pragma once

#include <Arduino.h>
#include "packet_parser_types.h"

class V1Display;
class PacketParser;
class SettingsManager;
class V1BLEClient;
class ObdRuntimeModule;

// All data needed to render the driving dashboard in one snapshot.
struct DashboardData {
    // Speed
    bool    speedValid    = false;
    float   speedMph      = 0.0f;

    // Connection states
    bool    v1Connected          = false;
    bool    proxyEnabled         = false;
    bool    proxyClientConnected = false;
    bool    wifiApActive         = false;
    bool    obdConnected         = false;

    // Battery (only meaningful when hasBattery == true)
    bool    hasBattery   = false;
    uint8_t batteryPct   = 0;

    // Active profile/mode
    int  activeSlot = 0;
    char slotName[12] = "DEFAULT";   // "DEFAULT" / "HIGHWAY" / "COMFORT"
    char modeChar = 0;               // 'A'=All Bogeys, 'L'=Logic, 'c'=Adv Logic, 0=unknown
    bool muted = false;

    // Last alert summary (zeroed when no history yet)
    bool      hasLastAlert  = false;
    Band      lastBand      = BAND_NONE;
    uint32_t  lastFreqMhz   = 0;
    Direction lastDirection = DIR_NONE;
};

class DashboardModule {
public:
    void begin(V1Display* display,
               PacketParser* parser,
               SettingsManager* settings,
               V1BLEClient* bleClient,
               ObdRuntimeModule* obd);

    bool isActive() const { return active_; }
    void toggle() { active_ = !active_; }
    void setActive(bool v) { active_ = v; }

    // Build a DashboardData snapshot from the injected dependencies.
    DashboardData snapshot(uint32_t nowMs) const;

    // If active and sufficient time has passed, redraw the dashboard.
    // Returns true if a redraw was performed.
    bool renderIfActive(uint32_t nowMs);

    static constexpr uint32_t REDRAW_INTERVAL_MS = 250;

private:
    bool active_ = false;

    V1Display*       display_  = nullptr;
    PacketParser*    parser_   = nullptr;
    SettingsManager* settings_ = nullptr;
    V1BLEClient*     ble_      = nullptr;
    ObdRuntimeModule* obd_     = nullptr;

    uint32_t lastRedrawMs_ = 0;

    // Track last alert across cycles so it persists on the dashboard.
    mutable DashboardData lastAlertCache_{};
};
