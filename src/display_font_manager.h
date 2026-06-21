/**
 * DisplayFontManager — owns all OpenFontRender instances, init flags,
 * glyph caches, and lazy-load helpers that were previously scattered as
 * file-scope statics in display.cpp.
 *
 * Lifetime: a single global instance (`fontMgr`) is defined in display.cpp
 *           and declared extern here for use by display sub-modules.
 *           init() is called once from V1Display::begin().
 *
 * Threading: same single-thread contract as the rest of the display system.
 */
#pragma once

#include "OpenFontRender.h"
#include <cstring>
#include <memory>

class Arduino_Canvas;

struct DisplayFontManager {

    // --- Layout constant shared with display drawing code ---
    static constexpr int TOP_COUNTER_FONT_SIZE = 60;

    // --- Renderers ---
    OpenFontRender segment7;    // Classic style (Segment7) — eager-loaded
    OpenFontRender topCounter;  // Dedicated Segment7 for bogey counter — eager-loaded
    OpenFontRender serpentine;  // Serpentine — lazy-loaded on first use
    OpenFontRender jetbrains;   // JetBrains Mono — lazy-loaded on first use
    OpenFontRender roboto;      // Roboto — lazy-loaded on first use
    OpenFontRender atkinson;    // Atkinson Hyperlegible — lazy-loaded on first use
    OpenFontRender din1451;     // Barlow Condensed Bold / DIN 1451 style — lazy-loaded
    OpenFontRender inter;       // Inter Medium — secondary UI font — lazy-loaded

    // --- Init flags ---
    bool segment7Ready    = false;
    bool topCounterReady  = false;
    bool serpentineReady  = false;
    bool jetbrainsReady   = false;
    bool robotoReady      = false;
    bool atkinsonReady    = false;
    bool din1451Ready     = false;
    bool interReady       = false;

    // --- Font cache budget (set once during init) ---
    uint32_t numericCacheBytes       = 8192u;
    bool     serpentineLoadAttempted = false;
    bool     jetbrainsLoadAttempted  = false;
    bool     robotoLoadAttempted     = false;
    bool     atkinsonLoadAttempted   = false;
    bool     din1451LoadAttempted    = false;
    bool     interLoadAttempted      = false;

    // --- Top-counter glyph bounds cache ---
    static constexpr int16_t BOUNDS_INVALID =
        static_cast<int16_t>(-32768);
    int16_t topCounterXMin[128][2];
    int16_t topCounterXMax[128][2];
    bool    topCounterBoundsReady = false;

    // --- Lifecycle ---

    /// Load Segment7 + TopCounter fonts, prime the top-counter bounds cache.
    /// Serpentine is deferred until ensureSerpentineLoaded().
    void init(Arduino_Canvas* canvas);
    void init(const std::unique_ptr<Arduino_Canvas>& canvas) { init(canvas.get()); }

    /// Pre-render common frequency glyphs once so first live alert draws
    /// don't stall while OpenFontRender builds glyph caches.
    void prewarmSegment7FrequencyGlyphs();

    /// Lazy-load a non-Segment7 font the first time it is requested.
    /// Each returns true when the renderer is ready to use.
    bool ensureSerpentineLoaded(Arduino_Canvas* canvas);
    bool ensureJetBrainsLoaded(Arduino_Canvas* canvas);
    bool ensureRobotoLoaded(Arduino_Canvas* canvas);
    bool ensureAtkinsonLoaded(Arduino_Canvas* canvas);
    bool ensureDIN1451Loaded(Arduino_Canvas* canvas);
    bool ensureInterLoaded(Arduino_Canvas* canvas);

    bool ensureSerpentineLoaded(const std::unique_ptr<Arduino_Canvas>& c) { return ensureSerpentineLoaded(c.get()); }
    bool ensureJetBrainsLoaded(const std::unique_ptr<Arduino_Canvas>& c)  { return ensureJetBrainsLoaded(c.get()); }
    bool ensureRobotoLoaded(const std::unique_ptr<Arduino_Canvas>& c)     { return ensureRobotoLoaded(c.get()); }
    bool ensureAtkinsonLoaded(const std::unique_ptr<Arduino_Canvas>& c)   { return ensureAtkinsonLoaded(c.get()); }
    bool ensureDIN1451Loaded(const std::unique_ptr<Arduino_Canvas>& c)    { return ensureDIN1451Loaded(c.get()); }
    bool ensureInterLoaded(const std::unique_ptr<Arduino_Canvas>& c)      { return ensureInterLoaded(c.get()); }

    // --- Top-counter bounds helpers ---

    void resetTopCounterBoundsCache();
    void primeTopCounterBoundsCache();
    bool getTopCounterBounds(char symbol, bool showDot, int& xMin, int& xMax);

    // --- Text width cache ---

    /// Small fixed-size LRU cache for OFR text widths.  Re-used by every
    /// drawFrequency* variant that needs cached bounding-box queries.
    struct WidthCacheEntry {
        bool valid    = false;
        char text[16] = {0};
        int  width    = 0;
    };

    /// Look up (or compute + cache) the pixel width of @p text at @p fontSize.
    /// The cache is stored in the caller's local static array so that each
    /// rendering path maintains its own independent history.
    template <size_t N>
    static int cachedTextWidth(OpenFontRender& renderer, int fontSize,
                               const char* text,
                               WidthCacheEntry (&cache)[N],
                               uint8_t& nextSlot);

};

// --- Template implementation (must be visible to all callers) ---
template <size_t N>
int DisplayFontManager::cachedTextWidth(
        OpenFontRender& renderer, int fontSize, const char* text,
        WidthCacheEntry (&cache)[N], uint8_t& nextSlot) {

    for (size_t i = 0; i < N; ++i) {
        if (cache[i].valid && strcmp(cache[i].text, text) == 0) {
            return cache[i].width;
        }
    }

    renderer.setFontSize(fontSize);
    FT_BBox bbox = renderer.calculateBoundingBox(
        0, 0, fontSize, Align::Left, Layout::Horizontal, text);
    int width = bbox.xMax - bbox.xMin;

    WidthCacheEntry& dst = cache[nextSlot];
    dst.valid = true;
    strncpy(dst.text, text, sizeof(dst.text));
    dst.text[sizeof(dst.text) - 1] = '\0';
    dst.width = width;

    nextSlot = static_cast<uint8_t>((nextSlot + 1) % N);
    return width;
}

// Global font manager instance — defined in display.cpp
extern DisplayFontManager fontMgr;
