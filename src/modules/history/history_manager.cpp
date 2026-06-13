#include "history_manager.h"
#include <string.h>

HistoryManager historyManager;

void HistoryManager::addEncounter(const char* band, const char* freq,
                                  uint32_t startMs, uint32_t endMs, uint8_t maxStrength) {
    Encounter& e = buf_[head_];
    strncpy(e.band, band ? band : "", sizeof(e.band) - 1);
    e.band[sizeof(e.band) - 1] = '\0';
    strncpy(e.frequency, freq ? freq : "", sizeof(e.frequency) - 1);
    e.frequency[sizeof(e.frequency) - 1] = '\0';
    e.startMs     = startMs;
    e.endMs       = endMs;
    e.maxStrength = maxStrength;

    head_ = (head_ + 1) % MAX_HISTORY;
    if (count_ < MAX_HISTORY) {
        count_++;
    }
}

void HistoryManager::clear() {
    head_  = 0;
    count_ = 0;
}

const Encounter& HistoryManager::get(int i) const {
    // i=0 is most recent; head_ points to next write slot, so most recent is head_-1
    int idx = (head_ - 1 - i + MAX_HISTORY * 2) % MAX_HISTORY;
    return buf_[idx];
}
