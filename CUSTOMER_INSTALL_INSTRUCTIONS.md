# Choroboros Installation Instructions for Customers
## How to Install and Allow the Plugin

---

## Installation Steps

### Step 1: Download
Download `Choroboros-v1.0.1-macOS.zip` from your purchase.

### Step 2: Extract
Double-click the ZIP file to extract it.

### Step 3: Install Plugins

**VST3 Plugin:**
1. Open the extracted folder
2. Navigate to `VST3/Choroboros.vst3`
3. Copy it to: `~/Library/Audio/Plug-Ins/VST3/`
   - Open Finder
   - Press ⌘+Shift+G (Go to Folder)
   - Type: `~/Library/Audio/Plug-Ins/VST3/`
   - Drag `Choroboros.vst3` into this folder

**AU Plugin:**
1. From the extracted folder, navigate to `AU/Choroboros.component`
2. Copy it to: `~/Library/Audio/Plug-Ins/Components/`
   - Press ⌘+Shift+G in Finder
   - Type: `~/Library/Audio/Plug-Ins/Components/`
   - Drag `Choroboros.component` into this folder

**Standalone (Optional):**
1. From the extracted folder, navigate to `Standalone/Choroboros.app`
2. Drag it to `/Applications/` or any location you prefer

### Step 4: Remove Quarantine Attribute (Important!)

macOS may block the plugin with a security warning. Fix this by running in Terminal:

```bash
# Remove quarantine from VST3
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3

# Remove quarantine from AU
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Choroboros.component

# Remove quarantine from Standalone (if installed)
xattr -d com.apple.quarantine ~/Applications/Choroboros.app
```

**Or use System Preferences method (see below).**

---

## If You See "Apple could not verify" Warning

### Method 1: System Preferences (Easiest)

1. **When you see the warning, click "Cancel"** (don't move to Trash)

2. **Open System Preferences:**
   - Go to **System Preferences** > **Security & Privacy**
   - (On macOS Ventura+: **System Settings** > **Privacy & Security**)

3. **Allow the plugin:**
   - Look for a message about "Choroboros.vst3" or "Choroboros.component" being blocked
   - Click **"Open Anyway"** button next to the message
   - Enter your password if prompted

4. **Try again:**
   - Go back to your DAW
   - Load the plugin again
   - It should work now

### Method 2: Right-Click to Open

1. **Quit your DAW**

2. **Right-click the plugin file:**
   - Navigate to: `~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3`
   - Right-click (or Control-click) the file
   - Select **"Open"** from the context menu

3. **Click "Open" in the security dialog**

4. **Open your DAW** - the plugin should now load

### Method 3: Terminal Command

Open Terminal and run:

```bash
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Choroboros.component
```

Then restart your DAW.

---

## Rescan Plugins in Your DAW

After installation, rescan plugins:

**Reaper:**
- Preferences > Plug-ins > VST
- Click "Clear cache and rescan directory"

**Ableton Live:**
- Preferences > Plug-Ins
- Click "Rescan"

**Logic Pro / GarageBand:**
- Restart the application (auto-rescans)

**Other DAWs:**
- Check Preferences/Settings for plugin rescan option

---

## Troubleshooting

**Plugin not appearing?**
- Make sure it's in the correct folder
- Rescan plugins in your DAW
- Some DAWs require a full restart

**Still getting security warning?**
- Make sure you removed quarantine attribute (see Step 4 above)
- Or use System Preferences method
- Try restarting your Mac

**Plugin crashes or doesn't load?**
- Check system requirements (macOS 10.13+)
- Make sure you have the correct format for your DAW (VST3 or AU)
- Contact support: info@kaizenstrategic.ai

---

## Support

For installation help or technical issues:
- **Email:** info@kaizenstrategic.ai
- **Company:** Kaizen Strategic AI Inc. (DBA: Green DSP)

---

**Enjoy Choroboros! 🎵**
