#pragma once

#include <cstdint>

class SpeedSourceSelector;
class SettingsManager;

// Determines whether configuration changes should be blocked because the
// vehicle is moving above the configured threshold.  Speed unavailability
// is treated as "not locked" (safe fallback per spec).
class DrivingSafetyLockout {
public:
    void begin(SpeedSourceSelector* speed, SettingsManager* settings);

    // Returns true when the vehicle is definitely moving above the threshold.
    // Returns false when speed is unknown/unavailable (safe fallback).
    bool isLocked() const;

    float   currentSpeedMph() const;
    bool    isSpeedValid()    const;

private:
    SpeedSourceSelector* speed_    = nullptr;
    SettingsManager*     settings_ = nullptr;
};
