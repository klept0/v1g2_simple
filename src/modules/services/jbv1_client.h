#pragma once
#include <Arduino.h>

struct JBV1Data {
    int speed       = 0;
    int speedLimit  = 0;
    int satellites  = 0;
    int gpsAccuracy = 0;
    char heading[4] = "--";
    uint32_t lastUpdateMs = 0;
    bool valid() const;
};

// Call from main loop to age out stale data
void jbv1_tick(uint32_t nowMs);
