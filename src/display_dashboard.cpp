/**
 * Driving Dashboard screen renderer for V1-Simple
 * Renders a compact status overview when no radar alert is active.
 *
 * Layout (640×172 landscape):
 *
 *  Y=0..26   Status row: V1 · PROXY · OBD · WiFi · BATT
 *  Y=26..118 Main area: Speed (left) | Profile+Mode (center) | Mute (right)
 *  Y=118..172 Alert summary bar: last seen alert or "NO RECENT ALERTS"
 */

#include "display.h"
#include "modules/display/dashboard_module.h"
#include "../include/config.h"
#include "../include/display_layout.h"
#include "../include/display_draw.h"
#include "../include/display_text.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Colour constants used only on the dashboard
// ============================================================================

static constexpr uint16_t DASH_BG         = 0x0000;  // Black
static constexpr uint16_t DASH_WHITE      = 0xFFFF;
static constexpr uint16_t DASH_GREY       = 0x8410;
static constexpr uint16_t DASH_GREEN      = 0x07E0;
static constexpr uint16_t DASH_RED        = 0xF800;
static constexpr uint16_t DASH_YELLOW     = 0xFFE0;
static constexpr uint16_t DASH_CYAN       = 0x07FF;
static constexpr uint16_t DASH_ORANGE     = 0xFD20;
static constexpr uint16_t DASH_DIM        = 0x4208;  // Very dim grey

// ============================================================================
// Layout constants
// ============================================================================

static constexpr int16_t W  = SCREEN_WIDTH;   // 640
static constexpr int16_t H  = SCREEN_HEIGHT;  // 172

static constexpr int16_t STATUS_ROW_H  = 26;
static constexpr int16_t ALERT_ROW_Y   = 118;
static constexpr int16_t ALERT_ROW_H   = H - ALERT_ROW_Y;   // 54

static constexpr int16_t MAIN_Y        = STATUS_ROW_H;
static constexpr int16_t MAIN_H        = ALERT_ROW_Y - MAIN_Y;  // 92

// Main area column widths
static constexpr int16_t SPEED_COL_W   = 210;
static constexpr int16_t PROFILE_COL_X = SPEED_COL_W;
static constexpr int16_t PROFILE_COL_W = 220;
static constexpr int16_t MUTE_COL_X    = SPEED_COL_W + PROFILE_COL_W;  // 430
static constexpr int16_t MUTE_COL_W    = W - MUTE_COL_X;               // 210

// ============================================================================
// Helpers: small status dot with label
// ============================================================================

// Draw a labelled dot: "V1 ●" where dot is green/red based on `ok`.
// Returns the x position after the drawn element.
static int16_t drawStatusDot(Arduino_Canvas* tft,
                              int16_t x, int16_t y,
                              const char* label, bool ok) {
    const uint16_t dotColor = ok ? DASH_GREEN : DASH_RED;
    tft->setTextSize(1);
    tft->setTextColor(DASH_DIM, DASH_BG);
    tft->setCursor(x, y + 2);
    tft->print(label);

    // Approximate width of label at textSize 1 (6px/char)
    const int16_t lblW = strlen(label) * 6;
    // Draw dot as a small filled circle
    const int16_t dotX = x + lblW + 3;
    const int16_t dotY = y + 7;
    tft->fillCircle(dotX, dotY, 5, dotColor);

    return dotX + 9;  // next x with 3px gap
}

// ============================================================================
// Helpers: band name string
// ============================================================================

static const char* dashBandName(Band b) {
    switch (b) {
        case BAND_LASER: return "LASER";
        case BAND_KA:    return "Ka";
        case BAND_K:     return "K";
        case BAND_X:     return "X";
        default:         return "?";
    }
}

static const char* dashDirName(Direction d) {
    switch (d) {
        case DIR_FRONT: return "AHEAD";
        case DIR_REAR:  return "BEHIND";
        case DIR_SIDE:  return "SIDE";
        default:        return "";
    }
}

// ============================================================================
// V1Display::showDashboard
// ============================================================================

void V1Display::showDashboard(const DashboardData& data) {
    // Full clear
    FILL_SCREEN(DASH_BG);

    // -------------------------------------------------------------------------
    // Status row (Y=0..STATUS_ROW_H)
    // -------------------------------------------------------------------------

    // Thin separator below status row
    DRAW_LINE(0, STATUS_ROW_H - 1, W - 1, STATUS_ROW_H - 1, DASH_DIM);

    // Left side: "DASHBOARD" label
    tft_->setTextSize(1);
    tft_->setTextColor(DASH_DIM, DASH_BG);
    tft_->setCursor(4, 8);
    tft_->print("DASHBOARD");

    // Right side: status dots, right-aligned, spaced left from edge
    {
        // Walk right-to-left. We'll compute positions left-to-right and
        // then offset so the last element sits near the right edge.
        // Simpler: just place them left-to-right from a fixed offset.
        int16_t x = 130;
        const int16_t dotY = 6;

        x = drawStatusDot(tft_.get(), x, dotY, "V1",    data.v1Connected);
        x += 6;
        x = drawStatusDot(tft_.get(), x, dotY, "PROXY",
                           data.proxyEnabled && data.proxyClientConnected);
        x += 6;
        x = drawStatusDot(tft_.get(), x, dotY, "OBD",  data.obdConnected);
        x += 6;
        x = drawStatusDot(tft_.get(), x, dotY, "WiFi", data.wifiApActive);
        x += 6;
        x = drawStatusDot(tft_.get(), x, dotY, "PHONE", data.phoneDataValid);
        x += 6;

        // Battery (show if present)
        if (data.hasBattery) {
            tft_->setTextSize(1);
            const uint16_t battCol = (data.batteryPct < 20) ? DASH_RED
                                   : (data.batteryPct < 50) ? DASH_YELLOW
                                                             : DASH_GREEN;
            tft_->setTextColor(battCol, DASH_BG);
            tft_->setCursor(x, dotY + 2);
            char buf[10];
            snprintf(buf, sizeof(buf), "BAT%d%%", data.batteryPct);
            tft_->print(buf);
        }
    }

    // -------------------------------------------------------------------------
    // Separator between columns in the main area
    // -------------------------------------------------------------------------

    const int16_t sepY0 = MAIN_Y + 4;
    const int16_t sepY1 = ALERT_ROW_Y - 4;
    DRAW_LINE(PROFILE_COL_X,  sepY0, PROFILE_COL_X,  sepY1, DASH_DIM);
    DRAW_LINE(MUTE_COL_X,     sepY0, MUTE_COL_X,     sepY1, DASH_DIM);

    // -------------------------------------------------------------------------
    // Speed column (X=0..SPEED_COL_W)
    // -------------------------------------------------------------------------

    {
        const int16_t colCx = SPEED_COL_W / 2;  // 105

        // "SPEED" label at top of column
        tft_->setTextSize(1);
        tft_->setTextColor(DASH_DIM, DASH_BG);
        tft_->setCursor(colCx - 15, MAIN_Y + 4);
        tft_->print("SPEED");

        if (data.speedValid) {
            // Large 7-segment speed number centred in column
            const int spd = static_cast<int>(data.speedMph + 0.5f);
            char spdBuf[8];
            snprintf(spdBuf, sizeof(spdBuf), "%d", spd);

            // Use 7-seg drawing at scale 1.3 (~36px tall)
            const float scale = 1.3f;
            const int segW = measureSevenSegmentText(spdBuf, scale);
            const int segX = colCx - segW / 2;
            const int segY = MAIN_Y + 22;
            drawSevenSegmentText(spdBuf, segX, segY, scale,
                                 DASH_CYAN, DASH_DIM);

            // "MPH" below number
            tft_->setTextSize(2);
            tft_->setTextColor(DASH_GREY, DASH_BG);
            tft_->setCursor(colCx - 18, MAIN_Y + 70);
            tft_->print("MPH");
        } else {
            // No OBD: show dashes
            const float scale = 1.3f;
            const int segW = measureSevenSegmentText("---", scale);
            const int segX = colCx - segW / 2;
            drawSevenSegmentText("---", segX, MAIN_Y + 22, scale,
                                 DASH_DIM, DASH_DIM);

            tft_->setTextSize(1);
            tft_->setTextColor(DASH_DIM, DASH_BG);
            tft_->setCursor(colCx - 15, MAIN_Y + 72);
            tft_->print("NO OBD");
        }
    }

    // -------------------------------------------------------------------------
    // Profile + Mode column (X=PROFILE_COL_X..MUTE_COL_X)
    // -------------------------------------------------------------------------

    {
        const int16_t colCx = PROFILE_COL_X + PROFILE_COL_W / 2;  // 320

        // "PROFILE" label
        tft_->setTextSize(1);
        tft_->setTextColor(DASH_DIM, DASH_BG);
        tft_->setCursor(colCx - 18, MAIN_Y + 4);
        tft_->print("PROFILE");

        // Slot name (large)
        tft_->setTextSize(2);
        tft_->setTextColor(DASH_WHITE, DASH_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), data.slotName, colCx, MAIN_Y + 20);

        // Mode label below slot name
        const char* modeName = "---";
        if (data.modeChar == 'A' || data.modeChar == 'a') modeName = "ALL BOGEYS";
        else if (data.modeChar == 'L' || data.modeChar == 'l') modeName = "LOGIC";
        else if (data.modeChar == 'c' || data.modeChar == 'C') modeName = "ADV LOGIC";
        else if (data.modeChar != 0) modeName = "CUSTOM";

        tft_->setTextSize(1);
        tft_->setTextColor(DASH_GREY, DASH_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), modeName, colCx, MAIN_Y + 52);
    }

    // -------------------------------------------------------------------------
    // Mute column (X=MUTE_COL_X..W)
    // -------------------------------------------------------------------------

    {
        const int16_t colCx = MUTE_COL_X + MUTE_COL_W / 2;  // 535

        // "MUTE" label
        tft_->setTextSize(1);
        tft_->setTextColor(DASH_DIM, DASH_BG);
        tft_->setCursor(colCx - 10, MAIN_Y + 4);
        tft_->print("MUTE");

        if (data.muted) {
            tft_->setTextSize(2);
            tft_->setTextColor(DASH_ORANGE, DASH_BG);
            GFX_setTextDatum(TC_DATUM);
            GFX_drawString(tft_.get(), "MUTED", colCx, MAIN_Y + 28);
        } else {
            tft_->setTextSize(2);
            tft_->setTextColor(DASH_GREEN, DASH_BG);
            GFX_setTextDatum(TC_DATUM);
            GFX_drawString(tft_.get(), "OFF", colCx, MAIN_Y + 28);
        }

        // V1 connection status below mute
        tft_->setTextSize(1);
        const char* v1status = data.v1Connected ? "V1 CONNECTED" : "V1 SCANNING";
        const uint16_t v1col = data.v1Connected ? DASH_GREEN : DASH_GREY;
        tft_->setTextColor(v1col, DASH_BG);
        GFX_setTextDatum(TC_DATUM);
        GFX_drawString(tft_.get(), v1status, colCx, MAIN_Y + 60);
    }

    // -------------------------------------------------------------------------
    // Alert summary row (Y=ALERT_ROW_Y..H)
    // -------------------------------------------------------------------------

    // Thin separator above alert row
    DRAW_LINE(0, ALERT_ROW_Y, W - 1, ALERT_ROW_Y, DASH_DIM);

    {
        const int16_t rowCy = ALERT_ROW_Y + ALERT_ROW_H / 2;

        if (data.hasLastAlert) {
            // "LAST:  Ka  34.7  AHEAD"
            tft_->setTextSize(1);
            tft_->setTextColor(DASH_DIM, DASH_BG);
            tft_->setCursor(6, rowCy - 4);
            tft_->print("LAST:");

            // Band
            tft_->setTextSize(2);
            const uint16_t bandCol = getBandColor(data.lastBand);
            tft_->setTextColor(bandCol, DASH_BG);
            tft_->setCursor(48, rowCy - 8);
            tft_->print(dashBandName(data.lastBand));

            // Frequency (if not laser)
            if (data.lastBand != BAND_LASER && data.lastFreqMhz > 0) {
                char freqBuf[12];
                const uint32_t intPart = data.lastFreqMhz / 1000;
                const uint32_t fracPart = (data.lastFreqMhz % 1000) / 10;
                snprintf(freqBuf, sizeof(freqBuf), "%lu.%02lu", intPart, fracPart);
                tft_->setTextSize(2);
                tft_->setTextColor(DASH_WHITE, DASH_BG);
                tft_->setCursor(148, rowCy - 8);
                tft_->print(freqBuf);
                tft_->setTextSize(1);
                tft_->setTextColor(DASH_DIM, DASH_BG);
                tft_->setCursor(260, rowCy - 2);
                tft_->print("GHz");
            }

            // Direction
            if (data.lastDirection != DIR_NONE) {
                tft_->setTextSize(2);
                tft_->setTextColor(DASH_YELLOW, DASH_BG);
                tft_->setCursor(310, rowCy - 8);
                tft_->print(dashDirName(data.lastDirection));
            }
        } else {
            tft_->setTextSize(1);
            tft_->setTextColor(DASH_DIM, DASH_BG);
            GFX_setTextDatum(TC_DATUM);
            GFX_drawString(tft_.get(), "NO RECENT ALERTS", W / 2, rowCy - 3);
        }
    }

    // Phone companion info — right side of alert row (X=440..W)
    if (data.phoneDataValid) {
        const int16_t px = 440;
        const int16_t py = ALERT_ROW_Y + 4;
        tft_->setTextSize(1);

        if (data.hasRoadName && data.roadName[0]) {
            tft_->setTextColor(DASH_CYAN, DASH_BG);
            tft_->setCursor(px, py);
            tft_->print(data.roadName);
        }

        if (data.hasHeading) {
            static const char* const kCards[] = {
                "N","NE","E","SE","S","SW","W","NW"
            };
            char headBuf[12];
            const char* card = kCards[((data.heading + 22) / 45) % 8];
            snprintf(headBuf, sizeof(headBuf), "%s %u\xb0", card, (unsigned)data.heading);
            tft_->setTextColor(DASH_WHITE, DASH_BG);
            tft_->setCursor(px, py + 12);
            tft_->print(headBuf);
        }

        if (data.hasPhoneBattery) {
            char battBuf[10];
            snprintf(battBuf, sizeof(battBuf), "Ph:%u%%", (unsigned)data.phoneBattery);
            tft_->setTextColor(data.phoneBattery < 20 ? DASH_RED : DASH_GREY, DASH_BG);
            tft_->setCursor(px + 100, py + 12);
            tft_->print(battBuf);
        }
    }

    // -------------------------------------------------------------------------
    // Tap hint — tiny text bottom-right
    // -------------------------------------------------------------------------

    tft_->setTextSize(1);
    tft_->setTextColor(DASH_DIM, DASH_BG);
    GFX_setTextDatum(BR_DATUM);
    GFX_drawString(tft_.get(), "TAP TO EXIT", W - 2, H - 2);

    flush();
}
