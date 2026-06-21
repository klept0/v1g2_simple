#pragma once

#include <WebServer.h>
#include <cstdint>
#include "../../settings.h"

namespace WifiDriveModeApiService {

struct Runtime {
    DrivingMode        (*getActiveMode)(void* ctx)                   = nullptr;
    const DrivingModeConfig& (*getModeConfig)(int mode, void* ctx)   = nullptr;
    void               (*applyMode)(DrivingMode mode, void* ctx)     = nullptr;
    void               (*setModeConfig)(int mode, const DrivingModeConfig& cfg, void* ctx) = nullptr;
    bool               (*checkRateLimit)(void* ctx)                  = nullptr;
    void*              ctx = nullptr;
};

void handleApiGet(WebServer& server, const Runtime& runtime);
void handleApiSave(WebServer& server, const Runtime& runtime);

}  // namespace WifiDriveModeApiService
