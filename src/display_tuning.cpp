/**
 * V1 Tuning Screen renderer for V1-Simple
 * Shows live alert diagnostics: band, frequency, signal bars, direction,
 * alert duration, active profile slot, and mute state. Useful for
 * validating sweep sensitivity and profile settings in the field.
 *
 * Layout (640×172 landscape):
 *
 *  Y=0..24    Header: "V1 TUNING" | slot name | mute/connected state
 *  Y=24..172  Four columns:
 *    X=0..160    BAND  (large band name, colour-coded)
 *    X=160..360  FREQ  (frequency in GHz)
 *    X=360..510  SIGNAL BARS (8-bar meter)
 *    X=510..640  DIRECTION + DURATION
 */

#include "display.h"
#include "modules/display/dashboard_module.h"
#include "../include/display_layout.h"
#include "../include/display_draw.h"
#include "../include/display_text.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Colours
// ============================================================================

static constexpr uint16_t TUN_BG      = 0x0000;
static constexpr uint16_t TUN_WHITE   = 0xFFFF;
static constexpr uint16_t TUN_GREY    = 0x8410;
static constexpr uint16_t TUN_DIM     = 0x39E7;
static constexpr uint16_t TUN_GREEN   = 0x07E0;
static constexpr uint16_t TUN_YELLOW  = 0xFFE0;
static constexpr uint16_t TUN_ORANGE  = 0xFD20;
static constexpr uint16_t TUN_RED     = 0xF800;
static constexpr uint16_t TUN_CYAN    = 0x07FF;

// ============================================================================
// Layout
// ============================================================================

static constexpr int16_t W = SCREEN_WIDTH;   // 640
static constexpr int16_t H = SCREEN_HEIGHT;  // 172

static constexpr int16_t HDR_H  = 24;
static constexpr int16_t BODY_Y = HDR_H;
static constexpr int16_t BODY_H = H - HDR_H;  // 148

static constexpr int16_t BAND_COL_X  = 0;
static constexpr int16_t BAND_COL_W  = 160;
static constexpr int16_t FREQ_COL_X  = 160;
static constexpr int16_t FREQ_COL_W  = 200;
static constexpr int16_t BARS_COL_X  = 360;
static constexpr int16_t BARS_COL_W  = 150;
static constexpr int16_t DIR_COL_X   = 510;
static constexpr int16_t DIR_COL_W   = W - DIR_COL_X;  // 130

// ============================================================================
// Helpers
// ============================================================================

static const char* tunBandName(Band b) {
    switch (b) {
        case BAND_LASER: return "LASER";
        case BAND_KA:    return "Ka";
        case BAND_K:     return "K";
        case BAND_X:     return "X";
        default:         return "---";
    }
}

static const char* tunDirName(Direction d) {
    switch (d) {
        case DIR_FRONT: return "AHEAD";
        case DIR_REAR:  return "BEHIND";
        case DIR_SIDE:  return "SIDE";
        default:        return "---";
    }
}

// Draw 6 vertical signal bars (V1 Gen2 uses 6-bar scale via frontStrength).
// Active bars transition green→yellow→red.
static void drawSignalBars(Arduino_Canvas* tft,
                            int16_t x0, int16_t y0,
                            int16_t totalW, int16_t totalH,
                            uint8_t bars) {
    static constexpr int kBars = 6;
    const int16_t gap  = 3;
    const int16_t barW = (totalW - gap * (kBars - 1)) / kBars;
    const int16_t maxH = totalH;

    for (int i = 0; i < kBars; i++) {
        const int16_t barH = maxH * (i + 1) / kBars;
        const int16_t bx   = x0 + i * (barW + gap);
        const int16_t by   = y0 + (maxH - barH);

        uint16_t col;
        if (i < static_cast<int>(bars)) {
            if (i < 2)      col = TUN_GREEN;
            else if (i < 4) col = TUN_YELLOW;
            else             col = TUN_RED;
        } else {
            col = TUN_DIM;
        }
        tft->fillRect(bx, by, barW, barH, col);
    }
}

// ============================================================================
// V1Display::showTuning
// ============================================================================

void V1Display::showTuning(const TuningData& data) {
    FILL_SCREEN(TUN_BG);

    // -------------------------------------------------------------------------
    // Header bar
    // -------------------------------------------------------------------------

    DRAW_LINE(0, HDR_H - 1, W - 1, HDR_H - 1, TUN_DIM);

    tft_->setTextSize(1);
    tft_->setTextColor(TUN_DIM, TUN_BG);
    tft_->setCursor(4, 7);
    tft_->print("V1 TUNING");

    // Slot name (centre)
    {
        char label[24];
        snprintf(label, sizeof(label), "SLOT %d  %s", data.activeSlot, data.slotName);
        tft_->setTextColor(TUN_WHITE, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), label, W / 2, 7);
    }

    // Mute / V1 state (right)
    {
        tft_->setTextSize(1);
        const char* muteLabel = data.muted ? "MUTED" : (data.v1Connected ? "LIVE" : "SCAN");
        const uint16_t muteCol = data.muted ? TUN_ORANGE : (data.v1Connected ? TUN_GREEN : TUN_GREY);
        tft_->setTextColor(muteCol, TUN_BG);
        GFX_setTextDatum(TR_DATUM);
        GFX_drawString(tft_.get(), muteLabel, W - 4, 7);
    }

    // -------------------------------------------------------------------------
    // Column dividers
    // -------------------------------------------------------------------------

    const int16_t sepY0 = BODY_Y + 4;
    const int16_t sepY1 = H - 4;
    DRAW_LINE(FREQ_COL_X, sepY0, FREQ_COL_X, sepY1, TUN_DIM);
    DRAW_LINE(BARS_COL_X, sepY0, BARS_COL_X, sepY1, TUN_DIM);
    DRAW_LINE(DIR_COL_X,  sepY0, DIR_COL_X,  sepY1, TUN_DIM);

    if (!data.hasAlert) {
        // No alert — show a centred "SCANNING" hint
        tft_->setTextSize(2);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(MC_DATUM);
        GFX_drawString(tft_.get(), "NO ALERT", W / 2, BODY_Y + BODY_H / 2);

        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(BC_DATUM);
        GFX_drawString(tft_.get(), "TAP TO CYCLE  |  TRIPLE-TAP PROFILE", W / 2, H - 2);
        flush();
        return;
    }

    // -------------------------------------------------------------------------
    // BAND column  (X=0..FREQ_COL_X)
    // -------------------------------------------------------------------------

    {
        const int16_t cx = BAND_COL_X + BAND_COL_W / 2;
        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), "BAND", cx, BODY_Y + 4);

        const uint16_t bandCol = getBandColor(data.band);
        tft_->setTextSize(3);
        tft_->setTextColor(bandCol, TUN_BG);
        GFX_setTextDatum(MC_DATUM);
        GFX_drawString(tft_.get(), tunBandName(data.band), cx, BODY_Y + BODY_H / 2 + 8);
    }

    // -------------------------------------------------------------------------
    // FREQ column  (X=FREQ_COL_X..BARS_COL_X)
    // -------------------------------------------------------------------------

    {
        const int16_t cx = FREQ_COL_X + FREQ_COL_W / 2;
        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), "FREQUENCY", cx, BODY_Y + 4);

        if (data.band != BAND_LASER && data.freqMHz > 0) {
            char buf[16];
            const uint32_t intPart  = data.freqMHz / 1000;
            const uint32_t fracPart = (data.freqMHz % 1000) / 10;
            snprintf(buf, sizeof(buf), "%lu.%02lu", intPart, fracPart);

            tft_->setTextSize(2);
            tft_->setTextColor(TUN_WHITE, TUN_BG);
            GFX_setTextDatum(MC_DATUM);
            GFX_drawString(tft_.get(), buf, cx, BODY_Y + BODY_H / 2 - 8);

            tft_->setTextSize(1);
            tft_->setTextColor(TUN_GREY, TUN_BG);
            GFX_setTextDatum(BC_DATUM);
            GFX_drawString(tft_.get(), "GHz", cx, BODY_Y + BODY_H / 2 + 16);

            // Frequency history — last 2 seen (skip index 0, that's current)
            if (data.freqHistCount > 1) {
                tft_->setTextSize(1);
                tft_->setTextColor(TUN_DIM, TUN_BG);
                GFX_setTextDatum(TC_DATUM);
                GFX_drawString(tft_.get(), "PREV", cx, BODY_Y + BODY_H / 2 + 24);
                for (uint8_t i = 1; i < data.freqHistCount && i < 3; ++i) {
                    char hbuf[12];
                    const uint32_t hp = data.freqHistory[i] / 1000;
                    const uint32_t hf = (data.freqHistory[i] % 1000) / 10;
                    snprintf(hbuf, sizeof(hbuf), "%lu.%02lu", hp, hf);
                    const int16_t hy = static_cast<int16_t>(BODY_Y + BODY_H / 2 + 34 + (i - 1) * 14);
                    GFX_drawString(tft_.get(), hbuf, cx, hy);
                }
            }
        } else {
            tft_->setTextSize(2);
            tft_->setTextColor(TUN_GREY, TUN_BG);
            GFX_setTextDatum(MC_DATUM);
            GFX_drawString(tft_.get(), "LASER", cx, BODY_Y + BODY_H / 2 + 4);
        }
    }

    // -------------------------------------------------------------------------
    // SIGNAL BARS column  (X=BARS_COL_X..DIR_COL_X)
    // -------------------------------------------------------------------------

    {
        const int16_t cx = BARS_COL_X + BARS_COL_W / 2;
        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), "SIGNAL", cx, BODY_Y + 4);

        const int16_t barsX = BARS_COL_X + 8;
        const int16_t barsY = BODY_Y + 24;
        const int16_t barsW = BARS_COL_W - 16;
        const int16_t barsH = BODY_H - 36;
        drawSignalBars(tft_.get(), barsX, barsY, barsW, barsH, data.signalBars);

        // Numeric bars / 8 below
        char barBuf[8];
        snprintf(barBuf, sizeof(barBuf), "%d/6", data.signalBars);
        tft_->setTextSize(1);
        tft_->setTextColor(TUN_GREY, TUN_BG);
        GFX_setTextDatum(BC_DATUM);
        GFX_drawString(tft_.get(), barBuf, cx, H - 2);
    }

    // -------------------------------------------------------------------------
    // DIRECTION + DURATION column  (X=DIR_COL_X..W)
    // -------------------------------------------------------------------------

    {
        const int16_t cx = DIR_COL_X + DIR_COL_W / 2;

        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), "DIRECTION", cx, BODY_Y + 4);

        tft_->setTextSize(2);
        tft_->setTextColor(TUN_CYAN, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), tunDirName(data.direction), cx, BODY_Y + 22);

        // Duration
        tft_->setTextSize(1);
        tft_->setTextColor(TUN_DIM, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), "DURATION", cx, BODY_Y + 70);

        char durBuf[12];
        const uint32_t sec = data.durationMs / 1000;
        const uint32_t ms  = data.durationMs % 1000;
        snprintf(durBuf, sizeof(durBuf), "%lu.%01lus", sec, ms / 100);

        tft_->setTextSize(2);
        tft_->setTextColor(TUN_WHITE, TUN_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), durBuf, cx, BODY_Y + 86);
    }

    // -------------------------------------------------------------------------
    // Tap hint
    // -------------------------------------------------------------------------

    tft_->setTextSize(1);
    tft_->setTextColor(TUN_DIM, TUN_BG);
    GFX_setTextDatum(BC_DATUM);
    GFX_drawString(tft_.get(), "TAP TO CYCLE  |  TRIPLE-TAP PROFILE", W / 2, H - 2);

    flush();
}
