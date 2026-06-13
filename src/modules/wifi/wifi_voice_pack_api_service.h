#pragma once

#include <WebServer.h>
#include <functional>

namespace WifiVoicePackApiService {

struct Runtime {
    // Called after activating a new pack so the audio engine picks it up immediately
    void (*onPackActivated)(const char* packName, void* ctx) = nullptr;
    void* ctx = nullptr;
};

// GET  /api/audio/voice-packs          — list available packs + active pack
void handleApiList(WebServer& server, const Runtime& runtime);

// POST /api/audio/voice-pack/activate  — body: pack=<name>  (empty = default)
void handleApiActivate(WebServer& server,
                       const Runtime& runtime,
                       bool (*checkRateLimit)(void* ctx), void* rateLimitCtx);

// POST /api/audio/voice-pack/delete    — body: pack=<name>
void handleApiDelete(WebServer& server,
                     const Runtime& runtime,
                     bool (*checkRateLimit)(void* ctx), void* rateLimitCtx);

// POST /api/audio/voice-pack/upload    — multipart; URL param: pack=<name>
// Two-handler pair required by ESP32 WebServer for upload routes:
void handleApiUploadDone(WebServer& server, const Runtime& runtime);
void handleApiUploadFile(WebServer& server);

}  // namespace WifiVoicePackApiService
