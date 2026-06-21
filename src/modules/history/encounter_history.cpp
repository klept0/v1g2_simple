#include "encounter_history.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <cstring>
#include "band_utils.h"

#ifndef UNIT_TEST

// ──────────────────────────────────────────────────────────────────────────────
// LittleFS-backed HistoryFS implementation (only compiled for firmware)
// ──────────────────────────────────────────────────────────────────────────────

#include <LittleFS.h>

class LittleFSHistoryAdapter : public HistoryFS {
public:
    bool exists(const char* path) override { return LittleFS.exists(path); }

    bool mkdir(const char* path) override {
        if (LittleFS.exists(path)) return true;
        return LittleFS.mkdir(path);
    }

    bool remove(const char* path) override { return LittleFS.remove(path); }

    size_t fileSize(const char* path) override {
        File f = LittleFS.open(path, FILE_READ);
        if (!f) return 0;
        size_t sz = f.size();
        f.close();
        return sz;
    }

    bool readLines(const char* path,
                   void (*cb)(const char* line, void* ctx),
                   void* ctx) override {
        File f = LittleFS.open(path, FILE_READ);
        if (!f) return false;
        String line;
        while (f.available()) {
            char c = f.read();
            if (c == '\n') {
                if (line.length() > 0) {
                    cb(line.c_str(), ctx);
                    line = "";
                }
            } else {
                line += c;
            }
        }
        if (line.length() > 0) cb(line.c_str(), ctx);
        f.close();
        return true;
    }

    bool appendLine(const char* path, const char* line) override {
        File f = LittleFS.open(path, FILE_APPEND);
        if (!f) return false;
        f.println(line);
        f.close();
        return true;
    }

    bool writeFile(const char* path, const char* data, size_t len) override {
        File f = LittleFS.open(path, FILE_WRITE);
        if (!f) return false;
        f.write(reinterpret_cast<const uint8_t*>(data), len);
        f.close();
        return true;
    }
};

static LittleFSHistoryAdapter s_littleFSAdapter;

#endif  // UNIT_TEST

// ──────────────────────────────────────────────────────────────────────────────
// Portable serialization helpers (compiled in all configurations)
// ──────────────────────────────────────────────────────────────────────────────

static const char* directionStr(Direction d) {
    if (d & DIR_FRONT) return "Front";
    if (d & DIR_REAR)  return "Rear";
    if (d & DIR_SIDE)  return "Side";
    return "Unknown";
}

void EncounterHistory::serializeEntry(const EncounterEntry& e, String& out) {
    JsonDocument doc;
    doc["id"]           = e.id;
    doc["ts"]           = e.timestampMs;
    doc["mono"]         = e.monoMs;
    doc["band"]         = bandName(e.band);
    doc["freq"]         = e.frequencyMhz;
    doc["dir"]          = directionStr(e.direction);
    doc["bogeys"]       = e.bogeyCount;
    doc["bars"]         = e.signalBars;
    doc["speed"]        = (int)(e.speedMph + 0.5f);
    doc["slot"]         = e.slotName;
    doc["mode"]         = (e.modeChar ? String(e.modeChar) : String("?"));
    doc["muted"]        = e.muted;
    doc["false"]        = e.falseAlert;
    serializeJson(doc, out);
}

bool EncounterHistory::parseEntry(const char* json, EncounterEntry& out) {
    JsonDocument doc;
    auto err = deserializeJson(doc, json);
    if (err) return false;

    out.id            = doc["id"]     | 0u;
    out.timestampMs   = doc["ts"]     | (int64_t)0;
    out.monoMs        = doc["mono"]   | 0u;
    out.frequencyMhz  = doc["freq"]   | 0u;
    out.bogeyCount    = doc["bogeys"] | 0u;
    out.signalBars    = doc["bars"]   | 0u;
    out.speedMph      = (float)(doc["speed"] | 0);
    out.muted         = doc["muted"]  | false;
    out.falseAlert    = doc["false"]  | false;

    const char* modeStr = doc["mode"] | "?";
    out.modeChar = (modeStr && modeStr[0]) ? modeStr[0] : 0;

    const char* slot = doc["slot"] | "DEFAULT";
    strlcpy(out.slotName, slot, sizeof(out.slotName));

    const char* bandStr = doc["band"] | "None";
    if      (strcmp(bandStr, "Ka")    == 0) out.band = BAND_KA;
    else if (strcmp(bandStr, "K")     == 0) out.band = BAND_K;
    else if (strcmp(bandStr, "X")     == 0) out.band = BAND_X;
    else if (strcmp(bandStr, "Laser") == 0) out.band = BAND_LASER;
    else                                    out.band = BAND_NONE;

    const char* dirStr = doc["dir"] | "Unknown";
    if      (strcmp(dirStr, "Front") == 0) out.direction = DIR_FRONT;
    else if (strcmp(dirStr, "Rear")  == 0) out.direction = DIR_REAR;
    else if (strcmp(dirStr, "Side")  == 0) out.direction = DIR_SIDE;
    else                                   out.direction = DIR_NONE;

    return out.id > 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// EncounterHistory public API
// ──────────────────────────────────────────────────────────────────────────────

void EncounterHistory::begin(HistoryFS* fs, uint16_t maxEntries) {
#ifndef UNIT_TEST
    if (!fs) fs = &s_littleFSAdapter;
#endif
    fs_         = fs;
    maxEntries_ = std::min(maxEntries, ABSOLUTE_MAX_ENTRIES);
    if (maxEntries_ == 0) maxEntries_ = DEFAULT_MAX_ENTRIES;
    loadFromDisk();
}

void EncounterHistory::record(const EncounterEntry& entry) {
    if (!fs_) return;

    EncounterEntry e = entry;
    e.id = nextId_++;

    entries_.push_back(e);

    // Trim oldest entries when over limit
    while (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin());
    }

    // Append to disk; rebuild file if we just trimmed (i.e. vector shrank)
    String line;
    serializeEntry(e, line);
    bool appended = fs_->appendLine(DATA_FILE, line.c_str());

    // If append failed or we trimmed, do a full rewrite
    if (!appended || entries_.size() == maxEntries_) {
        // Check if the disk file has more lines than entries_ (means we trimmed)
        // We just always flush when at capacity to keep disk in sync
        if (entries_.size() >= maxEntries_) {
            flushToDisk();
        }
    }

    saveMetadata();
}

size_t EncounterHistory::getRecent(EncounterEntry* out, size_t maxCount) const {
    if (!out || maxCount == 0 || entries_.empty()) return 0;
    size_t total = entries_.size();
    size_t start = (total > maxCount) ? (total - maxCount) : 0;
    size_t n     = total - start;
    // Copy in reverse order (most recent first)
    for (size_t i = 0; i < n; i++) {
        out[i] = entries_[total - 1 - i];
    }
    return n;
}

bool EncounterHistory::markFalseAlert(uint32_t id, bool falseAlert) {
    bool found = false;
    for (auto& e : entries_) {
        if (e.id == id) {
            e.falseAlert = falseAlert;
            found = true;
            break;
        }
    }
    if (found) flushToDisk();
    return found;
}

void EncounterHistory::clearAll() {
    entries_.clear();
    nextId_ = 1;
    if (fs_) {
        fs_->remove(DATA_FILE);
        fs_->remove(META_FILE);
    }
}

void EncounterHistory::toCsvString(String& out) const {
    out = "id,timestamp_ms,band,freq_mhz,direction,bogeys,signal_bars,speed_mph,slot,mode,muted,false_alert\n";
    for (const auto& e : entries_) {
        out += String(e.id); out += ',';
        out += String((int64_t)e.timestampMs); out += ',';
        out += bandName(e.band); out += ',';
        out += String(e.frequencyMhz); out += ',';
        out += directionStr(e.direction); out += ',';
        out += String(e.bogeyCount); out += ',';
        out += String(e.signalBars); out += ',';
        out += String((int)(e.speedMph + 0.5f)); out += ',';
        out += e.slotName; out += ',';
        out += (e.modeChar ? String(e.modeChar) : String("?")); out += ',';
        out += (e.muted ? "1" : "0"); out += ',';
        out += (e.falseAlert ? "1" : "0"); out += '\n';
    }
}

void EncounterHistory::setMaxEntries(uint16_t max) {
    maxEntries_ = std::min(max, ABSOLUTE_MAX_ENTRIES);
    if (maxEntries_ == 0) maxEntries_ = DEFAULT_MAX_ENTRIES;
    while (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin());
    }
    flushToDisk();
    saveMetadata();
}

// ──────────────────────────────────────────────────────────────────────────────
// Private helpers
// ──────────────────────────────────────────────────────────────────────────────

void EncounterHistory::loadFromDisk() {
    if (!fs_) return;

    fs_->mkdir(HISTORY_DIR);

    // Load metadata for nextId
    nextId_ = loadNextId();
    entries_.clear();

    if (!fs_->exists(DATA_FILE)) return;

    struct LoadCtx {
        EncounterHistory* self;
    } ctx{this};

    fs_->readLines(DATA_FILE, [](const char* line, void* raw) {
        auto* c = static_cast<LoadCtx*>(raw);
        EncounterEntry e;
        if (EncounterHistory::parseEntry(line, e)) {
            c->self->entries_.push_back(e);
            if (e.id >= c->self->nextId_) {
                c->self->nextId_ = e.id + 1;
            }
        }
    }, &ctx);

    // Enforce max on load
    while (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin());
    }
}

void EncounterHistory::flushToDisk() const {
    if (!fs_) return;
    fs_->mkdir(HISTORY_DIR);
    String buf;
    for (const auto& e : entries_) {
        String line;
        serializeEntry(e, line);
        buf += line;
        buf += '\n';
    }
    fs_->writeFile(DATA_FILE, buf.c_str(), buf.length());
}

void EncounterHistory::saveMetadata() const {
    if (!fs_) return;
    String meta = "{\"nextId\":" + String(nextId_) +
                  ",\"max\":"    + String(maxEntries_) + "}";
    fs_->writeFile(META_FILE, meta.c_str(), meta.length());
}

uint32_t EncounterHistory::loadNextId() {
    if (!fs_ || !fs_->exists(META_FILE)) return 1;
    uint32_t id = 1;
    struct Ctx { uint32_t* id; } ctx{&id};
    fs_->readLines(META_FILE, [](const char* line, void* raw) {
        auto* c = static_cast<Ctx*>(raw);
        JsonDocument doc;
        if (!deserializeJson(doc, line)) {
            *c->id = doc["nextId"] | 1u;
        }
    }, &ctx);
    return id ? id : 1;
}
