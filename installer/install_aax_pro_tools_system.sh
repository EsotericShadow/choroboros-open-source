#!/usr/bin/env bash
# Install Choroboros Beta.aaxplugin where Pro Tools looks (system Library).
# Avid documents: Macintosh HD/Library/Application Support/Avid/Audio/Plug-Ins
# The user ~/Library/... path is NOT scanned for AAX on standard Pro Tools setups.
#
# Usage (from choroboros-open-source):
#   ./installer/install_aax_pro_tools_system.sh
#
# Uses CHOROBOROS_AAX_PATH from .env if set (same sources as sign_aax_pace.sh);
# otherwise build/ then Universal-Build/ under this repo.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

PARENT_ENV="$(cd "$REPO_ROOT/.." && pwd)/.env"
ENV_FILE=""
if [[ -f "${REPO_ROOT}/.env" ]]; then
  ENV_FILE="${REPO_ROOT}/.env"
elif [[ -f "$PARENT_ENV" ]]; then
  ENV_FILE="$PARENT_ENV"
fi
if [[ -n "$ENV_FILE" && -f "$ENV_FILE" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "$ENV_FILE"
  set +a
fi

if [[ -n "${CHOROBOROS_AAX_PATH:-}" ]]; then
  SRC="${CHOROBOROS_AAX_PATH}"
else
  CAND1="${REPO_ROOT}/build/Choroboros_artefacts/Release/AAX/Choroboros Beta.aaxplugin"
  CAND2="${REPO_ROOT}/Universal-Build/Choroboros_artefacts/Release/AAX/Choroboros Beta.aaxplugin"
  if [[ -d "$CAND1" ]]; then
    SRC="$CAND1"
  elif [[ -d "$CAND2" ]]; then
    SRC="$CAND2"
  else
    echo "ERROR: No AAX bundle. Set CHOROBOROS_AAX_PATH in .env or build Release AAX."
    exit 1
  fi
fi

DEST="/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros Beta.aaxplugin"

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: Source missing: $SRC"
  exit 1
fi

echo "Source: $SRC"
echo "Dest:   $DEST (requires sudo)"
sudo rm -rf "$DEST"
sudo ditto "$SRC" "$DEST"
sudo xattr -cr "$DEST"
echo "Installed. Quit Pro Tools, reopen, and wait for plug-in scan."
codesign --verify --deep --strict --verbose=2 "$DEST" 2>&1 | tail -2
