#pragma once

#include <Arduino.h>
#include "packet_parser_types.h"

class V1Display;
class PacketParser;
class SettingsManager;
class V1BLEClient;
class ObdRuntimeModule;
class PhoneCompanionModule;

// Data needed to render the V1 tuning / sweep-validation screen.
struct TuningData {
    bool      hasAlert    = false;
    Band      band        = BAND_NONE;
    uint32_t  freqMHz     = 0;      // frequency × 1000 (matches AlertData::frequency)
    uint8_t   signalBars  = 0;      // 0-6 (from AlertData::frontStrength)
    Direction direction   = DIR_NONE;
    uint32_t  durationMs  = 0;      // how long the current alert has been active
    int       activeSlot  = 0;
    char      slotName[12] = "DEFAULT";
    bool      muted       = false;
    bool      v1Connected = false;
};

// Data needed to render the stealth night-mode HUD.
struct StealthData {
    bool      speedValid = false;
    float     speedMph   = 0.0f;
    bool      hasAlert   = false;
    Band      band       = BAND_NONE;
    Direction direction  = DIR_NONE;
    uint32_t  freqMHz    = 0;
    bool      muted      = false;
};

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

    // Phone companion data (valid when phoneDataValid == true)
    bool     phoneDataValid  = false;
    bool     hasHeading      = false;
    uint16_t heading         = 0;
    bool     hasGpsAccuracy  = false;
    float    gpsAccuracyM    = 0.0f;
    bool     hasRoadName     = false;
    char     roadName[33]    = {};
    bool     hasPhoneBattery = false;
    uint8_t  phoneBattery    = 0;
    uint32_t phoneAgeMs      = UINT32_MAX;
};

// Idle screen modes cycled by single tap (Off → Dashboard → Tuning → Stealth → Off).
enum class IdleScreen : uint8_t { Off, Dashboard, Tuning, Stealth };

class DashboardModule {
public:

    void begin(V1Display* display,
               PacketParser* parser,
               SettingsManager* settings,
               V1BLEClient* bleClient,
               ObdRuntimeModule* obd,
               PhoneCompanionModule* phone = nullptr);

    bool isActive() const { return activeScreen_ != IdleScreen::Off; }
    void setActive(bool v) { if (!v) activeScreen_ = IdleScreen::Off; }
    // Advance to the next idle screen (Off→Dashboard→Tuning→Stealth→Off).
    void cycleNext();
    IdleScreen activeScreen() const { return activeScreen_; }

    // Build snapshots from the injected dependencies.
    DashboardData snapshot(uint32_t nowMs) const;
    TuningData    tuningSnapshot(uint32_t nowMs) const;
    StealthData   stealthSnapshot(uint32_t nowMs) const;

    // If active and sufficient time has passed, redraw the current screen.
    // Returns true if a redraw was performed.
    bool renderIfActive(uint32_t nowMs);

    static constexpr uint32_t REDRAW_INTERVAL_MS = 250;

private:
    IdleScreen activeScreen_ = IdleScreen::Off;

    V1Display*            display_  = nullptr;
    PacketParser*         parser_   = nullptr;
    SettingsManager*      settings_ = nullptr;
    V1BLEClient*          ble_      = nullptr;
    ObdRuntimeModule*     obd_      = nullptr;
    PhoneCompanionModule* phone_    = nullptr;

    uint32_t lastRedrawMs_  = 0;
    uint32_t alertOnsetMs_  = 0;  // when the current alert sequence started
    bool     prevHasAlert_  = false;

    // Track last alert across cycles so it persists on the dashboard.
    mutable DashboardData lastAlertCache_{};
};
