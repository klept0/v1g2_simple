#include "wifi_phone_companion_api_service.h"
#include "../phone/phone_companion_module.h"
#include <Arduino.h>
#include <ArduinoJson.h>

namespace WifiPhoneCompanionApiService {

static bool parseMph(const char* unit, float raw, float& out) {
    if (!unit || raw < 0.0f || raw > 500.0f) return false;
    if (strcmp(unit, "kph") == 0 || strcmp(unit, "kmh") == 0) {
        out = raw * 0.621371f;
    } else {
        out = raw;  // assume mph
    }
    return true;
}

void handleApiUpdate(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) {
        server.send(429, "application/json", "{\"error\":\"Rate limit\"}");
        return;
    }
    if (!runtime.getCompanion || !runtime.ctx) {
        server.send(500, "application/json", "{\"error\":\"Unavailable\"}");
        return;
    }

    // Accept JSON body
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // 'source' is required
    if (!doc["source"].is<const char*>() || doc["source"].as<String>().isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"'source' is required\"}");
        return;
    }

    const uint32_t now = runtime.nowMs ? runtime.nowMs(runtime.ctx) : (uint32_t)millis();

    PhoneCompanionData data{};
    data.receivedMs = now;

    strncpy(data.source, doc["source"].as<const char*>(), sizeof(data.source) - 1);

    if (doc["speed"].is<float>() || doc["speed"].is<int>()) {
        float raw = doc["speed"].as<float>();
        const char* unit = doc["speed_unit"].is<const char*>()
            ? doc["speed_unit"].as<const char*>() : "mph";
        float mph = 0.0f;
        if (parseMph(unit, raw, mph)) {
            data.hasSpeed = true;
            data.speedMph = mph;
        }
    }

    if (doc["heading"].is<int>() || doc["heading"].is<float>()) {
        int h = doc["heading"].as<int>();
        if (h >= 0 && h <= 359) {
            data.hasHeading = true;
            data.heading = (uint16_t)h;
        }
    }

    if (doc["gps_accuracy"].is<float>() || doc["gps_accuracy"].is<int>()) {
        float acc = doc["gps_accuracy"].as<float>();
        if (acc >= 0.0f) {
            data.hasGpsAccuracy = true;
            data.gpsAccuracyM = acc;
        }
    }

    if (doc["road_name"].is<const char*>()) {
        const char* rn = doc["road_name"].as<const char*>();
        if (rn && *rn) {
            data.hasRoadName = true;
            strncpy(data.roadName, rn, sizeof(data.roadName) - 1);
        }
    }

    if (doc["phone_battery"].is<int>() || doc["phone_battery"].is<float>()) {
        int batt = doc["phone_battery"].as<int>();
        if (batt >= 0 && batt <= 100) {
            data.hasPhoneBattery = true;
            data.phoneBattery = (uint8_t)batt;
        }
    }

    runtime.getCompanion(runtime.ctx)->update(data);
    server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiStatus(WebServer& server, const Runtime& runtime) {
    if (!runtime.getCompanion || !runtime.ctx) {
        server.send(500, "application/json", "{\"error\":\"Unavailable\"}");
        return;
    }
    const uint32_t now = runtime.nowMs ? runtime.nowMs(runtime.ctx) : (uint32_t)millis();
    PhoneCompanionModule* mod = runtime.getCompanion(runtime.ctx);
    const bool valid = mod->isValid(now);
    const PhoneCompanionData& d = mod->data();

    JsonDocument doc;
    doc["valid"] = valid;
    doc["ageMs"] = (int32_t)mod->ageMs(now);
    if (valid) {
        doc["source"] = d.source;
        if (d.hasSpeed)        doc["speedMph"]    = d.speedMph;
        if (d.hasHeading)      doc["heading"]      = d.heading;
        if (d.hasGpsAccuracy)  doc["gpsAccuracyM"] = d.gpsAccuracyM;
        if (d.hasRoadName)     doc["roadName"]     = d.roadName;
        if (d.hasPhoneBattery) doc["phoneBattery"] = d.phoneBattery;
    }

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

}  // namespace WifiPhoneCompanionApiService
