#!/usr/bin/env bash
# =============================================================================
# Choroboros: Apple Developer ID sign + PACE wraptool sign (AAX only)
# =============================================================================
# Prerequisites:
#   - Release AAX built (Choroboros Beta.aaxplugin)
#   - Eden Lite / wraptool installed
#   - Eden Tools + PACE Central Access on your iLok account
#   - `.env` with ILOK_USER, ILOK_PASSWORD, WCGUID (see below)
#
# `.env` location (first match wins):
#   1) choroboros-open-source/.env
#   2) Parent folder (e.g. CHOROS_MASTER/.env) for monorepo layouts
#
# Usage (from choroboros-open-source root):
#   ./installer/sign_aax_pace.sh
#
# Pro Tools on macOS scans the *system* AAX folder, not ~/Library:
#   /Library/Application Support/Avid/Audio/Plug-Ins
# After signing, install with: ./installer/install_aax_pro_tools_system.sh (sudo).
#
# Never commit `.env` — it contains secrets.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

# Allow command-line / parent-shell overrides to beat values from .env.
CLI_CHOROBOROS_AAX_PATH="${CHOROBOROS_AAX_PATH:-}"

PARENT_ENV="$(cd "$REPO_ROOT/.." && pwd)/.env"
ENV_FILE=""
if [[ -f "${REPO_ROOT}/.env" ]]; then
  ENV_FILE="${REPO_ROOT}/.env"
elif [[ -f "$PARENT_ENV" ]]; then
  ENV_FILE="$PARENT_ENV"
fi
if [[ -z "$ENV_FILE" || ! -f "$ENV_FILE" ]]; then
  echo "ERROR: No .env found. Create one in either:"
  echo "  ${REPO_ROOT}/.env"
  echo "  ${PARENT_ENV}"
  echo "  cp .env.example .env   # from choroboros-open-source/"
  exit 1
fi
echo "Using env file: $ENV_FILE"

# Export all assignments in .env for child processes
set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

if [[ -n "$CLI_CHOROBOROS_AAX_PATH" ]]; then
  export CHOROBOROS_AAX_PATH="$CLI_CHOROBOROS_AAX_PATH"
fi

: "${ILOK_USER:?Set ILOK_USER in .env}"
: "${ILOK_PASSWORD:?Set ILOK_PASSWORD in .env}"
: "${WCGUID:?Set WCGUID in .env}"

IDENTITY="${CHOROBOROS_APP_SIGN_IDENTITY:-Developer ID Application: Kaizen Strategic AI Inc. (9XLRQU887D)}"
ENTITLEMENTS="${REPO_ROOT}/installer/entitlements.plist"
WRAPTOOL="${WRAPTOOL_PATH:-/Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool}"

if [[ -n "${CHOROBOROS_AAX_PATH:-}" ]]; then
  AAX="${CHOROBOROS_AAX_PATH}"
else
  CAND1="${REPO_ROOT}/build/Choroboros_artefacts/Release/AAX/Choroboros Beta.aaxplugin"
  CAND2="${REPO_ROOT}/Universal-Build/Choroboros_artefacts/Release/AAX/Choroboros Beta.aaxplugin"
  if [[ -d "$CAND1" ]]; then
    AAX="$CAND1"
  elif [[ -d "$CAND2" ]]; then
    AAX="$CAND2"
  else
    echo "ERROR: No AAX bundle found. Build Release AAX or set CHOROBOROS_AAX_PATH in .env"
    echo "  Tried: $CAND1"
    echo "  Tried: $CAND2"
    exit 1
  fi
fi

if [[ ! -x "$WRAPTOOL" ]]; then
  echo "ERROR: wraptool not found or not executable: $WRAPTOOL"
  exit 1
fi

if [[ ! -f "$ENTITLEMENTS" ]]; then
  echo "ERROR: Entitlements missing: $ENTITLEMENTS"
  exit 1
fi

if ! security find-identity -v -p codesigning 2>/dev/null | grep -Fq "$IDENTITY"; then
  echo "ERROR: Signing identity not found in keychain:"
  echo "  $IDENTITY"
  echo "Run: security find-identity -v -p codesigning"
  exit 1
fi

echo "AAX bundle: $AAX"
echo "wraptool:   $WRAPTOOL"
echo ""

SKIP_APPLE="${SKIP_APPLE_CODESIGN:-0}"
if [[ "$SKIP_APPLE" != "1" ]]; then
  echo "==> Apple codesign (Developer ID + entitlements)"
  codesign --force --sign "$IDENTITY" --timestamp --options runtime \
    --entitlements "$ENTITLEMENTS" "$AAX"
  codesign --verify --deep --strict --verbose=2 "$AAX"
  echo ""
else
  echo "==> Skipping Apple codesign (SKIP_APPLE_CODESIGN=1)"
  echo ""
fi

echo "==> wraptool sync"
"$WRAPTOOL" sync --verbose --account "$ILOK_USER" --password "$ILOK_PASSWORD"

SIGN_ARGS=(sign --verbose --account "$ILOK_USER" --signid "$IDENTITY" --wcguid "$WCGUID" --in "$AAX" --out "$AAX")
if [[ "${WRAPTOOL_ALLOW_SIGNING_SERVICE:-0}" == "1" ]]; then
  SIGN_ARGS+=(--allowsigningservice)
fi

echo "==> wraptool sign"
"$WRAPTOOL" "${SIGN_ARGS[@]}"

echo "==> wraptool verify"
"$WRAPTOOL" verify --verbose --in "$AAX"

echo ""
echo "Done: $AAX"
