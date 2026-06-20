#pragma once
// Mock DashboardModule for native unit tests.
// Only the methods called by DisplayPipelineModule need stubs here.

#include <cstdint>
#include "packet_parser_types.h"

struct DashboardData {
    bool    speedValid    = false;
    float   speedMph      = 0.0f;
    bool    v1Connected          = false;
    bool    proxyEnabled         = false;
    bool    proxyClientConnected = false;
    bool    wifiApActive         = false;
    bool    obdConnected         = false;
    bool    hasBattery   = false;
    uint8_t batteryPct   = 0;
    int  activeSlot = 0;
    char slotName[12] = "DEFAULT";
    char modeChar = 0;
    bool muted = false;
    bool      hasLastAlert  = false;
    Band      lastBand      = BAND_NONE;
    uint32_t  lastFreqMhz   = 0;
    Direction lastDirection = DIR_NONE;
};

class DashboardModule {
public:
    bool active_ = false;

    bool isActive() const { return active_; }
    void toggle() { active_ = !active_; }
    void setActive(bool v) { active_ = v; }

    bool renderIfActive(uint32_t /*nowMs*/) { return false; }
};
