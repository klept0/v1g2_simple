#pragma once

#include <WebServer.h>

class DrivingSafetyLockout;
class SettingsManager;

namespace WifiLockoutApiService {

struct Runtime {
    DrivingSafetyLockout* (*getLockout)(void* ctx)  = nullptr;
    SettingsManager*      (*getSettings)(void* ctx) = nullptr;
    bool                  (*checkRateLimit)(void* ctx) = nullptr;
    void*                 ctx = nullptr;
};

void handleApiGet(WebServer& server, const Runtime& runtime);
void handleApiSave(WebServer& server, const Runtime& runtime);

// Helper: send 423 Locked and return true when the vehicle is moving.
// Call at the start of any handler that should be blocked while driving.
bool sendLockoutIfLocked(WebServer& server, const Runtime& runtime);

}  // namespace WifiLockoutApiService
