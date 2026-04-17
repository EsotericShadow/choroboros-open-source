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
#   - AAX: run sign_aax_pace.sh before this so .aaxplugin is Developer ID + PACE signed
#   - Omit AAX from the product: export CHOROBOROS_SKIP_AAX_PACKAGE=1 (CI / no PACE)
#   - Xcode Command Line Tools installed
#
# Output:
#   dist/Choroboros-Beta-v2.05-Installer.pkg (unsigned)
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
PUBLIC_VERSION="${CHOROBOROS_PUBLIC_VERSION}"
BUILD_DIR="${CHOROBOROS_BUILD_DIR}"
BUNDLE_BASENAME="${CHOROBOROS_BUNDLE_BASENAME}"
PRODUCT_SLUG="${CHOROBOROS_PRODUCT_SLUG}"

# Working directories
INSTALLER_DIR="installer"
STAGING_DIR="${INSTALLER_DIR}/staging"
COMPONENTS_DIR="${INSTALLER_DIR}/components"
DIST_DIR="dist"
ASSET_PACK_BUILD_DIR="${STAGING_DIR}/generated-asset-pack"
ASSET_PACK_OUTPUT_DIR="${ASSET_PACK_BUILD_DIR}/ChoroborosAssets-${VERSION}"

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

# Check that at least VST3 or AU exists
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
            echo "  ERROR: $(basename "$BUNDLE") must be signed with Developer ID Application (Kaizen) for a shippable .pkg."
            echo "  Found: $SIGNING_ID"
            echo "  Run: ./installer/sign_bundles.sh"
            echo "  For local experiments only: CHOROBOROS_ALLOW_NON_DEV_ID_SIGNING=1 ./installer/build_installer.sh"
            if [[ "${CHOROBOROS_ALLOW_NON_DEV_ID_SIGNING:-}" != "1" ]]; then
                exit 1
            fi
            echo "  Continuing (CHOROBOROS_ALLOW_NON_DEV_ID_SIGNING=1)."
        fi
    fi
done

AAX_BUNDLE="${BUILD_DIR}/AAX/${BUNDLE_BASENAME}.aaxplugin"
WRAPTOOL_BIN="${WRAPTOOL_PATH:-/Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool}"
HAS_AAX=0
if [[ "${CHOROBOROS_SKIP_AAX_PACKAGE:-}" == "1" ]]; then
    echo "  AAX: skipped (CHOROBOROS_SKIP_AAX_PACKAGE=1)"
elif [ -e "$AAX_BUNDLE" ]; then
    echo "Checking AAX bundle (must be Developer ID + PACE signed)..."
    if ! codesign --verify --deep --strict "$AAX_BUNDLE" 2>/dev/null; then
        echo "ERROR: AAX codesign verify failed: $AAX_BUNDLE"
        echo "Run ./installer/sign_aax_pace.sh or set CHOROBOROS_SKIP_AAX_PACKAGE=1 to omit AAX from this .pkg."
        exit 1
    fi
    if [[ ! -x "$WRAPTOOL_BIN" ]]; then
        echo "ERROR: wraptool not found at $WRAPTOOL_BIN (PACE Eden). Install Eden or set WRAPTOOL_PATH."
        exit 1
    fi
    if ! "$WRAPTOOL_BIN" verify --in "$AAX_BUNDLE" >/dev/null 2>&1; then
        echo "ERROR: wraptool verify failed for AAX. Run ./installer/sign_aax_pace.sh"
        exit 1
    fi
    echo "  OK: AAX signed (codesign + PACE verify)"
    HAS_AAX=1
else
    echo "  AAX: no bundle at ${AAX_BUNDLE} (omitted from installer)"
fi
echo ""

# ---- Clean previous staging --------------------------------------------------

echo "Cleaning previous build artifacts..."
rm -rf "${STAGING_DIR}" "${COMPONENTS_DIR}" "${DIST_DIR}"
mkdir -p "${STAGING_DIR}" "${COMPONENTS_DIR}" "${DIST_DIR}"

# ---- Stage payloads ----------------------------------------------------------
# Each payload directory mirrors the final install location on the target Mac.
# pkgbuild uses --install-location / so the directory structure inside the
# payload becomes the absolute path on the target system.

echo "Staging installer payloads..."

if ! command -v python3 >/dev/null 2>&1; then
    echo "ERROR: python3 is required to build the shared asset pack."
    exit 1
fi

echo "Generating shared asset pack..."
python3 scripts/build_asset_pack.py \
    --source-root Assets \
    --output-root "${ASSET_PACK_BUILD_DIR}" \
    --pack-version "${VERSION}" >/dev/null

if [ ! -f "${ASSET_PACK_OUTPUT_DIR}/manifest.json" ]; then
    echo "ERROR: Shared asset pack manifest missing at ${ASSET_PACK_OUTPUT_DIR}/manifest.json"
    exit 1
fi

RESOURCES_PAYLOAD="${STAGING_DIR}/resources/Library/Application Support/Kaizen Strategic AI/Choroboros/Assets/${VERSION}"
mkdir -p "$(dirname "${RESOURCES_PAYLOAD}")"
rm -rf "${RESOURCES_PAYLOAD}"
ditto "${ASSET_PACK_OUTPUT_DIR}" "${RESOURCES_PAYLOAD}"
echo "  Staged: Shared resources"

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

# AAX -> /Library/Application Support/Avid/Audio/Plug-Ins/ (Pro Tools system scan path)
if [[ "$HAS_AAX" -eq 1 ]]; then
    AAX_PAYLOAD="${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins"
    mkdir -p "${AAX_PAYLOAD}"
    rm -rf "${AAX_PAYLOAD}/${BUNDLE_BASENAME}.aaxplugin"
    ditto "$AAX_BUNDLE" "${AAX_PAYLOAD}/${BUNDLE_BASENAME}.aaxplugin"
    xattr -cr "${AAX_PAYLOAD}/${BUNDLE_BASENAME}.aaxplugin" 2>/dev/null || true
    echo "  Staged: AAX (Pro Tools)"
fi

echo ""

# ---- Build component packages ------------------------------------------------

echo "Building component packages..."

# pkgbuild runs installer scripts; they must be executable.
if [ -d "${INSTALLER_DIR}/scripts" ]; then
    chmod 755 "${INSTALLER_DIR}/scripts/postinstall" 2>/dev/null || true
fi
if [ -d "${INSTALLER_DIR}/scripts-aax" ]; then
    chmod 755 "${INSTALLER_DIR}/scripts-aax/postinstall" 2>/dev/null || true
fi

# Shared resources component
if [ -d "${STAGING_DIR}/resources" ]; then
    pkgbuild \
        --root "${STAGING_DIR}/resources" \
        --identifier "${COMPANY_ID}.choroboros.resources" \
        --version "${VERSION}" \
        --install-location / \
        "${COMPONENTS_DIR}/Choroboros-Resources.pkg"
    echo "  Built: Choroboros-Resources.pkg"
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

# AAX component (Pro Tools cache helper postinstall)
if [ -d "${STAGING_DIR}/aax" ]; then
    pkgbuild \
        --root "${STAGING_DIR}/aax" \
        --identifier "${COMPANY_ID}.choroboros.aax" \
        --version "${VERSION}" \
        --install-location / \
        --scripts "${INSTALLER_DIR}/scripts-aax" \
        "${COMPONENTS_DIR}/Choroboros-AAX.pkg"
    echo "  Built: Choroboros-AAX.pkg (with AAX cache postinstall)"
fi

echo ""

# ---- Build product archive ---------------------------------------------------

echo "Building product archive..."

INSTALLER_PKG="${DIST_DIR}/${PRODUCT_SLUG}-${PUBLIC_VERSION}-Installer.pkg"
DIST_XML="${STAGING_DIR}/distribution.build.xml"
if [[ "$HAS_AAX" -eq 1 ]]; then
    cp "${INSTALLER_DIR}/distribution.xml" "$DIST_XML"
else
    python3 "${INSTALLER_DIR}/strip_distribution_remove_aax.py" \
        "${INSTALLER_DIR}/distribution.xml" "$DIST_XML"
fi

productbuild \
    --distribution "$DIST_XML" \
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
