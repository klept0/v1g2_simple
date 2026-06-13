#pragma once
#include <Arduino.h>

struct Encounter {
    char     band[8];
    char     frequency[12];
    uint32_t startMs;
    uint32_t endMs;
    uint8_t  maxStrength;
};

class HistoryManager {
public:
    static constexpr int MAX_HISTORY = 10;

    void addEncounter(const char* band, const char* freq,
                      uint32_t startMs, uint32_t endMs, uint8_t maxStrength);
    void clear();
    int count() const { return count_; }
    const Encounter& get(int i) const; // 0 = most recent

private:
    Encounter buf_[MAX_HISTORY];
    int head_  = 0;
    int count_ = 0;
};

extern HistoryManager historyManager;
