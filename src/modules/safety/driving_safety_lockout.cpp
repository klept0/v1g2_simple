#include "driving_safety_lockout.h"
#include "modules/speed/speed_source_selector.h"
#include "settings.h"

void DrivingSafetyLockout::begin(SpeedSourceSelector* speed, SettingsManager* settings) {
    speed_    = speed;
    settings_ = settings;
}

bool DrivingSafetyLockout::isLocked() const {
    if (!speed_ || !settings_) return false;
    if (!settings_->isLockoutEnabled()) return false;

    const SpeedSelection sel = speed_->selectedSpeed();
    if (!sel.valid) return false;  // speed unknown → not locked (safe fallback)

    return sel.speedMph >= static_cast<float>(settings_->getLockoutThresholdMph());
}

float DrivingSafetyLockout::currentSpeedMph() const {
    if (!speed_) return 0.0f;
    return speed_->selectedSpeed().speedMph;
}

bool DrivingSafetyLockout::isSpeedValid() const {
    if (!speed_) return false;
    return speed_->selectedSpeed().valid;
}
