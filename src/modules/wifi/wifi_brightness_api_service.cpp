#include "wifi_brightness_api_service.h"
#include "../../settings.h"
#include "../brightness/smart_brightness_engine.h"

namespace WifiBrightnessApiService {

void handleApiGet(WebServer& server, const Runtime& runtime) {
    if (!runtime.getSettings || !runtime.ctx) {
        server.send(500, "application/json", "{\"error\":\"No settings\"}");
        return;
    }
    SettingsManager* s = runtime.getSettings(runtime.ctx);
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"enabled\":%s,\"brtDay\":%u,\"brtNight\":%u,"
        "\"brtIdle\":%u,\"brtAlert\":%u,\"brtMute\":%u,\"brtIdleSec\":%u}",
        s->isBrightEngEnabled() ? "true" : "false",
        s->getBrtDay(), s->getBrtNight(),
        s->getBrtIdle(), s->getBrtAlert(), s->getBrtMute(),
        s->getBrtIdleSec());
    server.send(200, "application/json", buf);
}

void handleApiSave(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) {
        server.send(429, "application/json", "{\"error\":\"Rate limit\"}");
        return;
    }
    SettingsManager* s = runtime.getSettings ? runtime.getSettings(runtime.ctx) : nullptr;
    if (!s) { server.send(500, "application/json", "{\"error\":\"No settings\"}"); return; }

    if (server.hasArg("enabled"))   s->setBrightEngEnabled(server.arg("enabled") == "1");
    if (server.hasArg("brtDay"))    s->setBrtDay(static_cast<uint8_t>(server.arg("brtDay").toInt()));
    if (server.hasArg("brtNight"))  s->setBrtNight(static_cast<uint8_t>(server.arg("brtNight").toInt()));
    if (server.hasArg("brtIdle"))   s->setBrtIdle(static_cast<uint8_t>(server.arg("brtIdle").toInt()));
    if (server.hasArg("brtAlert"))  s->setBrtAlert(static_cast<uint8_t>(server.arg("brtAlert").toInt()));
    if (server.hasArg("brtMute"))   s->setBrtMute(static_cast<uint8_t>(server.arg("brtMute").toInt()));
    if (server.hasArg("brtIdleSec")) s->setBrtIdleSec(static_cast<uint16_t>(server.arg("brtIdleSec").toInt()));

    // Apply day-base to engine immediately if engine reference provided
    if (runtime.getEngine && runtime.ctx) {
        SmartBrightnessEngine* eng = runtime.getEngine(runtime.ctx);
        if (eng) eng->setDayBase(s->getBrtDay(), static_cast<uint32_t>(millis()));
    }

    server.send(200, "application/json", "{\"ok\":true}");
}

}  // namespace WifiBrightnessApiService
