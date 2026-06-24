#pragma once

#include "../src/settings.h"

struct MainRuntimeState {
    bool bootReady = false;
    unsigned long bootReadyDeadlineMs = 0;
    bool bootSplashHoldActive = false;
    unsigned long bootSplashHoldUntilMs = 0;
    bool initialScanningScreenShown = false;
    unsigned long activeScanScreenDwellMs = 0;
    unsigned long v1ConnectedAtMs = 0;
    bool wifiAutoStartDone = false;
    unsigned long lastLoopUs = 0;
    bool v1InfoSavedThisConnect = false;  // true once v1_info.json written after current connect
    DrivingMode lastDrivingMode = DrivingMode::Normal;  // detect mode changes in main loop
};

