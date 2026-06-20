/**
 * Settings slider methods — extracted from display.cpp (Phase 3B)
 *
 * Contains showSettingsSliders, updateSettingsSliders,
 * getActiveSliderFromTouch, hideBrightnessSlider.
 */

#include "display.h"
#include "../include/display_layout.h"
#include "../include/display_palette.h"
#include "../include/display_flush.h"
#include "../include/display_slider_math.h"

// Slider-specific color constants (not theme-dependent — fixed hardware UI)
static constexpr uint16_t SLIDER_WHITE         = 0xFFFF;  // Labels and thumb
static constexpr uint16_t SLIDER_TRACK_BORDER  = 0x4208;  // Outer border of track
static constexpr uint16_t SLIDER_TRACK_FILL    = 0x2104;  // Empty track fill
static constexpr uint16_t SLIDER_BRIGHTNESS_FG = 0x07E0;  // Green — brightness fill
static constexpr uint16_t SLIDER_VOLUME_FG     = 0x001F;  // Blue  — volume fill
static constexpr uint16_t SLIDER_HINT_TEXT     = 0x8410;  // Medium gray — hint text

// Combined settings screen with brightness and voice volume sliders
void V1Display::showSettingsSliders(uint8_t brightnessLevel, uint8_t volumeLevel) {
    // Clear screen to dark background
    tft_->fillScreen(PALETTE_BG);

    // Layout: 640x172 landscape - two horizontal sliders stacked
    const int sliderMargin = 40;
    const int sliderHeight = 10;
    const int sliderWidth = SCREEN_WIDTH - (sliderMargin * 2);  // 560 pixels
    const int sliderX = sliderMargin;

    // Brightness slider at top (y=45)
    const int brightnessY = 45;
    // Volume slider lower (y=115)
    const int volumeY = 115;

    // Title
    tft_->setTextColor(SLIDER_WHITE);
    tft_->setTextSize(2);
    tft_->setCursor((SCREEN_WIDTH - 120) / 2, 5);
    tft_->print("SETTINGS");

    // ============================================================================
    // Brightness slider
    // ============================================================================
    tft_->setTextSize(1);
    tft_->setTextColor(SLIDER_WHITE);
    tft_->setCursor(sliderMargin, brightnessY - 16);
    tft_->print("BRIGHTNESS");

    // Draw slider track
    tft_->drawRect(sliderX - 2, brightnessY - 2, sliderWidth + 4, sliderHeight + 4, SLIDER_TRACK_BORDER);
    tft_->fillRect(sliderX, brightnessY, sliderWidth, sliderHeight, SLIDER_TRACK_FILL);

    // Fill based on brightness level (80-255 range)
    int brightnessFill = computeBrightnessSliderFill(brightnessLevel, sliderWidth);
    tft_->fillRect(sliderX, brightnessY, brightnessFill, sliderHeight, SLIDER_BRIGHTNESS_FG);  // Green

    // Thumb
    int brightThumbX = sliderX + brightnessFill - 4;
    if (brightThumbX < sliderX) brightThumbX = sliderX;
    if (brightThumbX > sliderX + sliderWidth - 8) brightThumbX = sliderX + sliderWidth - 8;
    tft_->fillRect(brightThumbX, brightnessY - 4, 8, sliderHeight + 8, SLIDER_WHITE);

    // Percentage text
    char brightStr[8];
    int brightPercent = computeBrightnessSliderPercent(brightnessLevel);
    snprintf(brightStr, sizeof(brightStr), "%d%%", brightPercent);
    tft_->setCursor(sliderX + sliderWidth + 8, brightnessY);
    tft_->print(brightStr);

    // ============================================================================
    // Voice volume slider
    // ============================================================================
    tft_->setTextColor(SLIDER_WHITE);
    tft_->setCursor(sliderMargin, volumeY - 16);
    tft_->print("VOICE VOLUME");

    // Draw slider track
    tft_->drawRect(sliderX - 2, volumeY - 2, sliderWidth + 4, sliderHeight + 4, SLIDER_TRACK_BORDER);
    tft_->fillRect(sliderX, volumeY, sliderWidth, sliderHeight, SLIDER_TRACK_FILL);

    // Fill based on volume level (0-100 range)
    int volumeFill = (volumeLevel * sliderWidth) / 100;
    tft_->fillRect(sliderX, volumeY, volumeFill, sliderHeight, SLIDER_VOLUME_FG);  // Blue for volume

    // Thumb
    int volThumbX = sliderX + volumeFill - 4;
    if (volThumbX < sliderX) volThumbX = sliderX;
    if (volThumbX > sliderX + sliderWidth - 8) volThumbX = sliderX + sliderWidth - 8;
    tft_->fillRect(volThumbX, volumeY - 4, 8, sliderHeight + 8, SLIDER_WHITE);

    // Percentage text
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d%%", volumeLevel);
    tft_->setCursor(sliderX + sliderWidth + 8, volumeY);
    tft_->print(volStr);

    // Instructions at bottom
    tft_->setTextSize(1);
    tft_->setTextColor(SLIDER_HINT_TEXT);  // Medium gray
    tft_->setCursor((SCREEN_WIDTH - 220) / 2, 155);
    tft_->print("Touch sliders - BOOT to save");

    DISPLAY_FLUSH();
}

void V1Display::updateSettingsSliders(uint8_t brightnessLevel, uint8_t volumeLevel, int activeSlider) {
    // Apply brightness in real-time for visual feedback
    setBrightness(brightnessLevel);
    showSettingsSliders(brightnessLevel, volumeLevel);
}

// Returns which slider was touched: 0=brightness, 1=volume, -1=none
// Touch Y is inverted relative to display Y:
//   Low touch Y = bottom of display = volume slider
//   High touch Y = top of display = brightness slider
int V1Display::getActiveSliderFromTouch(int16_t touchY) {
    if (touchY <= 60) return 1;   // Volume (bottom of display)
    if (touchY >= 80) return 0;   // Brightness (top of display)
    return -1;  // Dead zone between sliders
}

void V1Display::hideBrightnessSlider() {
    clear();
}

// ─── Page 2: toggle buttons ──────────────────────────────────────────────────
// Three equal-width buttons across 640px, vertically centred in 172px.
// Button layout:  [  WiFi AP  ] [ BLE Proxy ] [  Mute→0  ]
//                  0..212        213..426       427..640

void V1Display::showTogglesPage(bool wifiOn, bool proxyOn, bool muteZeroOn) {
    clear();

    const int btnW   = SCREEN_WIDTH / 3;   // 213px each
    const int btnH   = 100;
    const int btnY   = (SCREEN_HEIGHT - btnH) / 2;  // vertically centred
    const int radius = 8;

    // Colours
    const uint16_t ON_BG     = 0x07E0;  // green
    const uint16_t OFF_BG    = 0x2104;  // dark grey
    const uint16_t LABEL_COL = 0xFFFF;  // white
    const uint16_t BORDER    = 0x4208;  // mid-grey border

    struct Btn { int x; const char* label; bool state; };
    Btn buttons[3] = {
        { 0,         "WiFi AP",   wifiOn      },
        { btnW,      "BLE Proxy", proxyOn     },
        { btnW * 2,  "Mute=0",   muteZeroOn  },
    };

    tft_->setTextSize(1);
    for (auto& b : buttons) {
        uint16_t bg = b.state ? ON_BG : OFF_BG;
        // Fill + border
        tft_->fillRoundRect(b.x + 6, btnY, btnW - 12, btnH, radius, bg);
        tft_->drawRoundRect(b.x + 6, btnY, btnW - 12, btnH, radius, BORDER);

        // Label (centred, white)
        tft_->setTextColor(LABEL_COL, bg);
        int16_t tx = b.x + (btnW / 2) - (strlen(b.label) * 6 / 2);
        tft_->setCursor(tx, btnY + 28);
        tft_->print(b.label);

        // State chip
        const char* stateStr = b.state ? " ON " : " OFF";
        uint16_t stateCol = b.state ? 0xFFFF : 0xAD75;
        tft_->setTextColor(stateCol, bg);
        int16_t sx = b.x + (btnW / 2) - (4 * 6 / 2);
        tft_->setCursor(sx, btnY + 52);
        tft_->print(stateStr);
    }

    // Footer hint
    tft_->setTextColor(0x8410, PALETTE_BG);  // dim grey
    tft_->setCursor(4, SCREEN_HEIGHT - 14);
    tft_->print("BOOT = exit & save");

    flush();
}
