#!/bin/bash

# Choroboros Plugin Packaging Script
# This script creates a distribution-ready package

set -e

VERSION="1.0.1"
PLUGIN_NAME="Choroboros"
RELEASE_DIR="Release/${PLUGIN_NAME}-v${VERSION}"
ARCHIVE_NAME="${PLUGIN_NAME}-v${VERSION}-macOS.zip"

echo "🎵 Packaging Choroboros v${VERSION}..."

# Clean previous release
if [ -d "Release" ]; then
    echo "Cleaning previous release..."
    rm -rf Release
fi

# Create release directory
mkdir -p "${RELEASE_DIR}"

# Copy build artifacts
echo "Copying build artifacts..."
if [ -d "Build/Choroboros_artefacts/VST3" ]; then
    cp -R Build/Choroboros_artefacts/VST3 "${RELEASE_DIR}/"
    echo "  ✅ VST3 copied"
else
    echo "  ⚠️  VST3 not found - make sure you've built the plugin!"
fi

if [ -d "Build/Choroboros_artefacts/AU" ]; then
    cp -R Build/Choroboros_artefacts/AU "${RELEASE_DIR}/"
    echo "  ✅ AU copied"
else
    echo "  ⚠️  AU not found - make sure you've built the plugin!"
fi

if [ -d "Build/Choroboros_artefacts/Standalone" ]; then
    cp -R Build/Choroboros_artefacts/Standalone "${RELEASE_DIR}/"
    echo "  ✅ Standalone copied"
else
    echo "  ⚠️  Standalone not found - make sure you've built the plugin!"
fi

# Copy documentation if it exists
if [ -f "README.md" ]; then
    cp README.md "${RELEASE_DIR}/"
    echo "  ✅ README copied"
fi

if [ -f "LICENSE" ]; then
    cp LICENSE "${RELEASE_DIR}/"
    echo "  ✅ LICENSE copied"
fi

if [ -f "EULA.md" ]; then
    cp EULA.md "${RELEASE_DIR}/"
    echo "  ✅ EULA.md copied"
fi

if [ -f "DISTRIBUTION.md" ]; then
    cp DISTRIBUTION.md "${RELEASE_DIR}/"
    echo "  ✅ DISTRIBUTION.md copied"
fi

# Create installation instructions
cat > "${RELEASE_DIR}/INSTALL.txt" << 'EOF'
Choroboros Installation Instructions
====================================

VST3 Plugin:
------------
1. Copy Choroboros.vst3 to one of these locations:
   - /Library/Audio/Plug-Ins/VST3/ (system-wide, requires admin)
   - ~/Library/Audio/Plug-Ins/VST3/ (user-specific, recommended)

2. Rescan plugins in your DAW

AU Plugin:
----------
1. Copy Choroboros.component to one of these locations:
   - /Library/Audio/Plug-Ins/Components/ (system-wide, requires admin)
   - ~/Library/Audio/Plug-Ins/Components/ (user-specific, recommended)

2. Rescan plugins in your DAW

Standalone Application:
-----------------------
1. Copy Choroboros.app to /Applications/ or any location you prefer
2. Double-click to launch

Troubleshooting:
----------------
- If plugins don't appear, make sure you've copied them to the correct location
- Some DAWs require a full restart to detect new plugins
- On macOS, you may need to allow the plugins in System Preferences > Security & Privacy
EOF

echo "  ✅ Installation instructions created"

# Create archive
echo "Creating archive..."
cd Release
zip -r "${ARCHIVE_NAME}" "${PLUGIN_NAME}-v${VERSION}" > /dev/null
cd ..

ARCHIVE_SIZE=$(du -h "Release/${ARCHIVE_NAME}" | cut -f1)
ARCHIVE_PATH="Release/${ARCHIVE_NAME}"

# Generate checksums
echo "Generating checksums..."
SHA256_CHECKSUM=$(shasum -a 256 "${ARCHIVE_PATH}" | cut -d' ' -f1)
echo "${SHA256_CHECKSUM}  ${ARCHIVE_NAME}" > "${ARCHIVE_PATH}.sha256"
echo "  ✅ SHA256 checksum: ${SHA256_CHECKSUM}"

echo ""
echo "✅ Package created successfully!"
echo ""
echo "📦 Archive: ${ARCHIVE_PATH}"
echo "📊 Size: ${ARCHIVE_SIZE}"
echo "🔐 SHA256: ${SHA256_CHECKSUM}"
echo ""
echo "Contents:"
ls -lh "${RELEASE_DIR}/"
echo ""
echo "Checksum file: ${ARCHIVE_PATH}.sha256"
echo ""
echo "Ready for distribution! 🚀"
