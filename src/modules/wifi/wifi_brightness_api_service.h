#pragma once

#include <WebServer.h>

class SettingsManager;
class SmartBrightnessEngine;

namespace WifiBrightnessApiService {

struct Runtime {
    SettingsManager*      (*getSettings)(void* ctx) = nullptr;
    SmartBrightnessEngine* (*getEngine)(void* ctx)  = nullptr;
    bool                  (*checkRateLimit)(void* ctx) = nullptr;
    void*                 ctx = nullptr;
};

void handleApiGet(WebServer& server, const Runtime& runtime);
void handleApiSave(WebServer& server, const Runtime& runtime);

}  // namespace WifiBrightnessApiService
