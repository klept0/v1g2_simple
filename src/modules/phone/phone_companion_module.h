#pragma once

#include <stdint.h>

// Maximum age before phone companion data is considered stale.
static constexpr uint32_t PHONE_DATA_STALE_MS = 5000;

struct PhoneCompanionData {
    // All optional fields; use has* flags to check presence.
    bool     hasSpeed       = false;
    float    speedMph       = 0.0f;

    bool     hasHeading     = false;
    uint16_t heading        = 0;        // degrees, 0-359

    bool     hasGpsAccuracy = false;
    float    gpsAccuracyM   = 0.0f;

    bool     hasRoadName    = false;
    char     roadName[33]   = {};       // 32 chars + NUL

    bool     hasPhoneBattery = false;
    uint8_t  phoneBattery    = 0;       // percent 0-100

    char     source[32]     = {};       // required: "tasker", "automate", etc.

    uint32_t receivedMs     = 0;        // millis() when data arrived
};

class PhoneCompanionModule {
public:
    // Store a new reading from the companion app.
    void update(const PhoneCompanionData& data);

    // Returns true when data is present and not stale.
    bool isValid(uint32_t nowMs) const;

    // Returns stale-safe speed reading.
    bool getFreshSpeed(uint32_t nowMs, float& speedMphOut) const;

    // Full data snapshot (caller should check isValid first).
    const PhoneCompanionData& data() const { return data_; }

    // Age of the most-recent update in milliseconds (UINT32_MAX if never received).
    uint32_t ageMs(uint32_t nowMs) const;

private:
    PhoneCompanionData data_{};
    bool               hasData_ = false;
};
