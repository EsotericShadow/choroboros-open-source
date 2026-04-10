#!/bin/bash
# =============================================================================
# Choroboros Beta: Build macOS .pkg Installer
# =============================================================================
# Takes the signed plugin bundles, stages them into payload directories
# that mirror the install destinations, builds individual component
# packages with pkgbuild, then wraps them into a single .pkg product
# archive with productbuild.
#
# Usage:
#   ./installer/build_installer.sh
#
# Full pipeline (typical): ./scripts/release_macos_signed_installer.sh
#
# Prerequisites:
#   - Plugin bundles already signed (run sign_bundles.sh first)
#   - Xcode Command Line Tools installed
#
# Output:
#   dist/ChoroborosBeta-2.0.41-Installer.pkg (unsigned)
#
# After this, sign and notarize with:
#   ./installer/sign_and_notarize.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=installer/installer_config.sh
source "${SCRIPT_DIR}/installer_config.sh"

# ---- Configuration ----------------------------------------------------------

COMPANY_ID="${CHOROBOROS_COMPANY_ID}"
VERSION="${CHOROBOROS_VERSION}"
BUILD_DIR="${CHOROBOROS_BUILD_DIR}"
BUNDLE_BASENAME="${CHOROBOROS_BUNDLE_BASENAME}"

# Working directories
INSTALLER_DIR="installer"
STAGING_DIR="${INSTALLER_DIR}/staging"
COMPONENTS_DIR="${INSTALLER_DIR}/components"
DIST_DIR="dist"

# ---- Preflight checks -------------------------------------------------------

echo ""
echo "============================================"
echo "  Choroboros Beta: Build .pkg Installer"
echo "============================================"
echo ""

if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Run this script from the repo root directory."
    exit 1
fi

# Check that at least VST3 or AU exists (the two formats we ship for now)
if [ ! -e "${BUILD_DIR}/VST3/${BUNDLE_BASENAME}.vst3" ] && [ ! -e "${BUILD_DIR}/AU/${BUNDLE_BASENAME}.component" ]; then
    echo "ERROR: No signed build artifacts found."
    echo "Run these first:"
    echo "  ./scripts/build_macos_universal.sh"
    echo "  ./installer/sign_bundles.sh"
    exit 1
fi

# Verify the bundles are actually signed (not just ad-hoc)
echo "Checking that bundles are properly signed..."
for BUNDLE in "${BUILD_DIR}/VST3/${BUNDLE_BASENAME}.vst3" "${BUILD_DIR}/AU/${BUNDLE_BASENAME}.component" "${BUILD_DIR}/Standalone/${BUNDLE_BASENAME}.app"; do
    if [ -e "$BUNDLE" ]; then
        if ! codesign --verify --deep --strict "$BUNDLE" 2>/dev/null; then
            echo "WARNING: $(basename "$BUNDLE") signature verification failed."
            echo "Run ./installer/sign_bundles.sh first."
            exit 1
        fi
        # Check it's not just an ad-hoc signature
        SIGNING_ID=$(codesign -dvv "$BUNDLE" 2>&1 | grep "Authority=" | head -1 || true)
        if echo "$SIGNING_ID" | grep -q "Authority=Developer ID Application: Kaizen"; then
            echo "  OK: $(basename "$BUNDLE") signed with Developer ID"
        else
            echo "  WARNING: $(basename "$BUNDLE") may not be signed with Developer ID."
            echo "  Found: $SIGNING_ID"
            echo "  Run ./installer/sign_bundles.sh to sign properly."
            read -p "  Continue anyway? (y/n) " -n 1 -r
            echo ""
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
        fi
    fi
done
echo ""

# ---- Clean previous staging --------------------------------------------------

echo "Cleaning previous build artifacts..."
rm -rf "${STAGING_DIR}" "${COMPONENTS_DIR}" "${DIST_DIR}"
mkdir -p "${STAGING_DIR}" "${COMPONENTS_DIR}" "${DIST_DIR}"

# ---- Stage payloads ----------------------------------------------------------
# Each payload directory mirrors the final install location on the target Mac.
# pkgbuild uses --install-location / so the directory structure inside the
# payload becomes the absolute path on the target system.

echo "Staging plugin binaries..."

# VST3 -> /Library/Audio/Plug-Ins/VST3/
if [ -e "${BUILD_DIR}/VST3/${BUNDLE_BASENAME}.vst3" ]; then
    VST3_PAYLOAD="${STAGING_DIR}/vst3/Library/Audio/Plug-Ins/VST3"
    mkdir -p "${VST3_PAYLOAD}"
    rm -rf "${VST3_PAYLOAD}/${BUNDLE_BASENAME}.vst3"
    ditto "${BUILD_DIR}/VST3/${BUNDLE_BASENAME}.vst3" "${VST3_PAYLOAD}/${BUNDLE_BASENAME}.vst3"
    xattr -cr "${VST3_PAYLOAD}/${BUNDLE_BASENAME}.vst3" 2>/dev/null || true
    echo "  Staged: VST3"
fi

# AU -> /Library/Audio/Plug-Ins/Components/
if [ -e "${BUILD_DIR}/AU/${BUNDLE_BASENAME}.component" ]; then
    AU_PAYLOAD="${STAGING_DIR}/au/Library/Audio/Plug-Ins/Components"
    mkdir -p "${AU_PAYLOAD}"
    rm -rf "${AU_PAYLOAD}/${BUNDLE_BASENAME}.component"
    ditto "${BUILD_DIR}/AU/${BUNDLE_BASENAME}.component" "${AU_PAYLOAD}/${BUNDLE_BASENAME}.component"
    xattr -cr "${AU_PAYLOAD}/${BUNDLE_BASENAME}.component" 2>/dev/null || true
    echo "  Staged: Audio Unit"
fi

# Standalone -> /Applications/
if [ -e "${BUILD_DIR}/Standalone/${BUNDLE_BASENAME}.app" ]; then
    STANDALONE_PAYLOAD="${STAGING_DIR}/standalone/Applications"
    mkdir -p "${STANDALONE_PAYLOAD}"
    rm -rf "${STANDALONE_PAYLOAD}/${BUNDLE_BASENAME}.app"
    ditto "${BUILD_DIR}/Standalone/${BUNDLE_BASENAME}.app" "${STANDALONE_PAYLOAD}/${BUNDLE_BASENAME}.app"
    xattr -cr "${STANDALONE_PAYLOAD}/${BUNDLE_BASENAME}.app" 2>/dev/null || true
    echo "  Staged: Standalone"
fi

echo ""

# ---- Build component packages ------------------------------------------------

echo "Building component packages..."

# pkgbuild runs installer scripts; they must be executable.
if [ -d "${INSTALLER_DIR}/scripts" ]; then
    chmod 755 "${INSTALLER_DIR}/scripts/postinstall" 2>/dev/null || true
fi

# VST3 component
if [ -d "${STAGING_DIR}/vst3" ]; then
    pkgbuild \
        --root "${STAGING_DIR}/vst3" \
        --identifier "${COMPANY_ID}.choroboros.vst3" \
        --version "${VERSION}" \
        --install-location / \
        "${COMPONENTS_DIR}/Choroboros-VST3.pkg"
    echo "  Built: Choroboros-VST3.pkg"
fi

# AU component (includes postinstall script to clear AU cache)
if [ -d "${STAGING_DIR}/au" ]; then
    pkgbuild \
        --root "${STAGING_DIR}/au" \
        --identifier "${COMPANY_ID}.choroboros.au" \
        --version "${VERSION}" \
        --install-location / \
        --scripts "${INSTALLER_DIR}/scripts" \
        "${COMPONENTS_DIR}/Choroboros-AU.pkg"
    echo "  Built: Choroboros-AU.pkg (with AU cache clear postinstall)"
fi

# Standalone component
if [ -d "${STAGING_DIR}/standalone" ]; then
    pkgbuild \
        --root "${STAGING_DIR}/standalone" \
        --identifier "${COMPANY_ID}.choroboros.standalone" \
        --version "${VERSION}" \
        --install-location / \
        "${COMPONENTS_DIR}/Choroboros-Standalone.pkg"
    echo "  Built: Choroboros-Standalone.pkg"
fi

echo ""

# ---- Build product archive ---------------------------------------------------

echo "Building product archive..."

INSTALLER_PKG="${DIST_DIR}/ChoroborosBeta-${VERSION}-Installer.pkg"

productbuild \
    --distribution "${INSTALLER_DIR}/distribution.xml" \
    --resources "${INSTALLER_DIR}/resources" \
    --package-path "${COMPONENTS_DIR}" \
    "${INSTALLER_PKG}"

echo ""
echo "============================================"
echo "  Installer built successfully"
echo "============================================"
echo ""
echo "  ${INSTALLER_PKG}"
echo ""
echo "  This .pkg is UNSIGNED. Before distributing:"
echo "  ./installer/sign_and_notarize.sh"
echo ""
