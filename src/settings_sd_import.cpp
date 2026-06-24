/**
 * SD card import and template generation.
 *
 * importFromSD()            — reads /v1simple_import.json, applies it once,
 *                             renames to /v1simple_import.done.
 * generateSDTemplateIfAbsent() — writes /v1simple_config_template.json with
 *                             every editable field, defaults, and comments so
 *                             users can edit it on a PC and drop it on the SD.
 */

#include "../include/settings_internals.h"
#include <Arduino.h>

static constexpr const char* SD_IMPORT_PATH      = "/v1simple_import.json";
static constexpr const char* SD_IMPORT_DONE_PATH = "/v1simple_import.done";
static constexpr const char* SD_TEMPLATE_PATH    = "/v1simple_config_template.json";

// ---------------------------------------------------------------------------
// importFromSD
// ---------------------------------------------------------------------------

bool SettingsManager::importFromSD() {
    if (!storageManager.isReady() || !storageManager.isSDCard()) return false;

    StorageManager::SDLockBlocking sdLock(storageManager.getSDMutex());
    if (!sdLock) return false;

    fs::FS* fs = storageManager.getFilesystem();
    if (!fs || !fs->exists(SD_IMPORT_PATH)) return false;

    Serial.println("[Settings] SD import file found — applying...");

    JsonDocument doc;
    if (!parseBackupFile(fs, SD_IMPORT_PATH, doc, true)) {
        Serial.println("[Settings] SD import: JSON parse failed — file left in place");
        return false;
    }

    // Accept the file even without the _type signature so hand-crafted files work.
    // A minimal file only needs the fields the user wants to change.
    const SettingsBackupApplyResult result = applyBackupDocument(doc, false);
    const bool ok = result.success;

    if (ok) {
        // Rename so it doesn't apply again on next boot.
        if (fs->exists(SD_IMPORT_DONE_PATH)) fs->remove(SD_IMPORT_DONE_PATH);
        fs->rename(SD_IMPORT_PATH, SD_IMPORT_DONE_PATH);
        Serial.println("[Settings] SD import applied — renamed to v1simple_import.done");
    } else {
        Serial.println("[Settings] SD import: applyBackupDocument failed — file left in place");
    }

    return ok;
}

// ---------------------------------------------------------------------------
// generateSDTemplateIfAbsent
// ---------------------------------------------------------------------------

void SettingsManager::generateSDTemplateIfAbsent() {
    if (!storageManager.isReady() || !storageManager.isSDCard()) return;

    StorageManager::SDLockBlocking sdLock(storageManager.getSDMutex());
    if (!sdLock) return;

    fs::FS* fs = storageManager.getFilesystem();
    if (!fs || fs->exists(SD_TEMPLATE_PATH)) return;

    Serial.println("[Settings] Writing SD config template...");

    File f = fs->open(SD_TEMPLATE_PATH, FILE_WRITE);
    if (!f) {
        Serial.println("[Settings] Failed to open template file for writing");
        return;
    }

    // Write human-readable JSON with inline comments (JSON5-style — strip comments
    // before parsing if needed, or keep as plain JSON without comments).
    // We use plain JSON so any standard parser can read it.
    const V1Settings& s = settings_;

    JsonDocument doc;

    // Metadata — helps the user understand the file
    doc["_type"]    = "v1simple_backup";
    doc["_version"] = SD_BACKUP_VERSION;
    doc["_note"]    = "Edit any fields below, save as v1simple_import.json on the SD card root, insert SD and power on. Applied once then renamed to v1simple_import.done. Omit fields you don't want to change.";

    // --- WiFi ---
    doc["enableWifi"]        = s.enableWifi;
    doc["enableWifiAtBoot"]  = s.enableWifiAtBoot;   // true = AP starts on every boot
    doc["apSSID"]            = s.apSSID;             // AP network name (max 31 chars)
    doc["apTimeoutMinutes"]  = s.apTimeoutMinutes;   // 0 = never timeout

    // --- BLE Proxy ---
    doc["proxyBLE"]  = s.proxyBLE;    // true = re-advertise V1 data for companion app
    doc["proxyName"] = s.proxyName;   // BLE device name shown to companion app (max 32 chars)

    // --- Display ---
    doc["brightness"]      = s.brightness;      // 0-255
    doc["turnOffDisplay"]  = s.turnOffDisplay;  // true = screen off when idle
    doc["displayStyle"]    = static_cast<int>(s.displayStyle);  // 0=Classic 1=OFR 2=V1G2Simple

    // --- Colours (RGB565 hex strings, e.g. "0xF800" = red) ---
    char buf[12];
    auto hex = [&](uint32_t v) -> const char* {
        snprintf(buf, sizeof(buf), "0x%04X", (unsigned)v);
        return buf;  // NOTE: single buf reused — doc.add() copies the string
    };
    doc["colorBandKa"]        = hex(s.colorBandKa);
    doc["colorBandK"]         = hex(s.colorBandK);
    doc["colorBandX"]         = hex(s.colorBandX);
    doc["colorBandL"]         = hex(s.colorBandL);       // Laser
    doc["colorBandPhoto"]     = hex(s.colorBandPhoto);
    doc["colorBogey"]         = hex(s.colorBogey);
    doc["colorFrequency"]     = hex(s.colorFrequency);
    doc["colorArrowFront"]    = hex(s.colorArrowFront);
    doc["colorArrowSide"]     = hex(s.colorArrowSide);
    doc["colorArrowRear"]     = hex(s.colorArrowRear);
    doc["colorMuted"]         = hex(s.colorMuted);
    doc["colorPersisted"]     = hex(s.colorPersisted);
    doc["freqUseBandColor"]   = s.freqUseBandColor;

    // --- Signal bar colours ---
    doc["colorBar1"] = hex(s.colorBar1);
    doc["colorBar2"] = hex(s.colorBar2);
    doc["colorBar3"] = hex(s.colorBar3);
    doc["colorBar4"] = hex(s.colorBar4);
    doc["colorBar5"] = hex(s.colorBar5);
    doc["colorBar6"] = hex(s.colorBar6);

    // --- Status bar icon colours ---
    doc["colorWiFiIcon"]        = hex(s.colorWiFiIcon);
    doc["colorWiFiConnected"]   = hex(s.colorWiFiConnected);
    doc["colorBleConnected"]    = hex(s.colorBleConnected);
    doc["colorBleDisconnected"] = hex(s.colorBleDisconnected);
    doc["colorObd"]             = hex(s.colorObd);
    doc["colorVolumeMain"]      = hex(s.colorVolumeMain);
    doc["colorVolumeMute"]      = hex(s.colorVolumeMute);
    doc["colorRssiV1"]          = hex(s.colorRssiV1);
    doc["colorRssiProxy"]       = hex(s.colorRssiProxy);

    // --- Status bar visibility ---
    doc["hideWifiIcon"]          = s.hideWifiIcon;
    doc["hideBleIcon"]           = s.hideBleIcon;
    doc["hideBatteryIcon"]       = s.hideBatteryIcon;
    doc["showBatteryPercent"]    = s.showBatteryPercent;
    doc["hideProfileIndicator"]  = s.hideProfileIndicator;
    doc["hideVolumeIndicator"]   = s.hideVolumeIndicator;
    doc["hideRssiIndicator"]     = s.hideRssiIndicator;

    // --- Voice alerts ---
    doc["voiceAlertMode"]         = static_cast<int>(s.voiceAlertMode);  // 0=off 1=band 2=freq 3=band+freq
    doc["voiceDirectionEnabled"]  = s.voiceDirectionEnabled;
    doc["announceBogeyCount"]     = s.announceBogeyCount;
    doc["announceSecondaryAlerts"]= s.announceSecondaryAlerts;
    doc["muteVoiceIfVolZero"]     = s.muteVoiceIfVolZero;
    doc["voiceVolume"]            = s.voiceVolume;  // 0-100

    // --- Alert filters ---
    doc["secondaryLaser"] = s.secondaryLaser;
    doc["secondaryKa"]    = s.secondaryKa;
    doc["secondaryK"]     = s.secondaryK;
    doc["secondaryX"]     = s.secondaryX;

    // --- Volume fade ---
    doc["alertVolumeFadeEnabled"]  = s.alertVolumeFadeEnabled;
    doc["alertVolumeFadeDelaySec"] = s.alertVolumeFadeDelaySec;

    // --- Driving modes / speed mute ---
    doc["activeDrivingMode"]   = static_cast<int>(s.activeDrivingMode);  // 0=Normal 1=Quiet 2=Highway 3=Night
    doc["autoPowerOffMinutes"] = s.autoPowerOffMinutes;  // 0 = never

    // --- Chimes ---
    doc["startupChimeEnabled"]  = s.startupSoundEnabled;
    doc["shutdownChimeEnabled"] = s.shutdownSoundEnabled;

    // --- Auto-push ---
    doc["autoPushEnabled"] = s.autoPushEnabled;
    doc["activeSlot"]      = s.activeSlot;  // active profile slot (0-2)

    serializeJsonPretty(doc, f);
    f.close();

    Serial.printf("[Settings] Template written to %s\n", SD_TEMPLATE_PATH);
}
