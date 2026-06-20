# V1 Gen2 Simple Display

A open-source touchscreen companion display for the **Valentine One Gen2** radar detector. It connects to the V1 over BLE, shows live alert data on a 3.49" color touchscreen, speaks alerts through a built-in speaker, and exposes a full web UI for configuration — all running on a $30 ESP32-S3 module with no cloud dependency.

**Hardware:** [Waveshare ESP32-S3-Touch-LCD-3.49](https://www.amazon.com/dp/B0FQM41PGX) (~$30)

---

## What it does

| Capability | Details |
|---|---|
| **Live alert display** | Band, frequency, signal strength (6 bars), direction arrow, bogey count |
| **Voice alerts** | Spoken announcements — "Ka 34.7 ahead", "2 bogeys" — from a built-in speaker |
| **Custom voice packs** | Upload your own `.mul` audio clips to replace the built-in TTS voice |
| **V1 profile management** | Store up to 3 sensitivity profiles; triple-tap display to switch slots instantly |
| **Auto-push** | Automatically pushes the active profile to V1 on BLE connect |
| **BLE proxy** | Re-advertises V1 data so a phone app (Escort Live, YaV1) can connect simultaneously |
| **Speed-based mute** | Silences alerts below a configurable speed threshold via OBD-II |
| **Full web UI** | SvelteKit interface served from the device — no app, no account, no internet |
| **WiFi client mode** | Optionally joins your home network while keeping the AP active |
| **OTA-style install** | One-click browser flasher via ESP Web Tools; no IDE needed |

---

## Tech stack

| Layer | Technology |
|---|---|
| **MCU** | ESP32-S3 (Xtensa dual-core 240 MHz, 8 MB PSRAM, 16 MB flash) |
| **Framework** | Arduino + FreeRTOS (PlatformIO build) |
| **Display** | 3.49" 320×960 IPS via SPI; rendered with LVGL-adjacent direct framebuffer |
| **BLE** | ESP-IDF BLE stack; full V1 packet parser (alerts, profiles, sweeps) |
| **Storage** | LittleFS (firmware partition) + SD card (backup, bond store) |
| **Audio** | µ-law (G.711) clips concatenated at runtime; decoded via 256-entry lookup table |
| **Web UI** | SvelteKit + Vite; built to static files embedded in LittleFS |
| **API** | ESP32 WebServer; REST JSON endpoints; multipart upload for voice packs |
| **CI** | GitHub Actions: 76 native unit test suites (960 cases), firmware size budget, interface lint, architectural contract checks |

---

## Quick Install

> **Before updating firmware:** back up your settings first — Settings page → Download Backup.

### Option 1: Web Installer (no tools needed)

Chrome + USB cable is all you need:

👉 **[Install via Web](https://klept0.github.io/v1g2_simple/install/)**

1. Hold **POWER + GEAR** while plugging in USB (bootloader mode)
2. Click "Install V1-Simple" and select your device
3. Wait for install to complete, then press **RESET**

### Option 2: Build from Source

**Prerequisites:** VS Code + [PlatformIO](https://platformio.org/install/ide?install=vscode), Node.js 18+, USB-C data cable

```bash
git clone https://github.com/klept0/v1g2_simple
cd v1g2_simple
./build.sh --all
```

First build downloads ~500 MB of toolchain (2–5 min). Subsequent builds are 30–60 seconds.

- Windows users: [docs/MANUAL.md — Windows Setup](docs/MANUAL.md#windows-setup)
- Full build reference: [docs/MANUAL.md](docs/MANUAL.md)

---

## First use

1. Press **RESET** — device shows boot splash, then "SCAN" animation
2. Plug in your V1 Gen2 — it pairs automatically over BLE within a few seconds
3. **Long press BOOT (~4s)** to start the WiFi access point
4. Connect to **`V1-Simple`** / password **`setupv1g2`**
5. Open **`http://192.168.35.5`** in a browser to configure

Change the default password from the Settings page before putting the device on a shared network.

---

## Controls

### Physical buttons

| Button | Action | Function |
|---|---|---|
| **BOOT** | Short press | Enter settings mode (brightness + volume sliders) |
| **BOOT** | Long press ~4s | Toggle WiFi AP on/off |

**Settings mode:** top slider (green) = display brightness; bottom slider (blue) = voice volume. Release the slider to hear a test clip. Short press BOOT again to save and exit.

### Touch gestures

| Gesture | When | Function |
|---|---|---|
| Single tap | Alert active | Mute / unmute the alert |
| Triple tap | No alert | Cycle profile slot (0 → 1 → 2 → 0) |

---

## Features

### Screens

The 640×172 display shows a single radar screen.

| Tap gesture | Action |
|-------------|--------|
| Any tap while alert is active | Mute/unmute |
| Triple tap within 600ms (no active alert) | Cycle profile |

| # | Screen | Description |
|---|--------|-------------|
| 1 | **Radar** | Standard V1 alert display (frequency, band, signal bars, direction arrow) |

### Voice alerts

Alerts are announced by concatenating individual audio clips at runtime. A Ka alert at 34.749 GHz ahead plays:

```
band_ka  +  tens_34  +  digit_7  +  tens_49  +  dir_ahead
  "Ka"    "thirty-four"  "seven"  "forty-nine"   "ahead"
```

- **Priority alerts** — full announcement on new alert; direction-only on movement
- **Bogey count** — optional "2 bogeys", "3 bogeys" suffix
- **Secondary alerts** — per-band filter; threat escalation when a weak signal goes strong
- **Auto-mute** — silences when a phone app is connected to the BLE proxy
- **5-second cooldown** — prevents rapid-fire re-announcements

Configure at `http://192.168.35.5/audio`.

#### Voice packs

Upload custom `.mul` clip sets to replace the built-in TTS voice. Any clip not in the pack falls back to the default — partial packs are fully supported.

**Clip format:** µ-law (G.711), mono, 22050 Hz, `.mul` extension.

```bash
ffmpeg -i input.wav -ar 22050 -ac 1 -acodec pcm_mulaw my_clip.mul
```

**Generation options:**

| Method | Tool | Notes |
|---|---|---|
| macOS TTS | `tools/generate_freq_audio.sh` | Uses `say` (Samantha voice); no API key |
| Google Gemini TTS | `tools/generate_tts.py` | Highest quality; requires Gemini API key |
| Custom recording | Any recorder + ffmpeg | Record in a quiet room; match loudness across clips |

Upload via **Audio → Voice Packs** in the web UI. Enter a pack name, select your `.mul` files, click Upload, then click **Use** to activate. The active pack survives reboots and can be switched at any time.

See [full voice pack reference](#audio-audio) for the complete 118-clip manifest and troubleshooting.

---

### Profiles and Auto-Push

Store up to 3 V1 sensitivity configurations as named profiles (e.g. Highway, City, Stealth). Assign each to a slot and triple-tap the display to switch between them — the active profile is automatically pushed to the V1 on connect.

Each slot independently controls: profile, V1 mode (All Bogeys / Logic / Advanced Logic), volume, dark mode, mute-to-zero, alert persistence, and priority-arrow-only display.

Configure at `http://192.168.35.5/profiles` and `http://192.168.35.5/autopush`.

---

### Display customization

Every color on the display is individually configurable — band indicators, direction arrows, signal bars (6 levels), bogey counter, frequency readout, status icons (WiFi, BLE, RSSI, battery), and muted/persisted states. Two display fonts: **Classic** (7-segment style) and **Serpentine**, plus JetBrains Mono, Roboto, and Atkinson Hyperlegible.

Configure at `http://192.168.35.5/colors`.

---

### BLE proxy

When enabled, the device re-advertises V1 data under a configurable name (default: `V1-Proxy`), allowing a phone app to connect simultaneously. The proxy and the display operate independently — the display does not depend on the phone app being present.

---

### Speed-based mute

Requires an OBD-II adapter connected via BLE. Set a speed threshold; alerts are suppressed below it (useful in slow traffic). Configurable from the Settings page.

---

## Web interface

| Page | URL | Purpose |
|---|---|---|
| Audio | `/audio` | Voice alerts, volume fade, speed mute, voice packs |
| Profiles | `/profiles` | Create and manage V1 sensitivity profiles |
| Auto-Push | `/autopush` | Assign profiles to slots; configure per-slot V1 settings |
| Colors | `/colors` | Display colors, fonts, icon visibility |
| Settings | `/settings` | AP credentials, BLE proxy, backup/restore |

Full REST API documented in [docs/API.md](docs/API.md).

---

## For developers

### Repository layout

```
src/                    C++ firmware (Arduino/FreeRTOS)
  modules/wifi/         WiFi API service modules (one file per route group)
  audio_voice.cpp       Runtime clip concatenation and voice pack resolution
  settings*.cpp         Settings split across load / setters / NVS persistence
interface/              SvelteKit web UI (compiled to static files in data/)
  src/routes/           One directory per page
  src/lib/              Shared components, utilities, fetchWithTimeout
config/                 Audio asset manifest (118 clip definitions)
tools/                  TTS generation scripts
test/                   76 native unit test suites (PlatformIO native env)
.github/workflows/      CI: build + test, release, Pages deploy
```

### Architecture notes

- **Settings** are split across four files: `settings.h` (struct), `settings.cpp` (load/NVS), `settings_setters.cpp` (mutation), `settings_nvs.cpp` (write). NVS keys live in `include/settings_keys.h`.
- **`settingsManager`** is a global, not a member of `WiFiManager`. Lambdas in `wifi_runtimes.cpp` access it directly.
- **HTTP in the UI** must go through `fetchWithTimeout` from `$lib/utils/poll` — raw `fetch()` calls fail CI.
- **Multipart upload** uses the 3-argument `server.on()` form with separate done and upload handlers.
- **Audio clips** are decoded via a 256-entry µ-law lookup table at playback time; no codec library required.

### Build and test

```bash
# Build firmware + filesystem and flash
./build.sh --all

# Run all 960 unit tests (native, no hardware needed)
pio test -e native

# Run a specific test suite
pio test -e native -f test_packet_parser

# Filesystem upload only
./build.sh --upload-fs
```

Authoritative filesystem upload path: `./build.sh --upload-fs` or `./build.sh --all`.

CI runs on every push to `main`, `dev`, and `feature/*`. Tests must pass before firmware compiles.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| V1 won't connect | Disconnect phone apps from V1 first; power cycle both devices |
| Can't find WiFi AP | Long-press BOOT (~4s) to start AP — WiFi is off by default |
| Upload fails | Try a different USB-C data cable (charge-only cables have no data lines) |
| Display shows nothing | Hold POWER + GEAR while plugging in USB to enter bootloader; reflash |

Full troubleshooting guide: [docs/MANUAL.md — Troubleshooting](docs/MANUAL.md#j-troubleshooting)

---

## Documentation

| Doc | Contents |
|---|---|
| [docs/MANUAL.md](docs/MANUAL.md) | Architecture, BLE protocol, display, developer guide, Windows setup, troubleshooting |
| [docs/API.md](docs/API.md) | Full REST API reference with request/response schemas |
| [docs/OBSERVABILITY.md](docs/OBSERVABILITY.md) | Observability surfaces, metric naming, perf counter derivation |
| [docs/PERF_SLOS.md](docs/PERF_SLOS.md) | Performance thresholds and SLO scoring |

---

## Credits

Originally forked from [ajmdroid/v1g2_simple](https://github.com/ajmdroid/v1g2_simple) — the first public version of this project.

Built on [Kenny Garreau's V1G2-T4S3](https://github.com/kennygarreau/v1g2-t4s3) — go star his repo!

**MIT License** — Use at your own risk. No warranty.
