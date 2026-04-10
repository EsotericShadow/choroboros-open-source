#!/bin/bash
# Build macOS Universal Binary (arm64 + x86_64)

set -e

echo "🎯 Building macOS Universal Binary (arm64 + x86_64)"
echo "=================================================="

# Clean previous builds
echo "🧹 Cleaning previous builds..."
rm -rf Build Universal-Build Release/Choroboros-v2.04-beta-macOS-Universal.zip

# Configure CMake for universal build
echo "⚙️  Configuring CMake for universal build..."
cmake -B Universal-Build \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release

# Build
echo "🔨 Building universal binary..."
cmake --build Universal-Build --config Release --parallel

# Verify architectures
echo "🔍 Verifying architectures..."
# Must match PRODUCT_NAME in CMakeLists.txt (juce_add_plugin)
BUNDLE_BASENAME="Choroboros Beta"
VST3_BINARY="Universal-Build/Choroboros_artefacts/Release/VST3/${BUNDLE_BASENAME}.vst3/Contents/MacOS/${BUNDLE_BASENAME}"
AU_BINARY="Universal-Build/Choroboros_artefacts/Release/AU/${BUNDLE_BASENAME}.component/Contents/MacOS/${BUNDLE_BASENAME}"
STANDALONE_BINARY="Universal-Build/Choroboros_artefacts/Release/Standalone/${BUNDLE_BASENAME}.app/Contents/MacOS/${BUNDLE_BASENAME}"

if [ -f "$VST3_BINARY" ]; then
    echo "✅ VST3 architectures:"
    file "$VST3_BINARY"
    lipo -info "$VST3_BINARY"
fi

if [ -f "$AU_BINARY" ]; then
    echo "✅ AU architectures:"
    file "$AU_BINARY"
    lipo -info "$AU_BINARY"
fi

if [ -f "$STANDALONE_BINARY" ]; then
    echo "✅ Standalone architectures:"
    file "$STANDALONE_BINARY"
    lipo -info "$STANDALONE_BINARY"
fi

# Package
echo "📦 Packaging universal build..."
mkdir -p Release

cd Universal-Build/Choroboros_artefacts/Release
zip -r -o ../../../Release/Choroboros-v2.04-beta-macOS-Universal.zip \
    VST3 AU Standalone

cd ../../..

# Add documentation and installer scripts
cd Release
unzip -q -o Choroboros-v2.04-beta-macOS-Universal.zip || true
cp ../README.md ../DISTRIBUTION.md ../INSTALL.txt ../LICENSE ../COPYING ../SOURCE_LINK.txt . 2>/dev/null || true
cp ../install.sh ../"Install Choroboros.command" . 2>/dev/null || true
chmod +x install.sh "Install Choroboros.command" 2>/dev/null || true
zip -r -o Choroboros-v2.04-beta-macOS-Universal.zip \
    README.md DISTRIBUTION.md INSTALL.txt LICENSE COPYING SOURCE_LINK.txt \
    install.sh "Install Choroboros.command" 2>/dev/null || true
rm -f README.md DISTRIBUTION.md INSTALL.txt LICENSE COPYING SOURCE_LINK.txt install.sh "Install Choroboros.command"
cd ..

# Generate checksum
echo "🔐 Generating SHA256 checksum..."
shasum -a 256 Release/Choroboros-v2.04-beta-macOS-Universal.zip > Release/Choroboros-v2.04-beta-macOS-Universal.zip.sha256

echo ""
echo "✅ macOS Universal build complete!"
echo "📦 Package: Release/Choroboros-v2.04-beta-macOS-Universal.zip"
echo "📊 Size: $(du -h Release/Choroboros-v2.04-beta-macOS-Universal.zip | cut -f1)"
