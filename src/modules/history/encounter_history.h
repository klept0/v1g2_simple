#pragma once

#include <Arduino.h>
#include <vector>
#include "packet_parser_types.h"

struct EncounterEntry {
    int64_t  timestampMs  = 0;       // Unix epoch ms (0 = unknown)
    uint32_t monoMs       = 0;       // millis() at alert onset (always valid)
    Band     band         = BAND_NONE;
    uint32_t frequencyMhz = 0;
    Direction direction   = DIR_NONE;
    uint8_t  bogeyCount   = 0;
    uint8_t  signalBars   = 0;       // 0-6
    float    speedMph     = 0.0f;
    char     slotName[13] = "DEFAULT";
    char     modeChar     = 0;
    bool     muted        = false;
    bool     falseAlert   = false;
    uint32_t id           = 0;       // Auto-incrementing, used for mark-false
};

// Minimal filesystem abstraction to keep this testable without LittleFS.
struct HistoryFS {
    virtual bool   exists(const char* path)                                       = 0;
    virtual bool   mkdir(const char* path)                                        = 0;
    virtual bool   remove(const char* path)                                       = 0;
    virtual size_t fileSize(const char* path)                                     = 0;
    virtual bool   readLines(const char* path,
                             void (*cb)(const char* line, void* ctx),
                             void* ctx)                                           = 0;
    virtual bool   appendLine(const char* path, const char* line)                 = 0;
    virtual bool   writeFile(const char* path, const char* data, size_t len)      = 0;
    virtual ~HistoryFS() = default;
};

class EncounterHistory {
public:
    static constexpr uint16_t DEFAULT_MAX_ENTRIES = 50;
    static constexpr uint16_t ABSOLUTE_MAX_ENTRIES = 250;

    static constexpr const char* HISTORY_DIR  = "/history";
    static constexpr const char* DATA_FILE    = "/history/encounters.ndjson";
    static constexpr const char* META_FILE    = "/history/meta.json";

    void begin(HistoryFS* fs, uint16_t maxEntries = DEFAULT_MAX_ENTRIES);

    // Record a new encounter. Trims oldest entry when over limit.
    void record(const EncounterEntry& entry);

    // Query — fills up to `maxCount` most-recent entries into out[], returns count filled.
    size_t getRecent(EncounterEntry* out, size_t maxCount) const;

    size_t count() const { return entries_.size(); }

    // Mark entry with matching id as a false alert; persists immediately.
    bool markFalseAlert(uint32_t id, bool falseAlert = true);

    // Clear all entries from memory and disk.
    void clearAll();

    // Write CSV to a String buffer (callers can stream it).
    void toCsvString(String& out) const;

    void setMaxEntries(uint16_t max);
    uint16_t getMaxEntries() const { return maxEntries_; }

private:
    HistoryFS* fs_          = nullptr;
    uint16_t   maxEntries_  = DEFAULT_MAX_ENTRIES;
    uint32_t   nextId_      = 1;

    // In-memory ring kept in insertion order (index 0 = oldest).
    // Loaded from disk at begin(); trimmed on record().
    // We cap in-memory load to ABSOLUTE_MAX_ENTRIES.
    std::vector<EncounterEntry> entries_;

    void loadFromDisk();
    void flushToDisk() const;
    void saveMetadata() const;
    uint32_t loadNextId();
    static bool parseEntry(const char* json, EncounterEntry& out);
    static void serializeEntry(const EncounterEntry& e, String& out);
};
