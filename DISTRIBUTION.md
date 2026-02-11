# Choroboros Distribution Guide

## Installation Instructions

### VST3 Plugin

1. **Locate the plugin file:**
   - Find `Choroboros.vst3` in the distribution package

2. **Choose installation location:**
   - **System-wide (requires admin):** `/Library/Audio/Plug-Ins/VST3/`
   - **User-specific (recommended):** `~/Library/Audio/Plug-Ins/VST3/`

3. **Install:**
   - Copy `Choroboros.vst3` to your chosen location
   - You can drag and drop the file into the folder, or use Terminal:
     ```bash
     # User-specific (recommended)
     cp -R Choroboros.vst3 ~/Library/Audio/Plug-Ins/VST3/
     
     # System-wide (requires sudo)
     sudo cp -R Choroboros.vst3 /Library/Audio/Plug-Ins/VST3/
     ```

4. **Rescan plugins in your DAW:**
   - Most DAWs: Go to Preferences/Settings > Plugins and click "Rescan" or "Refresh"
   - Some DAWs require a full restart to detect new plugins

### AU (Audio Unit) Plugin

1. **Locate the plugin file:**
   - Find `Choroboros.component` in the distribution package

2. **Choose installation location:**
   - **System-wide (requires admin):** `/Library/Audio/Plug-Ins/Components/`
   - **User-specific (recommended):** `~/Library/Audio/Plug-Ins/Components/`

3. **Install:**
   - Copy `Choroboros.component` to your chosen location
   - Terminal command:
     ```bash
     # User-specific (recommended)
     cp -R Choroboros.component ~/Library/Audio/Plug-Ins/Components/
     
     # System-wide (requires sudo)
     sudo cp -R Choroboros.component /Library/Audio/Plug-Ins/Components/
     ```

4. **Rescan plugins in your DAW** (same as VST3 above)

### Standalone Application

1. **Locate the application:**
   - Find `Choroboros.app` in the distribution package

2. **Install:**
   - Drag `Choroboros.app` to `/Applications/` or any location you prefer
   - Double-click to launch

3. **First Launch:**
   - macOS may show a security warning
   - If blocked, go to System Preferences > Security & Privacy
   - Click "Open Anyway" next to the blocked app message

## System Requirements

### Minimum Requirements
- **macOS:** 10.13 (High Sierra) or later
- **CPU:** Intel Core 2 Duo or Apple Silicon
- **RAM:** 4 GB
- **DAW:** Any DAW that supports VST3 or AU plugins
  - Logic Pro, GarageBand, Pro Tools, Ableton Live, Reaper, Studio One, etc.

### Recommended Requirements
- **macOS:** 11.0 (Big Sur) or later
- **CPU:** Intel Core i5/i7 or Apple Silicon (M1/M2/M3)
- **RAM:** 8 GB or more
- **Sample Rate:** Supports up to 192 kHz

### Supported Plugin Formats
- **VST3:** Universal format supported by most DAWs
- **AU (Audio Unit):** Native macOS format, required for Logic Pro
- **Standalone:** Runs independently without a DAW

## Troubleshooting

### Plugin Not Appearing in DAW

**Possible Causes:**
1. Plugin not installed in correct location
2. DAW hasn't rescanned plugins
3. macOS security blocking the plugin
4. Plugin format not supported by DAW

**Solutions:**
1. **Verify installation location:**
   - Check that the plugin is in the correct folder
   - VST3: `~/Library/Audio/Plug-Ins/VST3/` or `/Library/Audio/Plug-Ins/VST3/`
   - AU: `~/Library/Audio/Plug-Ins/Components/` or `/Library/Audio/Plug-Ins/Components/`

2. **Rescan plugins:**
   - Open DAW preferences/settings
   - Find "Plugins" or "VST" section
   - Click "Rescan", "Refresh", or "Rescan All"
   - Wait for scan to complete

3. **Restart DAW:**
   - Some DAWs require a full restart to detect new plugins
   - Quit the DAW completely and relaunch

4. **Check DAW compatibility:**
   - Logic Pro: Requires AU format
   - Most other DAWs: Use VST3 format
   - Check your DAW's documentation for supported formats

### macOS Security Warnings

**If macOS blocks the plugin:**

1. **System Preferences Method:**
   - Open System Preferences > Security & Privacy
   - Click the "General" tab
   - Look for a message about the blocked plugin
   - Click "Open Anyway"

2. **Right-click Method:**
   - Right-click (or Control-click) the plugin file
   - Select "Open" from the context menu
   - Click "Open" in the security dialog

3. **Terminal Method (Advanced):**
   ```bash
   # Remove quarantine attribute
   xattr -d com.apple.quarantine /path/to/Choroboros.vst3
   ```

### Permission Errors

**If you get "Permission Denied" errors:**

1. **Use user-specific location:**
   - Install to `~/Library/` instead of `/Library/`
   - No admin privileges required

2. **Check folder permissions:**
   - Make sure you have write access to the plugin folder
   - User-specific folders should always work

3. **Use Terminal with sudo (system-wide):**
   ```bash
   sudo cp -R Choroboros.vst3 /Library/Audio/Plug-Ins/VST3/
   ```
   - Enter your administrator password when prompted

### Plugin Crashes or Doesn't Load

1. **Check system requirements:**
   - Verify macOS version (10.13 or later)
   - Check CPU compatibility (Intel or Apple Silicon)

2. **Check DAW compatibility:**
   - Ensure your DAW supports the plugin format
   - Try a different format (VST3 vs AU)

3. **Check for conflicts:**
   - Remove any old versions of the plugin
   - Clear DAW plugin cache if available

4. **Contact support:**
   - Email: info@kaizenstrategic.ai
   - Include:
     - macOS version
     - DAW name and version
     - Error messages (if any)
     - Steps to reproduce the issue

## Verification

### Verify Installation

1. **Check plugin location:**
   ```bash
   # VST3
   ls -la ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
   
   # AU
   ls -la ~/Library/Audio/Plug-Ins/Components/Choroboros.component
   ```

2. **Check in DAW:**
   - Open your DAW
   - Look for "Choroboros" in the plugin list
   - Should appear under "Effects" or "Modulation" category

3. **Test the plugin:**
   - Load Choroboros on a track
   - Verify the interface opens correctly
   - Test audio processing

## Uninstallation

### Remove VST3 Plugin
```bash
rm -rf ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
# or for system-wide:
sudo rm -rf /Library/Audio/Plug-Ins/VST3/Choroboros.vst3
```

### Remove AU Plugin
```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/Choroboros.component
# or for system-wide:
sudo rm -rf /Library/Audio/Plug-Ins/Components/Choroboros.component
```

### Remove Standalone App
- Drag `Choroboros.app` from `/Applications/` to Trash
- Or use Terminal:
  ```bash
  rm -rf /Applications/Choroboros.app
  ```

## Support

For additional help or questions:

- **Email:** info@kaizenstrategic.ai
- **Company:** Kaizen Strategic AI Inc. (DBA: Green DSP)
- **Location:** British Columbia, Canada

## License

This software is licensed, not sold. By installing or using Choroboros, you agree to the terms of the End User License Agreement (EULA). The EULA can be viewed from the About dialog within the plugin (click "View License" button).

**Copyright © 2026 Kaizen Strategic AI Inc. All rights reserved.**

---

**Version:** 1.0.1  
**Last Updated:** January 2026
