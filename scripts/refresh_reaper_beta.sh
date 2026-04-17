#!/bin/bash

# Rebuild, reinstall, and force-rescan Choroboros Beta in REAPER.
# This installs the user-level VST3 and AU bundles, then clears only the
# matching REAPER VST/AU cache entries for Choroboros Beta.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEFAULT_BUILD_DIR="/tmp/choroboros-assets-phase1"

BUILD_DIR="${CHOROBOROS_CMAKE_BUILD_DIR:-$DEFAULT_BUILD_DIR}"
BUILD_JOBS="${CHOROBOROS_BUILD_JOBS:-4}"
SKIP_CONFIGURE=0

usage() {
    cat <<EOF
Usage: ./scripts/refresh_reaper_beta.sh [options]

Options:
  --build-dir PATH   CMake build directory (default: ${DEFAULT_BUILD_DIR})
  --jobs N           Parallel build jobs (default: ${BUILD_JOBS})
  --skip-configure   Skip the cmake -S/-B configure step
  --help             Show this help

Environment:
  CHOROBOROS_CMAKE_BUILD_DIR  Overrides the build directory
  CHOROBOROS_BUILD_JOBS       Overrides the parallel build count

What this does:
  1. Refuses to run if REAPER is open
  2. Configures the project if needed
  3. Builds the current Choroboros Beta VST3 and AU
  4. Reinstalls the user-level Choroboros Beta VST3 and AU
  5. Backs up REAPER cache files to /tmp
  6. Removes only the Choroboros Beta REAPER cache entries
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --jobs)
            BUILD_JOBS="$2"
            shift 2
            ;;
        --skip-configure)
            SKIP_CONFIGURE=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "" >&2
            usage >&2
            exit 1
            ;;
    esac
done

export CHOROBOROS_CMAKE_BUILD_DIR="$BUILD_DIR"

# shellcheck source=../installer/installer_config.sh
source "${REPO_ROOT}/installer/installer_config.sh"

PRODUCT_NAME="${CHOROBOROS_BUNDLE_BASENAME}"
USER_VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
USER_AU_DIR="$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
SYSTEM_AU_DIR="/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
REAPER_CACHE_DIR="$HOME/Library/Application Support/REAPER"
BUILD_VST3_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release/VST3/${PRODUCT_NAME}.vst3"
BUILD_AU_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release/AU/${PRODUCT_NAME}.component"

VST_CACHE_FILES=(
    "$REAPER_CACHE_DIR/reaper-vstplugins_arm64.ini"
    "$REAPER_CACHE_DIR/reaper-vstplugins64.ini"
)

AU_CACHE_FILES=(
    "$REAPER_CACHE_DIR/reaper-auplugins_arm64.ini"
    "$REAPER_CACHE_DIR/reaper-auplugins64.ini"
)

require_reaper_closed() {
    if pgrep -x "REAPER" >/dev/null 2>&1 || pgrep -f "/REAPER.app/Contents/MacOS/REAPER" >/dev/null 2>&1; then
        echo "REAPER appears to be running." >&2
        echo "Close REAPER before running this refresh script." >&2
        exit 1
    fi
}

backup_and_strip_line() {
    local file_path="$1"
    local pattern="$2"
    local backup_dir="$3"

    if [[ ! -f "$file_path" ]]; then
        return
    fi

    cp "$file_path" "$backup_dir/"

    local tmp_file
    tmp_file="$(mktemp)"
    grep -vE "$pattern" "$file_path" > "$tmp_file" || true
    mv "$tmp_file" "$file_path"
}

echo "Refreshing ${PRODUCT_NAME} for REAPER..."
echo "Build directory: ${CHOROBOROS_CMAKE_BUILD_DIR}"
echo ""

require_reaper_closed

if [[ "$SKIP_CONFIGURE" -eq 0 ]]; then
    cmake -S "$REPO_ROOT" -B "$CHOROBOROS_CMAKE_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

cmake --build "$CHOROBOROS_CMAKE_BUILD_DIR" --target Choroboros_VST3 Choroboros_AU --parallel "$BUILD_JOBS"

if [[ ! -d "$BUILD_VST3_DIR" ]]; then
    echo "Built VST3 bundle not found: $BUILD_VST3_DIR" >&2
    exit 1
fi

if [[ ! -d "$BUILD_AU_DIR" ]]; then
    echo "Built AU bundle not found: $BUILD_AU_DIR" >&2
    exit 1
fi

mkdir -p "$(dirname "$USER_VST3_DIR")"
rm -rf "$USER_VST3_DIR"
ditto "$BUILD_VST3_DIR" "$USER_VST3_DIR"
xattr -cr "$USER_VST3_DIR" 2>/dev/null || true

mkdir -p "$(dirname "$USER_AU_DIR")"
rm -rf "$USER_AU_DIR"
ditto "$BUILD_AU_DIR" "$USER_AU_DIR"
xattr -cr "$USER_AU_DIR" 2>/dev/null || true

BACKUP_DIR="/tmp/choroboros-reaper-refresh-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$BACKUP_DIR"

for cache_file in "${VST_CACHE_FILES[@]}"; do
    backup_and_strip_line "$cache_file" 'Choroboros_Beta\.vst3=' "$BACKUP_DIR"
done

for cache_file in "${AU_CACHE_FILES[@]}"; do
    backup_and_strip_line "$cache_file" 'Kaizen DSP: Choroboros Beta=' "$BACKUP_DIR"
done

echo ""
echo "Done."
echo "Installed VST3: $USER_VST3_DIR"
echo "Installed AU:   $USER_AU_DIR"
if [[ -d "$SYSTEM_AU_DIR" ]]; then
    echo "Warning: system AU still exists and was not modified: $SYSTEM_AU_DIR"
    echo "If REAPER keeps opening the old AU, replace or remove that system bundle separately."
fi
echo "REAPER cache backups: $BACKUP_DIR"
echo "Next REAPER launch should rescan only ${PRODUCT_NAME}."
