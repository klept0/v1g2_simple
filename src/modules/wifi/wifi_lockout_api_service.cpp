#include "wifi_lockout_api_service.h"

#include <ArduinoJson.h>
#include <algorithm>

#include "wifi_api_response.h"
#include "modules/safety/driving_safety_lockout.h"
#include "settings.h"

namespace WifiLockoutApiService {

void handleApiGet(WebServer& server, const Runtime& runtime) {
    DrivingSafetyLockout* lockout  = runtime.getLockout  ? runtime.getLockout(runtime.ctx)  : nullptr;
    SettingsManager*      settings = runtime.getSettings ? runtime.getSettings(runtime.ctx) : nullptr;

    if (!lockout || !settings) {
        server.send(500, "application/json", "{\"error\":\"Lockout unavailable\"}");
        return;
    }

    JsonDocument doc;
    doc["enabled"]      = settings->isLockoutEnabled();
    doc["thresholdMph"] = settings->getLockoutThresholdMph();
    doc["isLocked"]     = lockout->isLocked();
    doc["speedMph"]     = lockout->currentSpeedMph();
    doc["speedValid"]   = lockout->isSpeedValid();
    WifiApiResponse::sendJsonDocument(server, 200, doc);
}

void handleApiSave(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) return;

    SettingsManager* settings = runtime.getSettings ? runtime.getSettings(runtime.ctx) : nullptr;
    if (!settings) {
        server.send(500, "application/json", "{\"error\":\"Settings unavailable\"}");
        return;
    }

    Serial.println("[HTTP] POST /api/drive/lockout");

    if (server.hasArg("enabled")) {
        const String& v = server.arg("enabled");
        settings->setLockoutEnabled(v == "true" || v == "1");
    }
    if (server.hasArg("thresholdMph")) {
        int mph = server.arg("thresholdMph").toInt();
        mph = std::max(0, std::min(mph, 50));
        settings->setLockoutThresholdMph(static_cast<uint8_t>(mph));
    }

    server.send(200, "application/json", "{\"ok\":true}");
}

bool sendLockoutIfLocked(WebServer& server, const Runtime& runtime) {
    DrivingSafetyLockout* lockout = runtime.getLockout ? runtime.getLockout(runtime.ctx) : nullptr;
    if (!lockout || !lockout->isLocked()) return false;
    server.send(423, "application/json",
                "{\"error\":\"Unavailable while vehicle is moving\"}");
    return true;
}

}  // namespace WifiLockoutApiService
