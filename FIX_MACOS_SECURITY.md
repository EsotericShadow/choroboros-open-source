# Fix macOS Security Warning
## "Apple could not verify Choroboros.vst3 is free of malware"

This warning appears because the plugin isn't code-signed. Here are several ways to fix it:

---

## Quick Fix: Remove Quarantine Attribute

Run this command in Terminal:

```bash
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Choroboros.component
xattr -d com.apple.quarantine ~/Applications/Choroboros.app
```

Then try opening the plugin again in Reaper.

---

## Method 1: Allow in System Preferences (Recommended for Users)

1. **When you see the warning:**
   - Click "Cancel" (don't move to Trash)

2. **Open System Preferences:**
   - Go to **System Preferences** > **Security & Privacy** (or **System Settings** > **Privacy & Security** on newer macOS)

3. **Allow the plugin:**
   - Look for a message about "Choroboros.vst3" being blocked
   - Click **"Open Anyway"** button next to the message
   - You may need to enter your password

4. **Try again:**
   - Go back to Reaper
   - Load the plugin again
   - It should work now

---

## Method 2: Right-Click to Open

1. **Quit Reaper**

2. **Right-click (or Control-click) the plugin file:**
   - Navigate to: `~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3`
   - Right-click the file
   - Select **"Open"** from the context menu

3. **Click "Open" in the security dialog:**
   - macOS will show a security warning
   - Click **"Open"** button
   - This adds it to your allowed list

4. **Open Reaper:**
   - The plugin should now load without warnings

---

## Method 3: Terminal Command (Already Run)

The quarantine attribute has been removed. If you still see the warning:

1. **Quit Reaper completely**

2. **Run the command again:**
   ```bash
   xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
   ```

3. **Restart Reaper**

---

## Method 4: Code Signing (For Distribution)

For selling the plugin, you should code-sign it. This requires:

1. **Apple Developer Account** ($99/year)
2. **Code signing certificate**
3. **Notarization** (for distribution)

### Quick Code Signing (if you have Developer account):

```bash
# Sign the plugin
codesign --force --deep --sign "Developer ID Application: Kaizen Strategic AI Inc" ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3

# Verify signature
codesign --verify --verbose ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
```

**Note:** For immediate sale, you can distribute without code signing, but customers will need to allow it in System Preferences. For professional distribution, code signing is recommended.

---

## For Your Customers

When customers download and install, they'll see the same warning. Include these instructions in your distribution:

### Customer Instructions:

**If you see "Apple could not verify" warning:**

1. **Click "Cancel"** (don't move to Trash)

2. **Open System Preferences:**
   - Go to **Security & Privacy** (or **Privacy & Security**)

3. **Click "Open Anyway"** next to the blocked plugin message

4. **Or use Terminal:**
   ```bash
   xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
   ```

5. **Restart your DAW**

---

## Update Reinstall Script

The `reinstall_plugin.sh` script has been updated to automatically remove quarantine attributes.

---

## Long-Term Solution: Code Signing

For professional distribution, consider:

1. **Apple Developer Account:** https://developer.apple.com
2. **Code Signing:** Sign all plugin formats
3. **Notarization:** Submit to Apple for notarization (removes warnings completely)

**Cost:** $99/year for Apple Developer account

**Time:** Code signing takes 5-10 minutes, notarization takes 1-2 hours

**Benefit:** Customers won't see any security warnings

---

## Current Status

✅ Quarantine attributes removed from installed plugins
✅ Plugin should work in Reaper now
⚠️ Customers will need to allow it in System Preferences (or you can code-sign later)

---

**Try opening Reaper again - the plugin should work now!**
