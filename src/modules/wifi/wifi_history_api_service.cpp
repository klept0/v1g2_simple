#include "wifi_history_api_service.h"

#include <ArduinoJson.h>
#include "wifi_api_response.h"
#include "modules/history/encounter_history.h"
#include "band_utils.h"

namespace WifiHistoryApiService {

static const char* dirStr(Direction d) {
    if (d & DIR_FRONT) return "Front";
    if (d & DIR_REAR)  return "Rear";
    if (d & DIR_SIDE)  return "Side";
    return "Unknown";
}

void handleApiGet(WebServer& server, const Runtime& runtime) {
    EncounterHistory* hist = runtime.getHistory ? runtime.getHistory(runtime.ctx) : nullptr;
    if (!hist) {
        server.send(500, "application/json", "{\"error\":\"History unavailable\"}");
        return;
    }

    static EncounterEntry buf[EncounterHistory::ABSOLUTE_MAX_ENTRIES];
    size_t n = hist->getRecent(buf, EncounterHistory::ABSOLUTE_MAX_ENTRIES);

    // Stream the JSON array directly to avoid one large allocation
    String json = "[";
    for (size_t i = 0; i < n; i++) {
        const auto& e = buf[i];
        if (i > 0) json += ',';
        char tmp[220];
        snprintf(tmp, sizeof(tmp),
            "{\"id\":%lu,\"ts\":%lld,\"band\":\"%s\",\"freq\":%lu"
            ",\"dir\":\"%s\",\"bogeys\":%u,\"bars\":%u,\"speed\":%d"
            ",\"slot\":\"%s\",\"mode\":\"%c\",\"muted\":%s,\"false\":%s}",
            (unsigned long)e.id,
            (long long)e.timestampMs,
            bandName(e.band),
            (unsigned long)e.frequencyMhz,
            dirStr(e.direction),
            (unsigned)e.bogeyCount,
            (unsigned)e.signalBars,
            (int)(e.speedMph + 0.5f),
            e.slotName,
            e.modeChar ? e.modeChar : '?',
            e.muted      ? "true" : "false",
            e.falseAlert ? "true" : "false");
        json += tmp;
    }
    json += ']';
    server.send(200, "application/json", json);
}

void handleApiClear(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) return;
    EncounterHistory* hist = runtime.getHistory ? runtime.getHistory(runtime.ctx) : nullptr;
    if (!hist) {
        server.send(500, "application/json", "{\"error\":\"History unavailable\"}");
        return;
    }
    hist->clearAll();
    server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiCsvExport(WebServer& server, const Runtime& runtime) {
    EncounterHistory* hist = runtime.getHistory ? runtime.getHistory(runtime.ctx) : nullptr;
    if (!hist) {
        server.send(500, "text/plain", "History unavailable");
        return;
    }
    String csv;
    hist->toCsvString(csv);
    server.sendHeader("Content-Disposition", "attachment; filename=\"encounters.csv\"");
    server.send(200, "text/csv", csv);
}

void handleApiMarkFalse(WebServer& server, const Runtime& runtime) {
    if (runtime.checkRateLimit && !runtime.checkRateLimit(runtime.ctx)) return;
    EncounterHistory* hist = runtime.getHistory ? runtime.getHistory(runtime.ctx) : nullptr;
    if (!hist) {
        server.send(500, "application/json", "{\"error\":\"History unavailable\"}");
        return;
    }
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Missing id\"}");
        return;
    }
    uint32_t id = (uint32_t)server.arg("id").toInt();
    bool val = true;
    if (server.hasArg("value")) {
        val = server.arg("value") != "0" && server.arg("value") != "false";
    }
    bool ok = hist->markFalseAlert(id, val);
    if (ok) {
        server.send(200, "application/json", "{\"ok\":true}");
    } else {
        server.send(404, "application/json", "{\"error\":\"Entry not found\"}");
    }
}

}  // namespace WifiHistoryApiService
