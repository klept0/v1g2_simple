#include "wifi_drive_mode_api_service.h"

#include <ArduinoJson.h>
#include <algorithm>

#include "wifi_api_response.h"

namespace WifiDriveModeApiService {

static void serializeModeConfig(JsonObject obj, const DrivingModeConfig& cfg) {
    obj["brightness"]      = cfg.brightness;
    obj["voiceVolume"]     = cfg.voiceVolume;
    obj["alertPersistSec"] = cfg.alertPersistSec;
    obj["priorityArrowOnly"] = cfg.priorityArrowOnly;
}

void handleApiGet(WebServer& server, const Runtime& runtime) {
    if (!runtime.getActiveMode || !runtime.getModeConfig) {
        server.send(500, "application/json", "{\"error\":\"Runtime unavailable\"}");
        return;
    }

    JsonDocument doc;
    doc["activeMode"] = static_cast<int>(runtime.getActiveMode(runtime.ctx));

    JsonArray modes = doc["modes"].to<JsonArray>();
    for (int i = 0; i < kDrivingModeCount; i++) {
        JsonObject obj = modes.add<JsonObject>();
        serializeModeConfig(obj, runtime.getModeConfig(i, runtime.ctx));
    }

    WifiApiResponse::sendJsonDocument(server, 200, doc);
}

void handleApiSave(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) return;

    if (!runtime.applyMode || !runtime.setModeConfig || !runtime.getModeConfig) {
        server.send(500, "application/json", "{\"error\":\"Runtime unavailable\"}");
        return;
    }

    Serial.println("[HTTP] POST /api/drive/mode");

    auto argU8 = [&](const char* key, uint8_t fallback) -> uint8_t {
        if (!server.hasArg(key)) return fallback;
        int v = server.arg(key).toInt();
        return static_cast<uint8_t>(std::max(0, std::min(v, 255)));
    };
    auto argBool = [&](const char* key, bool fallback) -> bool {
        if (!server.hasArg(key)) return fallback;
        return server.arg(key) == "true" || server.arg(key) == "1";
    };

    // Apply per-mode config updates first (they don't apply to runtime yet)
    static const char* brightKeys[]  = {"m0bright","m1bright","m2bright","m3bright"};
    static const char* volKeys[]     = {"m0vol","m1vol","m2vol","m3vol"};
    static const char* persistKeys[] = {"m0persist","m1persist","m2persist","m3persist"};
    static const char* prioKeys[]    = {"m0prio","m1prio","m2prio","m3prio"};

    for (int i = 0; i < kDrivingModeCount; i++) {
        const DrivingModeConfig& existing = runtime.getModeConfig(i, runtime.ctx);
        bool anyChanged = server.hasArg(brightKeys[i])  || server.hasArg(volKeys[i]) ||
                          server.hasArg(persistKeys[i]) || server.hasArg(prioKeys[i]);
        if (!anyChanged) continue;

        DrivingModeConfig updated = existing;
        updated.brightness        = argU8(brightKeys[i],  existing.brightness);
        updated.voiceVolume       = std::min<uint8_t>(100, argU8(volKeys[i], existing.voiceVolume));
        updated.alertPersistSec   = std::min<uint8_t>(5,   argU8(persistKeys[i], existing.alertPersistSec));
        updated.priorityArrowOnly = argBool(prioKeys[i], existing.priorityArrowOnly);
        runtime.setModeConfig(i, updated, runtime.ctx);
    }

    // Switch active mode if requested
    if (server.hasArg("mode")) {
        int modeInt = server.arg("mode").toInt();
        modeInt = std::max(0, std::min(modeInt, kDrivingModeCount - 1));
        runtime.applyMode(static_cast<DrivingMode>(modeInt), runtime.ctx);
    }

    server.send(200, "application/json", "{\"ok\":true}");
}

}  // namespace WifiDriveModeApiService
