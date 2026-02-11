# DAW Rescan Instructions
## After Reinstalling Choroboros Plugin

The plugin has been reinstalled with the correct copyright (2026). Now you need to rescan plugins in your DAW to see the updated information.

---

## Reaper

1. **Quit Reaper completely** (if it's running)

2. **Open Reaper**

3. **Clear cache and rescan:**
   - Go to: **Preferences** (⌘,)
   - Navigate to: **Plug-ins** > **VST**
   - Click: **"Clear cache and rescan directory"**
   - Wait for the scan to complete (may take 1-2 minutes)

4. **Verify:**
   - Open a track
   - Add Choroboros plugin
   - Click "About" or "Info" button
   - Should show: **(C) 2026 Kaizen Strategic AI Inc**

---

## Ableton Live

1. **Quit Ableton Live completely** (if it's running)

2. **Open Ableton Live**

3. **Rescan plugins:**
   - Go to: **Preferences** (⌘,)
   - Navigate to: **Plug-Ins** tab
   - Click: **"Rescan"** or **"Rescan Plug-Ins"**
   - Wait for the scan to complete

4. **Verify:**
   - Open a track
   - Add Choroboros plugin
   - Check plugin info or About dialog
   - Should show: **(C) 2026 Kaizen Strategic AI Inc**

---

## GarageBand

1. **Quit GarageBand completely** (if it's running)

2. **Open GarageBand**

3. **GarageBand auto-rescans on startup:**
   - The plugin should be automatically detected
   - If not visible, quit and restart GarageBand

4. **Verify:**
   - Create a new track
   - Add Choroboros plugin
   - Check plugin information
   - Should show: **(C) 2026 Kaizen Strategic AI Inc**

---

## If Copyright Still Shows 2024

If you're still seeing 2024 after rescanning:

### Option 1: Full DAW Restart
1. Quit DAW completely
2. Wait 10 seconds
3. Restart DAW
4. Rescan plugins again

### Option 2: Clear DAW-Specific Cache

**Reaper:**
```bash
rm -f ~/Library/Application\ Support/REAPER/reaper-vstplugins64.ini
rm -f ~/Library/Application\ Support/REAPER/reaper-auplugins-bc.txt
```

**Ableton:**
```bash
find ~/Library/Caches/Ableton -name "*plugin*cache*" -delete
find ~/Library/Application\ Support/Ableton -name "PluginCache.cfg" -delete
```

**GarageBand:**
```bash
rm -rf ~/Library/Caches/com.apple.garageband10
```

Then restart the DAW and rescan.

### Option 3: System Restart
Sometimes a full system restart is needed to clear all caches:
1. Save all work
2. Restart your Mac
3. Open DAW
4. Rescan plugins

---

## Verification Checklist

After rescanning, verify:

- [ ] Plugin appears in DAW's plugin list
- [ ] Plugin loads without errors
- [ ] About dialog shows: **(C) 2026 Kaizen Strategic AI Inc**
- [ ] No "Ã" character (encoding issue fixed)
- [ ] All 4 engine colors work
- [ ] All parameters function correctly

---

## Still Having Issues?

If copyright still shows incorrectly:

1. **Check plugin file directly:**
   ```bash
   # Check VST3
   grep "NSHumanReadableCopyright" ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3/Contents/Info.plist
   
   # Check AU
   grep "NSHumanReadableCopyright" ~/Library/Audio/Plug-Ins/Components/Choroboros.component/Contents/Info.plist
   ```
   
   Should show: `Copyright (C) 2026 Kaizen Strategic AI Inc. All rights reserved.`

2. **Re-run reinstall script:**
   ```bash
   cd /Users/main/Desktop/green_chorus
   ./reinstall_plugin.sh
   ```

3. **Contact support:**
   - Email: info@kaizenstrategic.ai
   - Include: DAW name/version, macOS version, error messages

---

**The plugin is now correctly installed with copyright 2026. Just rescan in your DAW!**
