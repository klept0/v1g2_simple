# Changelog

All notable changes to the V1-Simple project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [4.2.1] - 2026-06-20

### Fixed
- **LASER text truncated on OFR fonts** — uppercase `R` was missing from the PROGMEM font subsets (JetBrains Mono, Roboto, Atkinson Hyperlegible), causing "LASER" to render as "LASE". Added `R` to `DISPLAY_CHARS` in `tools/create_font_headers.py` and regenerated all three font headers.
- **Toggle page touch zones swapped** — on the AXS15231B with `rotation=1`, touch X is mirrored relative to display X. The WiFi AP and Mute=0 buttons responded to each other's taps. Remapped zones: `touchX < 213` → Mute=0 (right), `213–426` → BLE Proxy (center), `≥ 427` → WiFi AP (left).
- **Mute button label malformed** — `"Mute→0"` embedded raw UTF-8 bytes (`\xE2\x86\x92`) that the GFX bitmap font cannot render. Replaced with plain ASCII `"Mute=0"`.

---

## [4.2.0] - 2026-06-19

### Added
- **Five display fonts** — Classic (7-segment), JetBrains Mono, Roboto, Serpentine, and Atkinson Hyperlegible. Select in Colors → Font Style. The three new OFR fonts (JetBrains Mono, Roboto, Atkinson) are lazy-loaded on first use; only the active font occupies RAM. PROGMEM headers are subsetted to display characters (~6–20 KB each).
- `tools/create_font_headers.py` — reproducible fontTools subsetting script for regenerating the PROGMEM font headers from source TTFs.
- **BOOT button two-page settings UI** — short press now cycles through two pages before exiting. Page 1 (existing): brightness and volume sliders. Page 2 (new): three large tap-toggle buttons for **WiFi AP**, **BLE Proxy**, and **Mute→0** (active slot). A third short press exits and saves. The 4-second long-press WiFi toggle is unaffected.

### Fixed
- **Touch navigation**: Removed zone-based swipe/screen-nav logic (the AXS15231B touch IC fires only a single rising-edge event per tap, making swipe accumulation impossible). Touch now handles only: tap while alert active = mute/unmute; triple-tap within 600 ms = cycle profile.
- **Profile cycling**: Triple-tap (no active alert, within 600 ms) now reliably cycles profiles on any tap position.
- **Web UI hamburger menu**: DaisyUI v4 dropdown requires `tabindex="0"` on both the trigger `<button>` and the `<ul>`; without it the `:focus-within` rule never fires on touch devices.

### Removed
- **Extra screens**: Screens 2–5 (JBV1, History, Diagnostics, Clock) and all associated modules — `ScreenManager`, `HistoryManager`, `JBV1Data`, `wifi_jbv1_api_service`, `history_manager`, `jbv1_client`, `screen_{clock,diag,history,jbv1}`. The display now shows only the radar view.
- **JBV1 GPS integration**: `POST /api/jbv1/update` endpoint, Tasker importable profile, and JBV1 section of the Integrations page removed.

---

## [4.1.0] - 2026-06-13

### Added
- **Multi-screen navigation** — swipe left/right to cycle through 5 screens; tap center to return to radar from any non-radar screen. Any active radar alert immediately forces the display back to the radar screen.
- **Screen 2 — JBV1 driving data** — shows speed, posted speed limit, speed delta, heading, satellite count, and GPS accuracy. Data fed via `POST /api/jbv1/update` with JSON payload `{"speed":72,"speedLimit":65,"heading":"NW","gpsAccuracy":3,"satellites":8}`. Data expires after 10 s of no updates.
- **Screen 3 — Encounter history** — RAM-only circular buffer of the last 10 radar encounters (band, frequency, time ago). Cleared on boot.
- **Screen 4 — Diagnostics** — BLE connection status, RSSI, packet rate, last packet age, and firmware identifier.
- **Screen 5 — Clock/idle** — large time display (12-hour AM/PM) with day/date, derived from web UI time push or NTP fallback via `TimeService`.
- **`ScreenManager`** — global `screenManager` handles screen selection, swipe/tap navigation routing, and alert-override forcing.
- **`HistoryManager`** — global `historyManager` circular buffer (max 10 `Encounter` records).
- **`JBV1Data` / `jbv1_tick()`** — global `g_jbv1` struct with `valid()` check (10 s staleness).
- **`POST /api/jbv1/update`** — REST endpoint accepting JBV1 JSON payload; populates `g_jbv1`.
- **`DisplayBleContext`** extended with `packetsPerSecond` and `lastPacketAgeMs` fields.

- **Voice Packs** — upload custom µ-law (`.mul`) clip sets via the web UI (Audio → Voice Packs) to replace the built-in TTS voice. Any clip not present in a pack falls back to the default voice automatically, so partial packs are fully supported — you only need to provide the clips you want to change.

  **How to create a pack:**

  - *macOS (built-in TTS, no API key):* run `tools/generate_freq_audio.sh` then encode `.raw` → `.mul` with `ffmpeg -acodec pcm_mulaw`.
  - *Google Gemini TTS (highest quality):* `pip install google-genai && python tools/generate_tts.py`; convert output WAVs to `.mul` with ffmpeg.
  - *Custom recordings / any source:* convert with `ffmpeg -ar 22050 -ac 1 -acodec pcm_mulaw`.

  Clip requirements: **µ-law (G.711), mono, 22050 Hz**, named to match the 118 entries in [`config/audio_asset_manifest.json`](config/audio_asset_manifest.json) (e.g. `band_ka.mul`, `tens_34.mul`, `dir_ahead.mul`). Upload via the web UI by entering a pack name and selecting `.mul` files; clips are written to `/audio/<packname>/` on LittleFS.

  **API endpoints added:**
  - `GET /api/audio/voice-packs` — list installed packs with clip counts and active flag
  - `POST /api/audio/voice-pack/activate` — switch active pack; persists to NVS immediately
  - `POST /api/audio/voice-pack/delete` — remove a pack and all its clips
  - `POST /api/audio/voice-pack/upload?pack=<name>` — multipart upload of individual `.mul` clip files

  **Settings:** `activeVoicePack` added to `V1Settings`; persisted in NVS under key `voicePack`; applied at boot alongside volume.

### Changed
- Bumped `softprops/action-gh-release` from `v1` to `v2` in the release workflow to resolve the Node.js 20 deprecation warning (GitHub retires Node.js 20 runners on 2026-09-16).
- Web installer link in README updated to the GitHub Pages URL (`https://klept0.github.io/v1g2_simple/install/`); removed stale `ajmdroid` account references.

---

## [4.0.2] - 2026-06-12

### Added
- JetBrains Mono, Roboto, and Atkinson Hyperlegible font options available in display settings (subsetted TTFs embedded as PROGMEM arrays via OpenFontRender).
- `tools/create_font_headers.py` script to regenerate font headers from source TTFs using `fontTools` subsetting.
- `deferredPersistRetryCount_` tracking in Settings; logs an error after ≥ 5 consecutive NVS persist failures.
- `isWellFormedBleAddress()` validation in OBD runtime; malformed saved addresses are discarded at load time rather than propagated.

### Fixed
- `std::atomic` members (`lastParsedTsMs_`, `hadSuccessfulParse_`) in `BleQueueModule` required explicit move-assignment operator — implicit one is deleted by the standard, causing `test_ble_display_pipeline` to fail to compile on native PlatformIO builds.
- `Serial.printf` call in `setSavedAddressFromBuffer()` was unguarded, causing `undefined reference to 'Serial'` linker errors in `test_obd_runtime` native builds. Wrapped in `#ifndef UNIT_TEST`.
- `cleanupConnection()` now logs a warning when its 20 ms BLE mutex wait times out, making contention visible in serial output.
- `tryBackupBondsToSD()` logs a warning when the SD lock is contended and defers the backup rather than silently dropping it.
- `SD_MMC.begin()` failures now log `errno` for easier hardware-level diagnostics.
- Web installer `ManifestURL` corrected from the original `ajmdroid` account to `klept0`.

### Changed
- `DisplayStyle` enum extended with `DISPLAY_STYLE_JETBRAINS_MONO` (1), `DISPLAY_STYLE_ROBOTO` (2), and `DISPLAY_STYLE_ATKINSON` (4) alongside the existing Classic (0) and Serpentine (3) styles.
- `display_font_manager` extended with lazy-load paths and a `rendererForStyle()` dispatcher for the three new OFR fonts.
- GitHub Actions `deploy-web-installer.yml`, `release-on-merge.yml`, and `refresh-web-installer-assets.yml`: removed `cname: v1simple.com` (domain not owned by this repo).

---

## [4.0.1] - 2026-04-04

### Fixed
- Web installer merged firmware now uses the correct `dio` flash mode for ESP Web Tools on the Waveshare ESP32-S3-Touch-LCD-3.49, preventing browser-flashed boards from appearing dead after install.
- Installer entrypoints now fail over to a secure hosted ESP Web Tools URL when the custom domain is not in a secure context, avoiding browser-side `Serial access not allowed` failures caused by site HTTPS issues.

### Changed
- Added dedicated GitHub Actions workflows to deploy installer HTML changes and manually refresh installer assets without reusing an existing release tag.
- Documentation now points users at the secure hosted installer during the 4.0.1 hotfix rollout.

## [4.0.0] - 2026-04-01

> Note: no tagged releases were published between `3.0.7` and `4.0.0`;
> ongoing work during that gap landed directly in the dev cycle summarized here.

### Added

**Module Extraction (625 commits since 2026-02-15)**
- Extracted 15 module directories under `src/modules/` (139 files, ~17,500 lines).
- Loop phase orchestration extracted to `main_loop_phases.cpp` (~190 lines) with 10 phase-router modules (`LoopIngestModule`, `LoopConnectionEarlyModule`, `LoopDisplayModule`, etc.).
- Core service splits: `ble_runtime.cpp` (511 lines), `packet_parser_alerts.cpp` (582 lines), `settings_restore.cpp` (782 lines).
- Boot-time helpers extracted to `main_boot.cpp` (248 lines).
- Persist save state machines extracted to `main_persist.cpp` (445 lines).
- WiFi subsystem modularized into dedicated runtime, policy, cadence, and visual-sync modules.
- BLE connection runtime and state dispatch modules extracted with Providers DI pattern.
- Speed-volume runtime, speaker-quiet sync, and voice-speed sync modules added.
- `SystemEventBus` and `PeriodicMaintenanceModule` for cross-cutting concerns.
- `DebugPerfFilesService` extracted from debug API for perf-file management.
- 8 CI contract scripts enforcing architectural invariants.

**Quality + Runtime Hardening**
- Expanded to 76 native test suites, 960 test cases (`pio test -e native`).
- Drive-scenario integration tests (15 scenarios).
- OBD runtime module integrated (speed polling, reconnect, scan-from-UI).
- Heap safety hardened with RAII ownership and teardown guards.

- **Security Warning**: Default password warning banner in web UI
  - Shows on all pages when using factory default password
  - Dismissible per session
  - Links to Settings page for easy password change

**WiFi Client Mode**
- Connect to external WiFi networks for internet access
- Maintains AP mode for device access while connected

**Performance**
- Perf CSV schema expanded with subsystem timing (OBD, display, BLE).
- Audio play/busy/fail counters, signal-observation queue drop counters wired.
- Perf CSV SLO scorecard tooling added.

### Changed
- **Architecture**: Main-loop `loop()` reduced to thin orchestrator (~80 lines); `configure*Module()` wiring functions handle DI (~290 lines).
- **DI Patterns**: Three new dependency-injection patterns documented (Providers, Callbacks hybrid, Pass-by-ref).
- **Display**: Per-element `s_force*Redraw` statics replaced by shared `DisplayDirtyFlags` struct with `dirty.setAll()` invalidation.
- **Boot**: BLE initialization reordered before storage; WiFi deferred; settle time absorbed.
- **CI/CD**: Build runs unit tests before firmware compilation; firmware size budget, static analysis, and interface lint gates added.
- **Input Validation**: Added proxy_name length limit (32 chars).
- **API**: Added `isDefaultPassword` flag to `/api/settings` response.
- **Docs**: Full documentation audit — MANUAL.md, API.md aligned to code.

### Fixed
- WiFi STA config recovery when NVS keys are missing (SD secret fallback).
- Display flush contract stabilized against line-offset drift.
- Settings flushed reliably on shutdown.

### Security
- Default password warning encourages users to change factory credentials

---

## [3.0.7] - Previous Release

### Features
- Full V1 BLE connectivity with packet parsing
- OBD-II speed source via BLE
- V1 profile management with auto-push
- Custom display themes and colors
- SD card backup functionality
- Runtime diagnostics and perf counters

### Technical
- ESP32-S3 on Waveshare 3.49" display
- BLE connection (V1)
- FreeRTOS multi-tasking
- LittleFS + SD card storage
- Responsive SvelteKit web interface

---

## Version History

| Version | Date | Highlights |
|---------|------|------------|
| 4.2.1 | 2026-06-20 | Fix LASER truncation on OFR fonts; fix toggle touch zones; fix Mute label |
| 4.2.0 | 2026-06-19 | Five display fonts; radar-only display; removed JBV1/History/Diag/Clock screens |
| 4.1.1 | 2026-06-19 | Zone-based tap navigation, web UI hamburger fix |
| 4.1.0 | 2026-06-13 | Multi-screen navigation, JBV1 screen, voice packs |
| 4.0.1 | 2026-04-04 | Web installer hotfix: corrected merged flash mode, secure hosted fallback |
| 4.0.0 | 2026-04-01 | Modular architecture, 141 module files, 960 tests, CI contracts |
| 3.0.7 | 2026 | Quality baseline before 4.x refactors |
| 3.0.x | 2024 | Speed source improvements |
| 2.x.x | 2024 | Profiles, display themes |
| 1.x.x | 2023 | Initial release, basic V1 display |

---

## Upgrade Notes

### From 4.0.0 to 4.0.1

This hotfix is recommended for anyone installing from the browser-based flasher.

Recommended post-upgrade actions:
1. Use the hosted fallback installer until the custom-domain HTTPS configuration is fully healthy again.
2. If a 4.0.0 browser flash left the board non-booting, recover with USB erase/reflash or re-install with 4.0.1 once the refreshed installer assets are published.

### From 3.x to 4.0.0

API/behavior changes exist versus 3.0.7; validate integrations before production use.

Recommended post-upgrade actions:
1. Change default WiFi password if not already done
2. Review new security warning banner
3. Re-validate any tooling/scripts that call REST endpoints against `docs/API.md`

### From 2.x.x to 3.x.x

**Settings migration required.**

1. Backup settings via web UI before upgrade
2. Perform firmware upgrade
3. Restore settings backup
4. Re-configure any missing options

---

## Development

### Running Tests
```bash
# Run all unit tests
pio test -e native

# Run specific test
pio test -e native -f test_packet_parser
```

### Building Firmware
```bash
# Build + flash firmware/filesystem
./build.sh --all

# Build firmware only
pio run -e waveshare-349
```

### CI Status
- Tests must pass before firmware builds
- Triggered on push to `dev`, `main`, `feature/*`
- Pull requests require passing CI

---

*For detailed API documentation, see [docs/API.md](docs/API.md)*
*For troubleshooting, see [docs/MANUAL.md](docs/MANUAL.md#j-troubleshooting)*
