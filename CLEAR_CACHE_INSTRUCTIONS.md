# Clear Plugin Cache Instructions
## Fix Copyright Display in DAWs

If you're still seeing "2024" or incorrect copyright information in your DAW, the DAW has cached the old plugin information. Follow these steps to clear the cache and reload the updated plugin.

---

## Quick Fix: Run the Script

```bash
cd /Users/main/Desktop/green_chorus
./clear_plugin_cache.sh
```

Then follow the steps below.

---

## Manual Steps for Each DAW

### 1. Reaper

**Step 1: Quit Reaper completely**

**Step 2: Clear Reaper's plugin cache**
```bash
# Delete VST3 cache
rm -f ~/Library/Application\ Support/REAPER/reaper-vstplugins64.ini

# Delete AU cache
rm -f ~/Library/Application\ Support/REAPER/reaper-auplugins-bc.txt

# Delete plugin blacklist (if exists)
rm -f ~/Library/Application\ Support/REAPER/reaper-vstplugins64-blacklist.ini
```

**Step 3: Remove old plugin installation**
```bash
# Remove old VST3
rm -rf ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3

# Remove old AU
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component
```

**Step 4: Install new plugin**
- Extract `Release/Choroboros-v1.0.1-macOS.zip`
- Copy `Choroboros.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
- Copy `Choroboros.component` to `~/Library/Audio/Plug-Ins/Components/`

**Step 5: Restart Reaper and rescan**
- Open Reaper
- Go to Preferences > Plug-ins > VST
- Click "Clear cache and rescan directory"
- Wait for scan to complete

---

### 2. Ableton Live

**Step 1: Quit Ableton Live completely**

**Step 2: Clear Ableton's plugin cache**
```bash
# Clear plugin cache files
find ~/Library/Caches/Ableton -name "*plugin*cache*" -delete 2>/dev/null
find ~/Library/Caches/Ableton -name "*vst*cache*" -delete 2>/dev/null
find ~/Library/Caches/Ableton -name "*au*cache*" -delete 2>/dev/null

# Clear PluginCache.cfg (if exists)
find ~/Library/Application\ Support/Ableton -name "PluginCache.cfg" -delete 2>/dev/null
```

**Step 3: Remove old plugin installation**
```bash
rm -rf ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component
```

**Step 4: Install new plugin**
- Extract `Release/Choroboros-v1.0.1-macOS.zip`
- Copy plugins to appropriate locations

**Step 5: Restart Ableton and rescan**
- Open Ableton Live
- Go to Preferences > Plug-Ins
- Click "Rescan" or "Rescan Plug-Ins"
- Wait for scan to complete

---

### 3. GarageBand

**Step 1: Quit GarageBand completely**

**Step 2: Clear GarageBand cache**
```bash
# Clear GarageBand cache
rm -rf ~/Library/Caches/com.apple.garageband10

# Clear GarageBand preferences (optional - will reset all GB settings)
# rm ~/Library/Preferences/com.apple.garageband10.plist
```

**Step 3: Clear macOS Audio Unit cache (affects GarageBand)**
```bash
rm -f ~/Library/Caches/AudioUnitCache
rm -f ~/Library/Caches/com.apple.audiounits.cache
```

**Step 4: Remove old plugin installation**
```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component
```

**Step 5: Install new plugin**
- Extract `Release/Choroboros-v1.0.1-macOS.zip`
- Copy `Choroboros.component` to `~/Library/Audio/Plug-Ins/Components/`

**Step 6: Restart GarageBand**
- Open GarageBand
- The plugin should be automatically rescanned
- If not, quit and restart GarageBand again

---

## System-Wide Cache (All DAWs)

**Clear macOS Audio Unit cache:**
```bash
rm -f ~/Library/Caches/AudioUnitCache
rm -f ~/Library/Caches/com.apple.audiounits.cache
```

**Clear system VST3 cache (requires admin):**
```bash
sudo rm -rf /Library/Caches/VST3
```

---

## Complete Reset (Nuclear Option)

If the above doesn't work, try this complete reset:

```bash
# 1. Quit all DAWs

# 2. Remove all plugin installations
rm -rf ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component
rm -rf /Library/Audio/Plug-Ins/VST3/Choroboros.vst3
rm -rf /Library/Audio/Plug-Ins/Components/Choroboros.component

# 3. Clear all caches
./clear_plugin_cache.sh

# 4. Clear macOS Audio Unit cache
rm -f ~/Library/Caches/AudioUnitCache
rm -f ~/Library/Caches/com.apple.audiounits.cache

# 5. Reinstall from new package
# Extract Release/Choroboros-v1.0.1-macOS.zip
# Copy plugins to ~/Library/Audio/Plug-Ins/

# 6. Restart computer (optional but recommended)

# 7. Open DAW and rescan plugins
```

---

## Verify the Fix

After clearing cache and reinstalling:

1. **Check About Dialog:**
   - Open plugin in your DAW
   - Click "About" or "Info" button
   - Should show: "(C) 2026 Kaizen Strategic AI Inc"

2. **Check Plugin Info in DAW:**
   - Reaper: Right-click plugin in browser > Properties
   - Ableton: Hover over plugin in browser
   - GarageBand: Plugin info should show in inspector

3. **Check Info.plist (if accessible):**
   - Right-click plugin bundle > Show Package Contents
   - Open Contents/Info.plist
   - Should show: "Copyright (C) 2026 Kaizen Strategic AI Inc. All rights reserved."

---

## Troubleshooting

**Still seeing 2024?**
- Make sure you removed the OLD plugin installation before installing new one
- Try restarting your computer
- Check that you're using the NEW package (v1.0.1)

**Plugin not appearing after cache clear?**
- Make sure plugin is installed in correct location
- Rescan plugins in DAW
- Check DAW's plugin search paths in preferences

**Getting errors?**
- Make sure all DAWs are completely quit
- Check file permissions
- Try installing to user location (~/Library/) instead of system (/Library/)

---

## Quick Command Reference

```bash
# Run cache clear script
./clear_plugin_cache.sh

# Remove old installations
rm -rf ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component

# Clear macOS Audio Unit cache
rm -f ~/Library/Caches/AudioUnitCache
rm -f ~/Library/Caches/com.apple.audiounits.cache

# Install new plugin (after extracting package)
cp -R Release/Choroboros-v1.0.1/VST3/Choroboros.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R Release/Choroboros-v1.0.1/AU/Choroboros.component ~/Library/Audio/Plug-Ins/Components/
```

---

**After clearing cache and reinstalling, the copyright should show correctly as 2026!**
