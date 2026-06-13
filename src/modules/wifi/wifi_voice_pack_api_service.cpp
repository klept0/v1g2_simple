#include "wifi_voice_pack_api_service.h"
#include "wifi_api_response.h"
#include "wifi_json_document.h"
#include "audio_beep.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Arduino.h>

namespace WifiVoicePackApiService {

static constexpr const char* AUDIO_ROOT = "/audio";
static constexpr size_t PACK_NAME_MAX   = 16;
static constexpr size_t CLIP_NAME_MAX   = 20;

// Returns true if every character is alphanumeric or underscore and length is 1-16
static bool isValidPackName(const String& name) {
    if (name.length() == 0 || name.length() > PACK_NAME_MAX) return false;
    for (size_t i = 0; i < name.length(); i++) {
        char c = name[i];
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

// Returns true if the filename ends in .mul and has no path separators
static bool isValidClipName(const String& name) {
    if (name.length() == 0 || name.length() > CLIP_NAME_MAX) return false;
    if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) return false;
    return name.endsWith(".mul");
}

// Count .mul files in a directory
static int countMulFiles(const char* dirPath) {
    File dir = LittleFS.open(dirPath);
    if (!dir || !dir.isDirectory()) return 0;
    int count = 0;
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String n = String(entry.name());
            if (n.endsWith(".mul")) count++;
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return count;
}

// Recursively delete all files in a directory, then remove the directory
static bool deleteDirRecursive(const char* dirPath) {
    File dir = LittleFS.open(dirPath);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
        char fullPath[64];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry.name());
        entry.close();
        LittleFS.remove(fullPath);
        entry = dir.openNextFile();
    }
    dir.close();
    return LittleFS.rmdir(dirPath);
}

// ── Handlers ──────────────────────────────────────────────────────────────

void handleApiList(WebServer& server, const Runtime& runtime) {
    (void)runtime;
    WifiJson::Document doc;
    doc["activePack"] = audio_get_active_pack();

    JsonArray packs = doc["packs"].to<JsonArray>();

    // Built-in default (the /audio root itself)
    {
        JsonObject def = packs.add<JsonObject>();
        def["name"]       = "";
        def["label"]      = "Default";
        def["clipCount"]  = countMulFiles(AUDIO_ROOT);
        def["active"]     = (audio_get_active_pack()[0] == '\0');
    }

    // Custom packs: subdirectories of /audio/
    File audioDir = LittleFS.open(AUDIO_ROOT);
    if (audioDir && audioDir.isDirectory()) {
        File entry = audioDir.openNextFile();
        while (entry) {
            if (entry.isDirectory()) {
                String packName = String(entry.name());
                entry.close();
                if (isValidPackName(packName)) {
                    char packPath[48];
                    snprintf(packPath, sizeof(packPath), "%s/%s", AUDIO_ROOT, packName.c_str());
                    JsonObject p = packs.add<JsonObject>();
                    p["name"]      = packName;
                    p["label"]     = packName;
                    p["clipCount"] = countMulFiles(packPath);
                    p["active"]    = (packName == audio_get_active_pack());
                }
            } else {
                entry.close();
            }
            entry = audioDir.openNextFile();
        }
        audioDir.close();
    }

    WifiApiResponse::sendJsonDocument(server, 200, doc);
}

void handleApiActivate(WebServer& server,
                       const Runtime& runtime,
                       bool (*checkRateLimit)(void* ctx), void* rateLimitCtx) {
    if (checkRateLimit && !checkRateLimit(rateLimitCtx)) return;

    String packName = server.hasArg("pack") ? server.arg("pack") : "";

    // Empty = revert to default; otherwise validate
    if (packName.length() > 0 && !isValidPackName(packName)) {
        server.send(400, "application/json", "{\"error\":\"Invalid pack name\"}");
        return;
    }

    // Verify the pack directory exists (skip for default)
    if (packName.length() > 0) {
        char packPath[48];
        snprintf(packPath, sizeof(packPath), "%s/%s", AUDIO_ROOT, packName.c_str());
        if (!LittleFS.exists(packPath)) {
            server.send(404, "application/json", "{\"error\":\"Pack not found\"}");
            return;
        }
    }

    // Apply immediately to the audio engine
    audio_set_voice_pack(packName.c_str());

    // Persist via callback (caller wires this to settingsManager.setActiveVoicePack)
    if (runtime.onPackActivated) {
        runtime.onPackActivated(packName.c_str(), runtime.ctx);
    }

    server.send(200, "application/json", "{\"success\":true}");
}

void handleApiDelete(WebServer& server,
                     const Runtime& runtime,
                     bool (*checkRateLimit)(void* ctx), void* rateLimitCtx) {
    (void)runtime;
    if (checkRateLimit && !checkRateLimit(rateLimitCtx)) return;

    String packName = server.hasArg("pack") ? server.arg("pack") : "";
    if (!isValidPackName(packName)) {
        server.send(400, "application/json", "{\"error\":\"Invalid pack name\"}");
        return;
    }

    char packPath[48];
    snprintf(packPath, sizeof(packPath), "%s/%s", AUDIO_ROOT, packName.c_str());

    if (!LittleFS.exists(packPath)) {
        server.send(404, "application/json", "{\"error\":\"Pack not found\"}");
        return;
    }

    // If this pack is currently active, revert to default before deleting
    if (packName == audio_get_active_pack()) {
        audio_set_voice_pack("");
        if (runtime.onPackActivated) runtime.onPackActivated("", runtime.ctx);
    }

    if (!deleteDirRecursive(packPath)) {
        server.send(500, "application/json", "{\"error\":\"Delete failed\"}");
        return;
    }

    server.send(200, "application/json", "{\"success\":true}");
}

// Persistent upload state across the multipart callbacks
static File s_uploadFile;
static bool s_uploadOk = false;

void handleApiUploadDone(WebServer& server, const Runtime& /*runtime*/) {
    if (s_uploadOk) {
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Upload failed or invalid\"}");
    }
    s_uploadOk = false;
}

void handleApiUploadFile(WebServer& server) {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        s_uploadOk = false;

        // Pack name from query param
        String packName = server.hasArg("pack") ? server.arg("pack") : "";
        if (!isValidPackName(packName)) {
            Serial.printf("[VoicePack] Upload rejected: invalid pack name '%s'\n", packName.c_str());
            return;
        }

        // Clip filename from the upload
        String clipName = String(upload.filename.c_str());
        // strip any path the browser may send
        int slash = clipName.lastIndexOf('/');
        if (slash >= 0) clipName = clipName.substring(slash + 1);
        if (!isValidClipName(clipName)) {
            Serial.printf("[VoicePack] Upload rejected: invalid clip name '%s'\n", clipName.c_str());
            return;
        }

        // Ensure pack directory exists
        char packPath[48];
        snprintf(packPath, sizeof(packPath), "%s/%s", AUDIO_ROOT, packName.c_str());
        if (!LittleFS.exists(packPath)) {
            if (!LittleFS.mkdir(packPath)) {
                Serial.printf("[VoicePack] Failed to create pack dir: %s\n", packPath);
                return;
            }
        }

        char filePath[64];
        snprintf(filePath, sizeof(filePath), "%s/%s/%s",
                 AUDIO_ROOT, packName.c_str(), clipName.c_str());
        s_uploadFile = LittleFS.open(filePath, "w");
        if (!s_uploadFile) {
            Serial.printf("[VoicePack] Failed to open for write: %s\n", filePath);
            return;
        }
        Serial.printf("[VoicePack] Uploading: %s\n", filePath);

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (s_uploadFile) {
            s_uploadFile.write(upload.buf, upload.currentSize);
        }

    } else if (upload.status == UPLOAD_FILE_END) {
        if (s_uploadFile) {
            s_uploadFile.close();
            s_uploadOk = true;
            Serial.printf("[VoicePack] Upload complete: %u bytes\n", upload.totalSize);
        }

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (s_uploadFile) {
            s_uploadFile.close();
            s_uploadFile = File();
        }
        s_uploadOk = false;
    }
}

}  // namespace WifiVoicePackApiService
