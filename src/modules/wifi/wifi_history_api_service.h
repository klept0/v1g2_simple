#pragma once

#include <WebServer.h>
#include <cstdint>

class EncounterHistory;

namespace WifiHistoryApiService {

struct Runtime {
    EncounterHistory* (*getHistory)(void* ctx) = nullptr;
    bool              (*checkRateLimit)(void* ctx) = nullptr;
    void*             ctx = nullptr;
};

void handleApiGet(WebServer& server, const Runtime& runtime);
void handleApiClear(WebServer& server, const Runtime& runtime);
void handleApiCsvExport(WebServer& server, const Runtime& runtime);
void handleApiMarkFalse(WebServer& server, const Runtime& runtime);

}  // namespace WifiHistoryApiService
