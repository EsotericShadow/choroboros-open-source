#!/bin/bash
# =============================================================================
# Choroboros Beta: Sign, Notarize, and Staple the .pkg Installer
# =============================================================================
# Takes the unsigned .pkg from build_installer.sh, signs it with the
# Developer ID Installer certificate, submits it to Apple for notarization,
# waits for approval, and staples the notarization ticket to the .pkg.
#
# After this script completes, the .pkg is ready for distribution.
# Users can double-click it and install without any Gatekeeper warnings.
#
# Usage:
#   ./installer/sign_and_notarize.sh
#
# Full pipeline: ./scripts/release_macos_signed_installer.sh
#
# Prerequisites:
#   - "Developer ID Installer: Kaizen Strategic AI Inc. (9XLRQU887D)"
#     certificate installed in your keychain
#   - notarytool credentials stored (default profile: notarytool-profile; override with CHOROBOROS_NOTARY_PROFILE)
#   - Unsigned .pkg already built by build_installer.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=installer/installer_config.sh
source "${SCRIPT_DIR}/installer_config.sh"

# ---- Configuration ----------------------------------------------------------

# Override on the shell if needed:
#   CHOROBOROS_INSTALLER_SIGN_IDENTITY='Developer ID Installer: …'
#   CHOROBOROS_NOTARY_PROFILE=my-profile
INSTALLER_IDENTITY="${CHOROBOROS_INSTALLER_SIGN_IDENTITY:-Developer ID Installer: Kaizen Strategic AI Inc. (9XLRQU887D)}"

# The notarytool keychain profile you created with:
#   xcrun notarytool store-credentials "notarytool-profile" ...
NOTARY_PROFILE="${CHOROBOROS_NOTARY_PROFILE:-notarytool-profile}"

VERSION="${CHOROBOROS_VERSION}"
DIST_DIR="dist"
INSTALLER_DIR="installer"
COMPONENTS_DIR="${INSTALLER_DIR}/components"
UNSIGNED_PKG="${DIST_DIR}/ChoroborosBeta-${VERSION}-Installer.pkg"
SIGNED_PKG="${DIST_DIR}/ChoroborosBeta-${VERSION}-Installer-Signed.pkg"

# ---- Preflight checks -------------------------------------------------------

echo ""
echo "============================================"
echo "  Choroboros Beta: Sign, Notarize, Staple"
echo "============================================"
echo ""

if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Run this script from the repo root directory."
    exit 1
fi

HAVE_COMPONENTS=false
if [ -f "${COMPONENTS_DIR}/Choroboros-VST3.pkg" ] \
    && [ -f "${COMPONENTS_DIR}/Choroboros-AU.pkg" ] \
    && [ -f "${COMPONENTS_DIR}/Choroboros-Standalone.pkg" ]; then
    HAVE_COMPONENTS=true
fi

if [ "$HAVE_COMPONENTS" = false ] && [ ! -f "$UNSIGNED_PKG" ]; then
    echo "ERROR: Nothing to sign."
    echo "  Either run ./installer/build_installer.sh first (keeps ${COMPONENTS_DIR}/),"
    echo "  or ensure unsigned product exists: $UNSIGNED_PKG"
    exit 1
fi

# Check that the Installer identity exists in the keychain
if ! security find-identity -v | grep -Fq "${INSTALLER_IDENTITY}"; then
    echo "ERROR: Developer ID Installer certificate not found in keychain."
    echo ""
    echo "Expected a line matching: ${INSTALLER_IDENTITY}"
    echo ""
    echo "Run this to check what's installed:"
    echo "  security find-identity -v"
    exit 1
fi

# ---- Step 1: Sign the .pkg --------------------------------------------------

echo "Step 1: Signing .pkg with Developer ID Installer certificate..."
echo ""

rm -f "$SIGNED_PKG"

if [ "$HAVE_COMPONENTS" = true ]; then
    # Re-wrap from component .pkgs (same as build_installer). More reliable than
    # productsign on some product archives that reference flaky component metadata.
    echo "Using productbuild --sign with component packages in ${COMPONENTS_DIR}/"
    productbuild \
        --distribution "${INSTALLER_DIR}/distribution.xml" \
        --resources "${INSTALLER_DIR}/resources" \
        --package-path "${COMPONENTS_DIR}" \
        --sign "$INSTALLER_IDENTITY" \
        "$SIGNED_PKG"
else
    echo "Component packages missing; signing flat product with productsign..."
    productsign \
        --sign "$INSTALLER_IDENTITY" \
        "$UNSIGNED_PKG" \
        "$SIGNED_PKG"
fi

# Verify the signature
echo ""
echo "Verifying .pkg signature..."
pkgutil --check-signature "$SIGNED_PKG"
echo ""
echo "OK: .pkg signed successfully."
echo ""

# ---- Step 2: Notarize -------------------------------------------------------

echo "Step 2: Submitting to Apple for notarization..."
echo "(This usually takes 1 to 5 minutes. Occasionally longer.)"
echo ""

NOTARY_LOG=$(mktemp)
xcrun notarytool submit "$SIGNED_PKG" \
    --keychain-profile "$NOTARY_PROFILE" \
    --wait 2>&1 | tee "$NOTARY_LOG"

echo ""
if grep -qi "status: Invalid" "$NOTARY_LOG"; then
    echo "NOTARIZATION FAILED (status Invalid)."
    SUB_ID=$(grep -Eo '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' "$NOTARY_LOG" | head -1)
    if [ -n "$SUB_ID" ]; then
        echo "Fetching log for submission $SUB_ID ..."
        xcrun notarytool log "$SUB_ID" --keychain-profile "$NOTARY_PROFILE" || true
    fi
    rm -f "$NOTARY_LOG"
    exit 1
fi

if ! grep -qi "status: Accepted" "$NOTARY_LOG"; then
    echo "WARNING: Could not confirm Accepted in notarytool output. Check the log above before shipping."
fi
rm -f "$NOTARY_LOG"
echo ""

# ---- Step 3: Staple ---------------------------------------------------------

echo "Step 3: Stapling notarization ticket to .pkg..."
echo ""

xcrun stapler staple "$SIGNED_PKG"

echo ""

# ---- Final verification ------------------------------------------------------

echo "Final verification..."
echo ""

spctl --assess --type install --verbose "$SIGNED_PKG" 2>&1

echo ""
echo "============================================"
echo "  DONE: Ready for distribution"
echo "============================================"
echo ""
echo "  ${SIGNED_PKG}"
echo ""
echo "  This .pkg is signed, notarized, and stapled."
echo "  Users can double-click to install with no"
echo "  Gatekeeper warnings."
echo ""
echo "  To distribute:"
echo "    1. Upload to GitHub Releases"
echo "    2. Link from choroboros.com"
echo "    3. Send to beta testers"
echo ""
