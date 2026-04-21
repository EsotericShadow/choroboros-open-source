#!/bin/bash
# Build macOS Universal Binary (arm64 + x86_64)
#
# Environment (optional):
#   CHOROBOROS_CMAKE_BUILD_DIR   cmake -B directory (default: Universal-Build under repo).
#                                Set to an absolute path on an external disk, e.g.
#                                /Volumes/T7/ChoroborosCMakeBuilds/universal
#   CHOROBOROS_SKIP_UNIVERSAL_CLEAN   if set to 1, do not rm -rf the CMake build dir first
#   CHOROBOROS_RELEASE_DIR     legacy folder where old macOS zip artefacts were written.
#                              Kept only so this script can remove stale zip outputs.
#   TMPDIR                     use a folder on an external SSD when internal disk is tight (linker temp)
#   CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK  optional override for cmake (default here: OFF).
#                              Set to ON only for local experiments without the shared asset pack.
#
# Full signed installer pipeline:
#   ./scripts/release_macos_signed_installer.sh
# Same on T7:
#   CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal" ./scripts/release_macos_signed_installer.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# shellcheck source=../installer/installer_config.sh
source "${REPO_ROOT}/installer/installer_config.sh"

: "${CHOROBOROS_CMAKE_BUILD_DIR:=Universal-Build}"

PRODUCT_NAME="${CHOROBOROS_BUNDLE_BASENAME}"
PRODUCT_SLUG="${CHOROBOROS_PRODUCT_SLUG}"
PUBLIC_VERSION="${CHOROBOROS_PUBLIC_VERSION}"
# Absolute path to CMake binary dir (for cd / rm)
if [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" = /* ]]; then
    CMAKE_BIN_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}"
else
    CMAKE_BIN_DIR="${REPO_ROOT}/${CHOROBOROS_CMAKE_BUILD_DIR}"
fi

ARTEFACT_RELEASE="${CMAKE_BIN_DIR}/Choroboros_artefacts/Release"

if [[ -n "${CHOROBOROS_RELEASE_DIR:-}" ]]; then
    RELEASE_DIR="${CHOROBOROS_RELEASE_DIR}"
elif [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" == /Volumes/* ]]; then
    RELEASE_DIR="$(dirname "${CMAKE_BIN_DIR}")/Release"
else
    RELEASE_DIR="${REPO_ROOT}/Release"
fi

echo "🎯 Building macOS Universal Binary (arm64 + x86_64)"
echo "=================================================="
echo "CMake binary dir: ${CMAKE_BIN_DIR}"
echo "Legacy zip dir:   ${RELEASE_DIR}"
echo ""

if [[ "${CHOROBOROS_SKIP_UNIVERSAL_CLEAN:-0}" != "1" ]]; then
    echo "🧹 Cleaning this CMake build tree and removing stale macOS zip output..."
    rm -rf "${CMAKE_BIN_DIR}"
    rm -f "${RELEASE_DIR}/${PRODUCT_SLUG}-${PUBLIC_VERSION}-macOS-Universal.zip" \
        "${RELEASE_DIR}/${PRODUCT_SLUG}-${PUBLIC_VERSION}-macOS-Universal.zip.sha256"
else
    echo "🧹 SKIP clean (CHOROBOROS_SKIP_UNIVERSAL_CLEAN=1)"
fi
rm -rf "${REPO_ROOT}/Build"

: "${CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK:=OFF}"

echo "⚙️  Configuring CMake for universal build..."
cmake -B "${CMAKE_BIN_DIR}" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK=${CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK}"

echo "🔨 Building universal binary..."
cmake --build "${CMAKE_BIN_DIR}" --config Release --parallel

echo "🔍 Verifying architectures..."
BUNDLE_BASENAME="${PRODUCT_NAME}"
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

echo ""
echo "✅ macOS Universal build complete!"
echo "📂 Artefacts: ${ARTEFACT_RELEASE}"
echo "📦 Distribution path: installer-only via ./scripts/release_macos_signed_installer.sh"
