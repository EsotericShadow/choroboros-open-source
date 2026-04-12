# Cursor: Clean Uninstall + Reinstall Choroboros Beta (macOS)

## Goal

Remove every trace of Choroboros Beta from macOS plugin directories and all DAW caches, then reinstall the freshly built v2.05 binaries. **No DAW should trigger a full plugin library rescan** — only the Choroboros entry should be refreshed.

The trick: we delete old bundles, copy new bundles to the **exact same paths with the exact same bundle names**. Every DAW below detects a changed (or re-appeared) bundle on next launch and rescans only that file. A full rescan only happens when you change scan paths or explicitly hit "Rescan All" in preferences.

---

## Step 0 — Quit every DAW

Before touching any files, **quit all of these** (force-quit if needed):

```bash
# Quit DAWs gracefully first, then force-kill any stragglers
osascript -e 'quit app "Cubase"' 2>/dev/null
osascript -e 'quit app "FL Studio"' 2>/dev/null
osascript -e 'quit app "REAPER"' 2>/dev/null
osascript -e 'quit app "Ableton Live 12 Suite"' 2>/dev/null
osascript -e 'quit app "Waveform Pro 13"' 2>/dev/null
osascript -e 'quit app "GarageBand"' 2>/dev/null
sleep 3
```

Verify nothing is running:

```bash
ps aux | grep -iE "cubase|flstudio|fl studio|reaper|ableton|live|waveform|garageband" | grep -v grep
```

If anything shows up, `kill -9` the PID.

---

## Step 1 — Nuke old Choroboros bundles

The build's `PRODUCT_NAME` is `"Choroboros Beta"`, but older installs or the `install.sh` script used `"Choroboros"` (no "Beta"). Clean **both** naming variants.

```bash
echo "=== Removing Choroboros from plugin directories ==="

# ---- VST3 ----
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Choroboros Beta.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
rm -rf "/Library/Audio/Plug-Ins/VST3/Choroboros Beta.vst3"
rm -rf "/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"

# ---- AU ----
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Choroboros Beta.component"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Choroboros.component"
rm -rf "/Library/Audio/Plug-Ins/Components/Choroboros Beta.component"
rm -rf "/Library/Audio/Plug-Ins/Components/Choroboros.component"

# ---- AAX (Pro Tools developer path) ----
rm -rf "$HOME/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros Beta.aaxplugin"
rm -rf "$HOME/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros.aaxplugin"
rm -rf "/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros Beta.aaxplugin"
rm -rf "/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros.aaxplugin"

# ---- Standalone ----
rm -rf "/Applications/Choroboros Beta.app"
rm -rf "/Applications/Choroboros.app"

echo "Done — all Choroboros bundles removed."
```

Verify nothing remains:

```bash
find ~/Library/Audio /Library/Audio "$HOME/Library/Application Support/Avid" /Applications \
     -iname "*choroboros*" 2>/dev/null
```

Should print nothing.

---

## Step 2 — Clear DAW-specific caches for Choroboros ONLY

These commands surgically remove Choroboros entries from each DAW's plugin cache without wiping the full cache. This prevents the "plugin not found" ghost entry and avoids a full rescan.

### Cubase (Steinberg)

Cubase stores its plugin database in a binary cache. The safest way to force it to re-read just Choroboros without a full rescan is to **not touch the cache at all** — Cubase detects missing/changed VST3 bundles on launch and rescans only those. Since we deleted the old bundle and will place the new one at the same path, Cubase handles this automatically.

If Choroboros still shows as "missing" or blacklisted after reinstall:

```bash
# Only if needed — this deletes blacklist entries, not the full cache
CUBASE_PREFS="$HOME/Library/Preferences/Steinberg"
if [ -d "$CUBASE_PREFS" ]; then
    find "$CUBASE_PREFS" -name "*.xml" -exec grep -l -i "choroboros" {} \; 2>/dev/null | while read f; do
        echo "Cubase cache file mentions Choroboros: $f"
        echo "  (Open this file and delete the Choroboros entry if the plugin shows as blacklisted)"
    done
fi
```

### FL Studio (Image-Line)

FL Studio on macOS rescans standard VST3/AU paths on launch. No manual cache clearing needed — it detects the new bundle automatically. If it doesn't:

```bash
# FL's VST scan database (macOS)
FL_DB="$HOME/Library/Application Support/Image-Line/FL Studio"
if [ -d "$FL_DB" ]; then
    find "$FL_DB" -iname "*plugin*" -o -iname "*scan*" -o -iname "*cache*" 2>/dev/null | head -20
    echo "(If Choroboros doesn't appear, delete FL's plugin database file above and re-open FL)"
fi
```

### REAPER

REAPER keeps a plugin cache at `reaper-vstplugins64.ini` (VST3) and `reaper-auplugins64.ini` (AU). We can surgically remove just the Choroboros lines:

```bash
REAPER_DIR="$HOME/Library/Application Support/REAPER"
for CACHE_FILE in "reaper-vstplugins64.ini" "reaper-auplugins64.ini" "reaper-vst3plugins64.ini"; do
    FULL_PATH="$REAPER_DIR/$CACHE_FILE"
    if [ -f "$FULL_PATH" ]; then
        if grep -qi "choroboros" "$FULL_PATH"; then
            echo "Removing Choroboros entries from $CACHE_FILE"
            grep -vi "choroboros" "$FULL_PATH" > "${FULL_PATH}.tmp"
            mv "${FULL_PATH}.tmp" "$FULL_PATH"
        fi
    fi
done
```

On next launch, REAPER detects the missing cache entry + new bundle and rescans just that plugin.

### Ableton Live

Ableton's plugin database is binary. Like Cubase, it detects new/changed bundles on launch without a full rescan. No manual cache work needed.

If Choroboros doesn't appear after reinstall:

```bash
# Ableton's AU/VST3 cache (macOS)
ABLETON_PREFS="$HOME/Library/Preferences/Ableton"
if [ -d "$ABLETON_PREFS" ]; then
    find "$ABLETON_PREFS" -iname "*plugin*" -o -iname "*cache*" -o -iname "*db*" 2>/dev/null | head -10
    echo "(Delete the plugin database file above to force Ableton to rescan — it only rescans changed plugins)"
fi
```

### Waveform 13 (Tracktion)

```bash
TRACKTION_DIR="$HOME/Library/Application Support/Tracktion"
if [ -d "$TRACKTION_DIR" ]; then
    find "$TRACKTION_DIR" -iname "*plugin*" -o -iname "*scan*" 2>/dev/null | while read f; do
        if grep -qi "choroboros" "$f" 2>/dev/null; then
            echo "Removing Choroboros entries from: $f"
            grep -vi "choroboros" "$f" > "${f}.tmp" && mv "${f}.tmp" "$f"
        fi
    done
fi
```

### GarageBand (AU only)

GarageBand uses the system AU cache managed by `AudioComponentRegistrar`. Force it to re-read:

```bash
killall -9 AudioComponentRegistrar 2>/dev/null
echo "AudioComponentRegistrar killed — macOS will rebuild AU cache on next DAW launch."
```

This is also needed for **Logic Pro** and any AU host. Run it once, it covers all of them.

---

## Step 3 — Install fresh v2.05 binaries

```bash
cd /Users/main/Desktop/CHOROS_MASTER/choroboros-open-source

# Source paths — adjust if your build dir is different
BUILD_DIR="build/Choroboros_artefacts/Release"
VST3_SRC="${BUILD_DIR}/VST3/Choroboros Beta.vst3"
AU_SRC="${BUILD_DIR}/AU/Choroboros Beta.component"
AAX_SRC="${BUILD_DIR}/AAX/Choroboros Beta.aaxplugin"
STANDALONE_SRC="${BUILD_DIR}/Standalone/Choroboros Beta.app"

echo "=== Installing Choroboros Beta v2.05 ==="

# ---- VST3 ----
if [ -d "$VST3_SRC" ]; then
    cp -R "$VST3_SRC" "$HOME/Library/Audio/Plug-Ins/VST3/"
    echo "  VST3 installed to ~/Library/Audio/Plug-Ins/VST3/"
fi

# ---- AU ----
if [ -d "$AU_SRC" ]; then
    cp -R "$AU_SRC" "$HOME/Library/Audio/Plug-Ins/Components/"
    echo "  AU installed to ~/Library/Audio/Plug-Ins/Components/"
fi

# ---- AAX ----
if [ -d "$AAX_SRC" ]; then
    mkdir -p "$HOME/Library/Application Support/Avid/Audio/Plug-Ins"
    cp -R "$AAX_SRC" "$HOME/Library/Application Support/Avid/Audio/Plug-Ins/"
    echo "  AAX installed to ~/Library/Application Support/Avid/Audio/Plug-Ins/"
fi

# ---- Standalone ----
if [ -d "$STANDALONE_SRC" ]; then
    cp -R "$STANDALONE_SRC" "/Applications/"
    echo "  Standalone installed to /Applications/"
fi

echo ""
echo "=== Clearing quarantine flags ==="
xattr -cr "$HOME/Library/Audio/Plug-Ins/VST3/Choroboros Beta.vst3" 2>/dev/null
xattr -cr "$HOME/Library/Audio/Plug-Ins/Components/Choroboros Beta.component" 2>/dev/null
xattr -cr "$HOME/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros Beta.aaxplugin" 2>/dev/null
xattr -cr "/Applications/Choroboros Beta.app" 2>/dev/null
echo "  Quarantine cleared."

echo ""
echo "=== Refreshing AU registration ==="
killall -9 AudioComponentRegistrar 2>/dev/null
echo "  AudioComponentRegistrar restarted."
```

---

## Step 4 — Verify installation

```bash
echo "=== Installed Choroboros bundles ==="
ls -la "$HOME/Library/Audio/Plug-Ins/VST3/" | grep -i choroboros
ls -la "$HOME/Library/Audio/Plug-Ins/Components/" | grep -i choroboros
ls -la "$HOME/Library/Application Support/Avid/Audio/Plug-Ins/" 2>/dev/null | grep -i choroboros
ls -la "/Applications/" | grep -i choroboros
echo ""

echo "=== AU validation (quick) ==="
auval -a 2>/dev/null | grep -i choroboros || echo "  (auval not found or Choroboros AU not registered yet — open a DAW first)"
```

---

## Step 5 — Launch each DAW and verify

Open each DAW one at a time. For each one:

1. Let it start fully (it will detect the new bundle and rescan just Choroboros)
2. Open a new project
3. Insert Choroboros Beta on a track
4. Confirm the plugin loads and the GUI opens
5. Check the DAW's latency readout shows **221 samples** (44.1 kHz) or **240 samples** (48 kHz) — this is the safety limiter lookahead

### DAW-specific notes

| DAW | Plugin format to test | Where to check latency |
|---|---|---|
| Cubase | VST3 | Plugin info bar at bottom of plugin window |
| FL Studio | VST3 | Wrapper Settings → Processing → Plugin reported latency |
| REAPER | VST3 + AU | FX chain header shows latency in samples |
| Ableton Live | AU (preferred) or VST3 | Track delay shown in mixer |
| Waveform 13 | VST3 | Plugin properties / latency column in mixer |
| GarageBand | AU only | Not directly shown — but PDC should auto-compensate |

### Smoke test (do this in at least one DAW)

1. Load the **Psychedelic** preset
2. Play a loud drum loop through it (peaking around -3 dBFS input)
3. Watch the output meter — it should **never exceed -0.7 dBFS**
4. If you see output hitting 0 dBFS or the DAW's clip indicator lights up, something is wrong — stop and report back

---

## What NOT to do

- **Do NOT click "Rescan All Plugins"** in any DAW preferences. The bundles are in standard locations with the same names — DAWs detect them automatically.
- **Do NOT delete DAW preference folders wholesale.** That forces a full rescan of everything.
- **Do NOT run `sudo` for the plugin copies** unless you're installing to `/Library/Audio/` (system-wide). User-level `~/Library/Audio/` doesn't need it.
