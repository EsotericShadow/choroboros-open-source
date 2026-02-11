#!/bin/bash

# Choroboros DMG Creator Script
# Creates a professional DMG installer for distribution

set -e

VERSION="1.0.1"
PLUGIN_NAME="Choroboros"
DMG_NAME="${PLUGIN_NAME}-v${VERSION}-macOS"
TEMP_DIR="dmg_temp"
DMG_DIR="${TEMP_DIR}/${PLUGIN_NAME}"

echo "📦 Creating DMG installer for ${PLUGIN_NAME} v${VERSION}..."

# Clean up previous attempts
if [ -d "${TEMP_DIR}" ]; then
    rm -rf "${TEMP_DIR}"
fi

if [ -f "${DMG_NAME}.dmg" ]; then
    echo "Removing old DMG..."
    rm -f "${DMG_NAME}.dmg"
fi

# Create temp directory structure
mkdir -p "${DMG_DIR}"

# Copy plugins
echo "Copying plugins..."
cp -R Build/Choroboros_artefacts/VST3 "${DMG_DIR}/"
cp -R Build/Choroboros_artefacts/AU "${DMG_DIR}/"
cp -R Build/Choroboros_artefacts/Standalone "${DMG_DIR}/"

# Create installation instructions
cat > "${DMG_DIR}/INSTALL.txt" << 'EOF'
Choroboros Installation Instructions
====================================

VST3 Plugin:
------------
1. Drag Choroboros.vst3 to:
   - /Library/Audio/Plug-Ins/VST3/ (system-wide, requires password)
   - ~/Library/Audio/Plug-Ins/VST3/ (user-specific, recommended)

2. Rescan plugins in your DAW

AU Plugin:
----------
1. Drag Choroboros.component to:
   - /Library/Audio/Plug-Ins/Components/ (system-wide, requires password)
   - ~/Library/Audio/Plug-Ins/Components/ (user-specific, recommended)

2. Rescan plugins in your DAW

Standalone Application:
-----------------------
1. Drag Choroboros.app to /Applications/ or any location you prefer
2. Double-click to launch

Quick Install (Recommended):
----------------------------
For user-specific installation (no password required):
- VST3: ~/Library/Audio/Plug-Ins/VST3/
- AU: ~/Library/Audio/Plug-Ins/Components/
- App: /Applications/

Troubleshooting:
----------------
- If plugins don't appear, make sure you've copied them to the correct location
- Some DAWs require a full restart to detect new plugins
- On macOS, you may need to allow the plugins in System Preferences > Security & Privacy
- After installation, rescan plugins in your DAW's preferences

Version: 1.0.1
EOF

# Copy README if it exists
if [ -f "README.md" ]; then
    cp README.md "${DMG_DIR}/"
fi

# Create Applications and Library shortcuts for easy dragging
echo "Creating installation shortcuts..."
ln -s /Applications "${DMG_DIR}/Applications"
mkdir -p "${DMG_DIR}/Library"
ln -s ~/Library/Audio/Plug-Ins/VST3 "${DMG_DIR}/Library/VST3 (User)"
ln -s ~/Library/Audio/Plug-Ins/Components "${DMG_DIR}/Library/Components (User)"

# Create a background image info (optional - you can add a .png background later)
# For now, we'll use a simple layout

# Create DMG
echo "Creating DMG..."
hdiutil create -volname "${PLUGIN_NAME}" \
    -srcfolder "${DMG_DIR}" \
    -ov -format UDZO \
    "${DMG_NAME}.dmg"

# Clean up
rm -rf "${TEMP_DIR}"

DMG_SIZE=$(du -h "${DMG_NAME}.dmg" | cut -f1)
echo ""
echo "✅ DMG created successfully!"
echo ""
echo "📦 DMG: ${DMG_NAME}.dmg"
echo "📊 Size: ${DMG_SIZE}"
echo ""
echo "The DMG contains:"
echo "  - Choroboros.vst3 (VST3 plugin)"
echo "  - Choroboros.component (AU plugin)"
echo "  - Choroboros.app (Standalone application)"
echo "  - INSTALL.txt (Installation instructions)"
echo "  - Shortcuts to installation folders"
echo ""
echo "Ready for distribution! 🚀"
