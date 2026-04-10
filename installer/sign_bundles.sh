#!/bin/bash
# =============================================================================
# Choroboros Beta: Sign Plugin Bundles
# =============================================================================
# Signs the VST3, AU, and Standalone binaries with your Developer ID
# Application certificate. Run this AFTER building in Release mode
# and BEFORE building the .pkg installer.
#
# Usage:
#   ./installer/sign_bundles.sh
#
# Prerequisites:
#   - "Developer ID Application: Kaizen Strategic AI Inc. (9XLRQU887D)"
#     certificate installed in your keychain
#   - Xcode Command Line Tools installed
#   - Universal Release build completed (scripts/build_macos_universal.sh)
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=installer/installer_config.sh
source "${SCRIPT_DIR}/installer_config.sh"

# ---- Configuration ----------------------------------------------------------

# Your Developer ID Application certificate identity.
# This must match exactly what shows in: security find-identity -v -p codesigning
APP_IDENTITY="Developer ID Application: Kaizen Strategic AI Inc. (9XLRQU887D)"

# Path to entitlements file (relative to repo root)
ENTITLEMENTS="installer/entitlements.plist"

BUILD_DIR="${CHOROBOROS_BUILD_DIR}"

# Bundle names = PRODUCT_NAME from CMake (includes space — always quote paths)
VST3_BUNDLE="${BUILD_DIR}/VST3/${CHOROBOROS_BUNDLE_BASENAME}.vst3"
AU_BUNDLE="${BUILD_DIR}/AU/${CHOROBOROS_BUNDLE_BASENAME}.component"
STANDALONE_BUNDLE="${BUILD_DIR}/Standalone/${CHOROBOROS_BUNDLE_BASENAME}.app"
# AAX is built but not signed here. AAX signing requires PACE wraptool
# and the iLok dongle, which is a separate step.

# ---- Preflight checks -------------------------------------------------------

echo ""
echo "============================================"
echo "  Choroboros Beta: Sign Plugin Bundles"
echo "============================================"
echo ""

# Make sure we're running from the repo root
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Run this script from the repo root directory."
    echo "  cd /path/to/choroboros-open-source"
    echo "  ./installer/sign_bundles.sh"
    exit 1
fi

# Make sure the entitlements file exists
if [ ! -f "$ENTITLEMENTS" ]; then
    echo "ERROR: Entitlements file not found at: $ENTITLEMENTS"
    exit 1
fi

# Check that the signing identity exists in the keychain
if ! security find-identity -v -p codesigning | grep -q "Developer ID Application: Kaizen Strategic AI Inc."; then
    echo "ERROR: Developer ID Application certificate not found in keychain."
    echo ""
    echo "Expected: $APP_IDENTITY"
    echo ""
    echo "Run this to check what's installed:"
    echo "  security find-identity -v -p codesigning"
    exit 1
fi

# Check that at least one build artifact exists
FOUND_ANY=false
for BUNDLE in "$VST3_BUNDLE" "$AU_BUNDLE" "$STANDALONE_BUNDLE"; do
    if [ -e "$BUNDLE" ]; then
        FOUND_ANY=true
        break
    fi
done

if [ "$FOUND_ANY" = false ]; then
    echo "ERROR: No build artifacts found in ${BUILD_DIR}/"
    echo ""
    echo "Expected at least one of:"
    echo "  ${VST3_BUNDLE}"
    echo "  ${AU_BUNDLE}"
    echo "  ${STANDALONE_BUNDLE}"
    echo ""
    echo "Build first with:"
    echo "  ./scripts/build_macos_universal.sh"
    exit 1
fi

# ---- Sign each bundle -------------------------------------------------------

sign_bundle() {
    local bundle_path="$1"
    local bundle_name
    bundle_name=$(basename "$bundle_path")

    if [ ! -e "$bundle_path" ]; then
        echo "  SKIP: $bundle_name (not found, skipping)"
        return
    fi

    echo "  Signing $bundle_name..."

    codesign --force \
        --sign "$APP_IDENTITY" \
        --timestamp \
        --options runtime \
        --entitlements "$ENTITLEMENTS" \
        "$bundle_path"

    # Verify the signature
    echo "  Verifying $bundle_name..."
    codesign --verify --deep --strict --verbose=2 "$bundle_path" 2>&1 | tail -1

    echo "  OK: $bundle_name signed and verified."
    echo ""
}

echo "Signing with: $APP_IDENTITY"
echo "Entitlements: $ENTITLEMENTS"
echo ""

sign_bundle "$VST3_BUNDLE"
sign_bundle "$AU_BUNDLE"
sign_bundle "$STANDALONE_BUNDLE"

echo "============================================"
echo "  All bundles signed successfully."
echo "============================================"
echo ""
echo "Next step: build the .pkg installer"
echo "  ./installer/build_installer.sh"
echo ""
