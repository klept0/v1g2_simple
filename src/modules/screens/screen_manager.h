#pragma once
#include <Arduino.h>

enum class ScreenType : uint8_t {
    RADAR   = 0,
    JBV1    = 1,
    HISTORY = 2,
    DIAG    = 3,
    CLOCK   = 4,
    COUNT   = 5
};

class V1Display;
struct DisplayBleContext;

class ScreenManager {
public:
    ScreenType current() const { return current_; }
    void next();
    void previous();
    void set(ScreenType s);
    void forceRadar();
    bool isRadar() const { return current_ == ScreenType::RADAR; }

    // Called each loop with alert-active flag; auto-forces Radar when alert starts
    void tick(bool hasAlert, uint32_t nowMs);

    // Render the active non-radar screen
    void render(V1Display& disp, const DisplayBleContext& ble);

private:
    ScreenType current_ = ScreenType::RADAR;
    bool lastHadAlert_ = false;
};

