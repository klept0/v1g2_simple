#pragma once

#include <WebServer.h>

class SettingsManager;

namespace WifiWizardApiService {

struct Runtime {
    SettingsManager* (*getSettings)(void* ctx) = nullptr;
    void*            ctx = nullptr;
};

// GET /api/setup/wizard — returns {done, step}
void handleApiGet(WebServer& server, const Runtime& runtime);

// POST /api/setup/wizard — accepts {done?, step?}, saves, returns updated state
void handleApiSave(WebServer& server, const Runtime& runtime);

}  // namespace WifiWizardApiService
