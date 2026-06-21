#include "phone_companion_module.h"
#include <string.h>

void PhoneCompanionModule::update(const PhoneCompanionData& data) {
    data_    = data;
    hasData_ = true;
}

bool PhoneCompanionModule::isValid(uint32_t nowMs) const {
    if (!hasData_) return false;
    return (nowMs - data_.receivedMs) < PHONE_DATA_STALE_MS;
}

bool PhoneCompanionModule::getFreshSpeed(uint32_t nowMs, float& speedMphOut) const {
    if (!isValid(nowMs)) return false;
    if (!data_.hasSpeed) return false;
    speedMphOut = data_.speedMph;
    return true;
}

uint32_t PhoneCompanionModule::ageMs(uint32_t nowMs) const {
    if (!hasData_) return UINT32_MAX;
    return nowMs - data_.receivedMs;
}
