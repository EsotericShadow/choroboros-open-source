#!/bin/bash
# Build macOS Universal Binary (arm64 + x86_64)
#
# Environment (optional):
#   CHOROBOROS_CMAKE_BUILD_DIR   cmake -B directory (default: Universal-Build under repo).
#                                Set to an absolute path on an external disk, e.g.
#                                /Volumes/T7/ChoroborosCMakeBuilds/universal
#   CHOROBOROS_SKIP_UNIVERSAL_CLEAN   if set to 1, do not rm -rf the CMake build dir first
#
# Full signed installer pipeline:
#   ./scripts/release_macos_signed_installer.sh
# Same on T7:
#   CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal" ./scripts/release_macos_signed_installer.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

: "${CHOROBOROS_CMAKE_BUILD_DIR:=Universal-Build}"

# Absolute path to CMake binary dir (for cd / rm)
if [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" = /* ]]; then
    CMAKE_BIN_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}"
else
    CMAKE_BIN_DIR="${REPO_ROOT}/${CHOROBOROS_CMAKE_BUILD_DIR}"
fi

ARTEFACT_RELEASE="${CMAKE_BIN_DIR}/Choroboros_artefacts/Release"
RELEASE_DIR="${REPO_ROOT}/Release"

echo "🎯 Building macOS Universal Binary (arm64 + x86_64)"
echo "=================================================="
echo "CMake binary dir: ${CMAKE_BIN_DIR}"
echo ""

if [[ "${CHOROBOROS_SKIP_UNIVERSAL_CLEAN:-0}" != "1" ]]; then
    echo "🧹 Cleaning this CMake build tree and local zip output..."
    rm -rf "${CMAKE_BIN_DIR}"
    rm -f "${RELEASE_DIR}"/Choroboros-v2.04-beta-macOS-Universal.zip \
        "${RELEASE_DIR}"/Choroboros-v2.04-beta-macOS-Universal.zip.sha256
else
    echo "🧹 SKIP clean (CHOROBOROS_SKIP_UNIVERSAL_CLEAN=1)"
fi
rm -rf "${REPO_ROOT}/Build"

echo "⚙️  Configuring CMake for universal build..."
cmake -B "${CMAKE_BIN_DIR}" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release

echo "🔨 Building universal binary..."
cmake --build "${CMAKE_BIN_DIR}" --config Release --parallel

echo "🔍 Verifying architectures..."
BUNDLE_BASENAME="Choroboros Beta"
VST3_BINARY="${ARTEFACT_RELEASE}/VST3/${BUNDLE_BASENAME}.vst3/Contents/MacOS/${BUNDLE_BASENAME}"
AU_BINARY="${ARTEFACT_RELEASE}/AU/${BUNDLE_BASENAME}.component/Contents/MacOS/${BUNDLE_BASENAME}"
STANDALONE_BINARY="${ARTEFACT_RELEASE}/Standalone/${BUNDLE_BASENAME}.app/Contents/MacOS/${BUNDLE_BASENAME}"

if [[ -f "$VST3_BINARY" ]]; then
    echo "✅ VST3 architectures:"
    file "$VST3_BINARY"
    lipo -info "$VST3_BINARY"
fi

if [[ -f "$AU_BINARY" ]]; then
    echo "✅ AU architectures:"
    file "$AU_BINARY"
    lipo -info "$AU_BINARY"
fi

if [[ -f "$STANDALONE_BINARY" ]]; then
    echo "✅ Standalone architectures:"
    file "$STANDALONE_BINARY"
    lipo -info "$STANDALONE_BINARY"
fi

echo "📦 Packaging universal build..."
mkdir -p "${RELEASE_DIR}"

(
    cd "${ARTEFACT_RELEASE}"
    zip -r -o "${RELEASE_DIR}/Choroboros-v2.04-beta-macOS-Universal.zip" VST3 AU Standalone
)

cd "${RELEASE_DIR}"
unzip -q -o Choroboros-v2.04-beta-macOS-Universal.zip || true
cp "${REPO_ROOT}/README.md" "${REPO_ROOT}/DISTRIBUTION.md" "${REPO_ROOT}/INSTALL.txt" \
    "${REPO_ROOT}/LICENSE" "${REPO_ROOT}/COPYING" "${REPO_ROOT}/SOURCE_LINK.txt" . 2>/dev/null || true
cp "${REPO_ROOT}/install.sh" "${REPO_ROOT}/Install Choroboros.command" . 2>/dev/null || true
chmod +x install.sh "Install Choroboros.command" 2>/dev/null || true
zip -r -o Choroboros-v2.04-beta-macOS-Universal.zip \
    README.md DISTRIBUTION.md INSTALL.txt LICENSE COPYING SOURCE_LINK.txt \
    install.sh "Install Choroboros.command" 2>/dev/null || true
rm -f README.md DISTRIBUTION.md INSTALL.txt LICENSE COPYING SOURCE_LINK.txt install.sh "Install Choroboros.command"
cd "${REPO_ROOT}"

echo "🔐 Generating SHA256 checksum..."
shasum -a 256 "${RELEASE_DIR}/Choroboros-v2.04-beta-macOS-Universal.zip" > "${RELEASE_DIR}/Choroboros-v2.04-beta-macOS-Universal.zip.sha256"

echo ""
echo "✅ macOS Universal build complete!"
echo "📦 Package: Release/Choroboros-v2.04-beta-macOS-Universal.zip"
echo "📊 Size: $(du -h "${RELEASE_DIR}/Choroboros-v2.04-beta-macOS-Universal.zip" | cut -f1)"
echo "📂 Artefacts: ${ARTEFACT_RELEASE}"
