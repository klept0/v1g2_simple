/**
 * Stealth Night Mode renderer for V1-Simple
 * Minimal fighter-jet HUD: speed left, alert centre, direction right.
 * Pure red/orange monochrome on black — nothing else on screen.
 *
 * Layout (640×172 landscape):
 *
 *  Left  (X=0..213)    SPEED — large 7-segment number in radar-red
 *  Centre (X=213..450) ALERT — band + frequency in orange (or "CLEAR" dim)
 *  Right  (X=450..640) DIRECTION / MUTED in red
 */

#include "display.h"
#include "modules/display/dashboard_module.h"
#include "../include/display_layout.h"
#include "../include/display_draw.h"
#include "../include/display_text.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Stealth palette — red/orange on black only
// ============================================================================

static constexpr uint16_t STH_BG      = 0x0000;
static constexpr uint16_t STH_RED     = 0xF800;
static constexpr uint16_t STH_ORANGE  = 0xFD20;
static constexpr uint16_t STH_DIM_RED = 0x6000;  // very dim red for labels
static constexpr uint16_t STH_AMBER   = 0xFC00;  // amber for direction

// ============================================================================
// Layout
// ============================================================================

static constexpr int16_t W = SCREEN_WIDTH;   // 640
static constexpr int16_t H = SCREEN_HEIGHT;  // 172

static constexpr int16_t SPEED_COL_W  = 213;
static constexpr int16_t ALERT_COL_X  = 213;
static constexpr int16_t ALERT_COL_W  = 237;
static constexpr int16_t DIR_COL_X    = 450;
static constexpr int16_t DIR_COL_W    = W - DIR_COL_X;  // 190

// ============================================================================
// Helpers
// ============================================================================

static const char* sthBandName(Band b) {
    switch (b) {
        case BAND_LASER: return "LASER";
        case BAND_KA:    return "Ka";
        case BAND_K:     return "K";
        case BAND_X:     return "X";
        default:         return "";
    }
}

static const char* sthDirArrow(Direction d) {
    switch (d) {
        case DIR_FRONT: return "AHEAD";
        case DIR_REAR:  return "BEHIND";
        case DIR_SIDE:  return "SIDE";
        default:        return "";
    }
}

// ============================================================================
// V1Display::showStealth
// ============================================================================

void V1Display::showStealth(const StealthData& data) {
    FILL_SCREEN(STH_BG);

    // Thin dim separators
    const int16_t sepY0 = 8;
    const int16_t sepY1 = H - 8;
    DRAW_LINE(ALERT_COL_X, sepY0, ALERT_COL_X, sepY1, STH_DIM_RED);
    DRAW_LINE(DIR_COL_X,   sepY0, DIR_COL_X,   sepY1, STH_DIM_RED);

    // -------------------------------------------------------------------------
    // SPEED — left column
    // -------------------------------------------------------------------------

    {
        const int16_t cx = SPEED_COL_W / 2;  // 106

        if (data.speedValid) {
            const int spd = static_cast<int>(data.speedMph + 0.5f);
            char spdBuf[8];
            snprintf(spdBuf, sizeof(spdBuf), "%d", spd);

            const float scale = 2.0f;
            const int segW = measureSevenSegmentText(spdBuf, scale);
            const int segX = cx - segW / 2;
            drawSevenSegmentText(spdBuf, segX, 20, scale,
                                 STH_RED, STH_DIM_RED);

            tft_->setTextSize(1);
            tft_->setTextColor(STH_DIM_RED, STH_BG);
            GFX_setTextDatum(BC_DATUM);
            GFX_drawString(tft_.get(), "MPH", cx, H - 6);
        } else {
            // No OBD speed — show dim dashes
            const float scale = 2.0f;
            const int segW = measureSevenSegmentText("--", scale);
            const int segX = cx - segW / 2;
            drawSevenSegmentText("--", segX, 20, scale,
                                 STH_DIM_RED, STH_DIM_RED);
        }
    }

    // -------------------------------------------------------------------------
    // ALERT — centre column
    // -------------------------------------------------------------------------

    {
        const int16_t cx = ALERT_COL_X + ALERT_COL_W / 2;

        if (data.hasAlert) {
            // Band name (large)
            tft_->setTextSize(3);
            tft_->setTextColor(STH_ORANGE, STH_BG);
            GFX_setTextDatum(TC_DATUM);
            GFX_drawString(tft_.get(), sthBandName(data.band), cx, 14);

            // Frequency
            if (data.band != BAND_LASER && data.freqMHz > 0) {
                char freqBuf[14];
                const uint32_t intP  = data.freqMHz / 1000;
                const uint32_t fracP = (data.freqMHz % 1000) / 10;
                snprintf(freqBuf, sizeof(freqBuf), "%lu.%02lu", intP, fracP);

                tft_->setTextSize(2);
                tft_->setTextColor(STH_ORANGE, STH_BG);
                GFX_setTextDatum(TC_DATUM);
                GFX_drawString(tft_.get(), freqBuf, cx, 74);

                tft_->setTextSize(1);
                tft_->setTextColor(STH_DIM_RED, STH_BG);
                GFX_setTextDatum(TC_DATUM);
                GFX_drawString(tft_.get(), "GHz", cx, 110);
            }
        } else {
            // Clear — minimal dim indicator
            tft_->setTextSize(1);
            tft_->setTextColor(STH_DIM_RED, STH_BG);
            GFX_setTextDatum(MC_DATUM);
            GFX_drawString(tft_.get(), "CLEAR", cx, H / 2);
        }
    }

    // -------------------------------------------------------------------------
    // DIRECTION / MUTED — right column
    // -------------------------------------------------------------------------

    {
        const int16_t cx = DIR_COL_X + DIR_COL_W / 2;

        if (data.muted) {
            tft_->setTextSize(2);
            tft_->setTextColor(STH_DIM_RED, STH_BG);
            GFX_setTextDatum(MC_DATUM);
            GFX_drawString(tft_.get(), "MUTED", cx, H / 2);
        } else if (data.hasAlert && data.direction != DIR_NONE) {
            tft_->setTextSize(2);
            tft_->setTextColor(STH_AMBER, STH_BG);
            GFX_setTextDatum(MC_DATUM);
            GFX_drawString(tft_.get(), sthDirArrow(data.direction), cx, H / 2);
        }
        // else: nothing — keep the right side dark when no alert
    }

    flush();
}
