# V1 Gen2 Simple Display

A touchscreen display for the Valentine One Gen2 radar detector.

**Hardware:** [Waveshare ESP32-S3-Touch-LCD-3.49](https://www.amazon.com/dp/B0FQM41PGX)

---

## Quick Install

> ⚠️ **Before updating firmware:** Download a backup of your settings from the Settings page (`/settings` → Download Backup). This preserves your colors, profiles, and configuration.

### Option 1: Web Installer (Easiest)

No tools needed — just a Chrome browser and USB cable:

👉 **[Install via Hosted Web](https://klept0.github.io/v1g2_simple/install/)**

1. Put device in bootloader mode (hold POWER + GEAR while plugging in USB)
2. Click "Install V1-Simple" and select your device
3. Wait for install to complete, then press RESET

### Option 2: Build from Source

#### Prerequisites
- Visual Studio Code with [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- Node.js 18+ (for building web UI)
- USB-C data cable (not charge-only)

#### Build & Flash

```bash
git clone https://github.com/klept0/v1g2_simple
cd v1g2_simple
./build.sh --all
```

The script auto-detects your OS. First build takes 2-5 minutes to download libraries.

Authoritative filesystem upload path: `./build.sh --upload-fs` or `./build.sh --all`.
Do not use raw `pio ... uploadfs` as the normal workflow; it bypasses the repo's deploy/audio staging path.

For manual PlatformIO command variants and OS-specific troubleshooting:
- Windows setup: [docs/MANUAL.md — Windows Setup](docs/MANUAL.md#windows-setup)
- Full command reference: [docs/MANUAL.md](docs/MANUAL.md)

Hardware regression runs use the root wrapper:

```bash
./test.sh --all
```

---

## Button Controls

The **BOOT** button (top right, looking at the display) controls WiFi and settings:

| Action | Function |
|--------|----------|
| **Short press** | Enter settings mode (brightness + voice volume) |
| **Long press (~4s)** | Toggle WiFi AP on/off |

### Settings Mode
1. Short press BOOT → settings sliders appear
2. **Top slider (green):** Display brightness (left = dim, right = bright)
3. **Bottom slider (blue):** Voice alert volume (left = quiet, right = loud)
   - Releases the slider to hear a test voice ("Ka ahead")
4. Short press BOOT again to save and exit

### WiFi Access Point
WiFi is **off by default** to save power. To access web settings:

1. Long press BOOT (~4s) → WiFi icon appears on display
2. Connect your phone/computer to:
   - **Network:** `V1-Simple`
   - **Password:** `setupv1g2`
3. Open browser: `http://192.168.35.5`
4. Long press BOOT again to turn WiFi off when done

---

## Touch Controls

| Gesture | When | Function |
|---------|------|----------|
| **Single tap** | Alert active | Mute/unmute the alert |
| **Triple tap** | No alert | Cycle profile slots (0→1→2→0) |

---

## Voice Alerts

The display has a built-in speaker that announces alerts:

### Priority Alerts
- **New alert:** Full announcement with band, frequency, and direction (e.g., "Ka 34.712 ahead")
- **Direction change:** Direction-only announcement when same alert moves (e.g., "behind")
- **Bogey count:** Optionally append "2 bogeys", "3 bogeys" when multiple alerts active
- **Laser:** Always includes direction ("Laser ahead") since there's no frequency to announce

### Secondary Alerts (Optional)
When enabled, non-priority alerts are announced after the priority stabilizes:
- **Per-band filters:** Choose which bands (Ka, K, X, Laser) to announce as secondary
- **Threat escalation:** When a secondary alert ramps from weak (≤2 bars) to strong (≥4 bars sustained), announces direction breakdown (e.g., "2 bogeys, 1 ahead, 1 behind")

### General
- **Auto-disable:** Voice alerts mute when a phone app is connected
- **5-second cooldown:** Prevents rapid-fire announcements

**Volume:** Adjust via the **blue slider** in settings mode (short press BOOT).

**Configure:** Go to `http://192.168.35.5/audio` to customize voice content, direction, bogey count, and secondary alerts.

---

## Web Interface Setup

### Profiles (/profiles)

Create V1 settings profiles to push to your detector:

1. Go to `http://192.168.35.5/profiles`
2. Click **"Pull from V1"** to capture current V1 settings
3. Review settings, then click **Save**
4. Name it (e.g., "Highway", "City", "Stealth")
5. Repeat to create additional profiles with different V1 configurations

### Auto-Push (/autopush)

Set up 3 quick-switch slots that automatically configure your V1:

| Slot | Suggested Use |
|------|---------------|
| 0 | Default / everyday |
| 1 | Highway / max sensitivity |
| 2 | Comfort / quieter urban |

**Setup each slot:**
1. Go to `http://192.168.35.5/autopush`
2. Select a profile from dropdown (created above)
3. Set V1 mode (All Bogeys / Logic / Advanced Logic)
4. Optional: Set main volume (0-9) and mute volume (0-9)
5. Optional: Enable **Dark Mode** (dims V1's display)
6. Optional: Enable **Mute to Zero** (complete silence when muted)
7. Optional: Set **Alert Persistence** (0-5 sec ghost after alert clears)
8. Optional: Enable **Priority Arrow Only** (shows only strongest alert direction)
9. Click **Save**

**Enable Auto-Push:** Toggle on "Auto-Push on Connect" to apply active slot when V1 connects.

**Switch slots:** Triple-tap the display (when no alert) to cycle 0→1→2→0.

### Colors (/colors)

Customize every color on the display:

1. Go to `http://192.168.35.5/colors`
2. **Display Style:** Classic (7-segment) or Serpentine font
3. **Custom colors:** Click any color swatch to open the color picker:
   - Band indicators (L, Ka, K, X)
   - Direction arrows (Front, Side, Rear)
   - Signal bars (6 levels, weak to strong)
   - Bogey counter, frequency, muted/persisted states
   - WiFi icons (AP mode, client connected)
   - BLE icons (connected, disconnected)
   - RSSI labels (V1 signal, Proxy signal)
4. **Test:** Click "Test" to preview colors on display
5. **Hide icons:** Toggle off WiFi, battery, BLE, RSSI, or profile indicator
6. Click **Save**

### Settings (/settings)

General configuration:

- **AP Name/Password:** Change WiFi network name and password
- **BLE Proxy:** Enable to relay V1 data to companion app (advertises as \"V1-Proxy\")
- **Proxy Name:** Change BLE advertised name
- **Backup & Restore:** Download all settings to JSON file, or restore from a previous backup

### Audio (/audio)

Voice alert options:

- **Enable Voice Alerts:** Toggle spoken announcements on/off
- **Mute Voice at Volume 0:** Silence alerts when V1 volume is 0 (warning still plays)
- **Volume Fade:** Reduce V1 volume after initial alert, restore for new threats
- **Speed-Based Mute:** Mute V1 alerts below a configurable speed threshold (requires OBD)

#### Voice Packs

The Audio page includes a **Voice Packs** section that lets you upload custom clip sets to replace the built-in TTS voice. Any clip not present in a pack falls back to the default voice automatically — **partial packs are fully supported**, so you only need to upload the clips you want to change.

---

##### How the clip system works

Each voice alert is assembled at runtime by concatenating individual audio clips. For example, a Ka-band alert at 34.749 GHz coming from ahead plays:

```
band_ka.mul  +  tens_34.mul  +  digit_7.mul  +  tens_49.mul  +  dir_ahead.mul
   "Ka"           "thirty four"     "seven"        "forty nine"      "ahead"
```

The full list of required clip filenames is defined in [`config/audio_asset_manifest.json`](config/audio_asset_manifest.json):

| Filename pattern | Count | Purpose |
|---|---|---|
| `band_ka.mul`, `band_k.mul`, `band_x.mul`, `band_laser.mul` | 4 | Band names |
| `dir_ahead.mul`, `dir_behind.mul`, `dir_side.mul` | 3 | Directions |
| `bogeys.mul` | 1 | "bogeys" word for count announcements |
| `digit_0.mul` … `digit_9.mul` | 10 | Single digit (hundreds place of MHz) |
| `tens_00.mul` … `tens_99.mul` | 100 | Two-digit numbers (GHz token + MHz last two digits) |

**Total: 118 clips.** You only need to provide the ones you want to override.

---

##### Clip format

All clips must be **µ-law encoded, mono, 22050 Hz** audio with the `.mul` extension.

Convert any WAV file with ffmpeg:

```bash
ffmpeg -i input.wav -ar 22050 -ac 1 -acodec pcm_mulaw my_clip.mul
```

Requirements:
- Sample rate: **22050 Hz** (device plays at this rate; other rates will sound wrong)
- Channels: **mono** (1 channel)
- Codec: **pcm_mulaw** (G.711 µ-law, 8-bit compressed)
- Keep clips short — band clips should be under 0.5s, number clips under 0.3s for natural-sounding concatenation

---

##### Method 1 — macOS built-in TTS (fastest, no API key needed)

Requires macOS with ffmpeg installed (`brew install ffmpeg`).

```bash
cd tools
./generate_freq_audio.sh        # generates all 118 .raw files in tools/freq_audio/
```

This uses the macOS `say` command with the Samantha voice at a slightly faster rate. Edit `VOICE` and `RATE` at the top of the script to use a different voice (run `say -v '?'` to list available voices).

After generation, the `.raw` files are raw 16-bit PCM — convert to `.mul` with ffmpeg:

```bash
for f in tools/freq_audio/*.raw; do
  base=$(basename "$f" .raw)
  ffmpeg -y -f s16le -ar 22050 -ac 1 -i "$f" \
         -acodec pcm_mulaw "tools/freq_audio/mulaw/${base}.mul" 2>/dev/null
done
```

---

##### Method 2 — Google Gemini TTS (highest quality)

Requires a Gemini API key and the `google-genai` Python package.

```bash
pip install google-genai
export GEMINI_API_KEY=your_key_here
python tools/generate_tts.py
```

The script generates WAV files using Gemini's TTS model. Convert to `.mul` with ffmpeg as shown above.

---

##### Method 3 — Record your own clips

Record any audio source (your own voice, a text-to-speech app, a synthesiser) and convert with ffmpeg. Example starting from an MP3:

```bash
ffmpeg -i my_recording.mp3 -ar 22050 -ac 1 -acodec pcm_mulaw band_ka.mul
```

Tips for natural-sounding results:
- Record in a quiet room or use noise reduction before converting
- Match the loudness of your clips — inconsistent levels are noticeable when clips are concatenated
- Keep brief pauses at the end of each clip (50–100 ms of silence) to avoid clipping between words

---

##### Uploading a voice pack

1. Open the device web UI → **Audio** page
2. Scroll to **Voice Packs**
3. Enter a pack name (1–16 alphanumeric/underscore characters, e.g. `gemini` or `my_voice`)
4. Click **Choose Files** and select your `.mul` files (you can select all 118 at once, or just the ones you want to override)
5. Click **Upload to Pack** — each file is uploaded individually; progress is shown
6. Once uploaded, click **Use** next to your new pack to activate it

The active pack is saved to device storage and survives reboots. To revert to the built-in voice, click **Use** next to **Default**.

---

##### Troubleshooting voice packs

| Symptom | Likely cause | Fix |
|---|---|---|
| Clip sounds pitched wrong | Wrong sample rate | Re-encode at exactly 22050 Hz |
| Clip sounds distorted | Wrong codec | Use `-acodec pcm_mulaw` not `pcm_s16le` |
| Clip plays but is silent | File is valid but empty | Re-record and re-convert |
| Upload fails | File too large or LittleFS full | Keep clips short; delete unused packs |
| Pack listed but clips still sound like default | Pack not activated | Click **Use** next to the pack name |

---

## Troubleshooting

For comprehensive troubleshooting (connection, display, audio,
performance, factory reset), see [docs/MANUAL.md Section J](docs/MANUAL.md#j-troubleshooting).

**Quick fixes:**
- **V1 won't connect** — disconnect phone apps from V1 first, power cycle both devices
- **Can't find WiFi** — long-press BOOT (~4s) to start AP; WiFi is off by default
- **Upload fails** — try a different USB-C data cable; hold BOOT while connecting

---

## Documentation

| Doc | Role |
|-----|------|
| [docs/MANUAL.md](docs/MANUAL.md) | Architecture, BLE protocol, display, troubleshooting, developer guide, Windows setup |
| [docs/OBSERVABILITY.md](docs/OBSERVABILITY.md) | **Authoritative.** Observability surfaces, metric naming, offline derivation, and test-evidence interpretation |
| [docs/PERF_SLOS.md](docs/PERF_SLOS.md) | Perf thresholds and scoring rules |
| [docs/API.md](docs/API.md) | Full HTTP REST API reference with request/response schemas |

Observability/testing authority lives in `docs/OBSERVABILITY.md`. Each topic has ONE home.

---

## Credits

Originally forked from [ajmdroid/v1g2_simple](https://github.com/ajmdroid/v1g2_simple) — the first public version of this project.

Built on [Kenny Garreau's V1G2-T4S3](https://github.com/kennygarreau/v1g2-t4s3) - go star his repo!

**MIT License** - Use at your own risk. No warranty.
