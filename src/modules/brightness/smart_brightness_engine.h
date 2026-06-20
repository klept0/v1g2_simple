#pragma once

#include <stdint.h>

class V1Display;
class SettingsManager;

// Manages display brightness in response to alerts, mute state, and idle timeout.
// Call process() every loop iteration; call notifyActivity() on touch events.
class SmartBrightnessEngine {
public:
    void begin(V1Display* display, SettingsManager* settings, uint32_t nowMs);

    // Called each loop. hasAlerts / isMuted reflect current alert state.
    void process(uint32_t nowMs, bool hasAlerts, bool isMuted);

    // Notify that user interacted (touch, button, etc.) to reset idle dim timer.
    void notifyActivity(uint32_t nowMs);

    // Override the day base brightness (e.g. from driving mode switch or API).
    // Re-applies immediately unless an alert override is active.
    void setDayBase(uint8_t brightness, uint32_t nowMs);

private:
    enum class State : uint8_t { DayActive, IdleDim, AlertBoosted, AlertMuted };

    void applyBrightness(uint8_t level);

    V1Display*      display_  = nullptr;
    SettingsManager* settings_ = nullptr;

    State    state_           = State::DayActive;
    uint32_t lastActivityMs_  = 0;
    uint8_t  dayBase_         = 200;  // mirrors brtDay or last driving-mode brightness
    uint8_t  currentLevel_    = 255;  // track last applied value to skip no-ops
    bool     prevHasAlerts_   = false;
    bool     prevIsMuted_     = false;
};
