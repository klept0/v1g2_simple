#pragma once

#include <WebServer.h>

class PhoneCompanionModule;

namespace WifiPhoneCompanionApiService {

struct Runtime {
    PhoneCompanionModule* (*getCompanion)(void* ctx) = nullptr;
    uint32_t              (*nowMs)(void* ctx)         = nullptr;
    bool                  (*checkRateLimit)(void* ctx) = nullptr;
    void*                 ctx = nullptr;
};

// POST /api/drive/update — receive companion telemetry
void handleApiUpdate(WebServer& server, const Runtime& runtime);

// GET /api/drive/status — confirm latest companion snapshot
void handleApiStatus(WebServer& server, const Runtime& runtime);

}  // namespace WifiPhoneCompanionApiService
