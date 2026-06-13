#include "wifi_jbv1_api_service.h"
#include "modules/services/jbv1_client.h"
#include <ArduinoJson.h>
#include <Arduino.h>

namespace WifiJBV1ApiService {

void handleApiUpdate(WebServer& server) {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"No body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (doc["speed"].is<int>())        g_jbv1.speed        = doc["speed"].as<int>();
    if (doc["speedLimit"].is<int>())   g_jbv1.speedLimit   = doc["speedLimit"].as<int>();
    if (doc["satellites"].is<int>())   g_jbv1.satellites   = doc["satellites"].as<int>();
    if (doc["gpsAccuracy"].is<int>())  g_jbv1.gpsAccuracy  = doc["gpsAccuracy"].as<int>();

    if (doc["heading"].is<const char*>()) {
        const char* h = doc["heading"].as<const char*>();
        strncpy(g_jbv1.heading, h ? h : "--", sizeof(g_jbv1.heading) - 1);
        g_jbv1.heading[sizeof(g_jbv1.heading) - 1] = '\0';
    }

    g_jbv1.lastUpdateMs = millis();

    server.send(200, "application/json", "{\"ok\":true}");
}

}  // namespace WifiJBV1ApiService
