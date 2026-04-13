#!/bin/bash

# Choroboros Beta Plugin Packaging Script
# This script creates a distribution-ready package

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# shellcheck source=../installer/installer_config.sh
source "${REPO_ROOT}/installer/installer_config.sh"

VERSION="${CHOROBOROS_PUBLIC_VERSION}"
PRODUCT_NAME="${CHOROBOROS_BUNDLE_BASENAME}"
PRODUCT_SLUG="${CHOROBOROS_PRODUCT_SLUG}"
RELEASE_DIR="Release/${PRODUCT_SLUG}-${VERSION}"
ARCHIVE_NAME="${PRODUCT_SLUG}-${VERSION}-macOS.zip"

echo "🎵 Packaging ${PRODUCT_NAME} ${VERSION}..."

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

if [ -f "COPYING" ]; then
    cp COPYING "${RELEASE_DIR}/"
    echo "  ✅ COPYING copied"
fi

if [ -f "SOURCE_LINK.txt" ]; then
    cp SOURCE_LINK.txt "${RELEASE_DIR}/"
    echo "  ✅ SOURCE_LINK.txt copied"
fi

if [ -f "DISTRIBUTION.md" ]; then
    cp DISTRIBUTION.md "${RELEASE_DIR}/"
    echo "  ✅ DISTRIBUTION.md copied"
fi

# Create installation instructions
cat > "${RELEASE_DIR}/INSTALL.txt" <<EOF
${PRODUCT_NAME} Installation Instructions
====================================

VST3 Plugin:
------------
1. Copy ${PRODUCT_NAME}.vst3 to one of these locations:
   - /Library/Audio/Plug-Ins/VST3/ (system-wide, requires admin)
   - ~/Library/Audio/Plug-Ins/VST3/ (user-specific, recommended)

2. Rescan plugins in your DAW

AU Plugin:
----------
1. Copy ${PRODUCT_NAME}.component to one of these locations:
   - /Library/Audio/Plug-Ins/Components/ (system-wide, requires admin)
   - ~/Library/Audio/Plug-Ins/Components/ (user-specific, recommended)

2. Rescan plugins in your DAW

Standalone Application:
-----------------------
1. Copy ${PRODUCT_NAME}.app to /Applications/ or any location you prefer
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
zip -r "${ARCHIVE_NAME}" "${PRODUCT_SLUG}-${VERSION}" > /dev/null
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
