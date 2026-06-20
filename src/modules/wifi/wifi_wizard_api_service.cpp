#include "wifi_wizard_api_service.h"
#include "settings.h"
#include <ArduinoJson.h>

namespace WifiWizardApiService {

void handleApiGet(WebServer& server, const Runtime& runtime) {
    if (!runtime.getSettings || !runtime.ctx) {
        server.send(500, "application/json", "{\"error\":\"Unavailable\"}");
        return;
    }
    SettingsManager* s = runtime.getSettings(runtime.ctx);
    JsonDocument doc;
    doc["done"] = s->isWzdDone();
    doc["step"] = s->getWzdStep();
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

void handleApiSave(WebServer& server, const Runtime& runtime) {
    if (!runtime.getSettings || !runtime.ctx) {
        server.send(500, "application/json", "{\"error\":\"Unavailable\"}");
        return;
    }
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, body)) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    SettingsManager* s = runtime.getSettings(runtime.ctx);

    if (doc["step"].is<int>()) {
        int step = doc["step"].as<int>();
        if (step >= 0 && step <= 7) s->setWzdStep((uint8_t)step);
    }
    if (doc["done"].is<bool>()) {
        s->setWzdDone(doc["done"].as<bool>());
    }

    JsonDocument resp;
    resp["done"] = s->isWzdDone();
    resp["step"] = s->getWzdStep();
    String out;
    serializeJson(resp, out);
    server.send(200, "application/json", out);
}

}  // namespace WifiWizardApiService
