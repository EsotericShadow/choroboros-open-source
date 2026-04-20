#!/bin/bash

# Rebuild, reinstall, and force-rescan Choroboros Beta in REAPER.
# This script hardens the beta path against stale embedded UI art by:
#   - forcing external asset-pack mode
#   - installing the current repo asset pack into a user-writable runtime path
#   - removing stale installed plugin bundles that can shadow the beta
#   - verifying the built/installled binaries no longer contain embedded UI art
# It then clears only the matching REAPER VST/AU cache entries.

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
  3. Builds and installs the current shared asset pack from repo Assets
  4. Builds the current Choroboros Beta VST3 and AU with embedded UI fallback OFF
  5. Removes stale installed Choroboros bundles that can shadow the beta
  6. Reinstalls the user-level Choroboros Beta VST3 and AU
  7. Backs up REAPER cache files to /tmp
  8. Removes only the Choroboros Beta REAPER cache entries
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
ASSET_PACK_VERSION="${CHOROBOROS_VERSION}"
ASSET_PACK_DIRNAME="ChoroborosAssets-${ASSET_PACK_VERSION}"
USER_VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
USER_AU_DIR="$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
USER_RELEASE_VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
USER_RELEASE_AU_DIR="$HOME/Library/Audio/Plug-Ins/Components/Choroboros.component"
USER_DISABLED_RELEASE_VST3="$HOME/Library/Audio/Plug-Ins/VST3/Choroboros.vst3.disabled-backup"
USER_DISABLED_BETA_VST3="$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3.disabled-backup"
USER_DISABLED_BETA_AU="$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component.disabled-backup"
USER_ASSET_ROOT="$HOME/Library/Application Support/Kaizen Strategic AI/Choroboros/Assets"
USER_ASSET_PACK_DIR="$USER_ASSET_ROOT/${ASSET_PACK_DIRNAME}"
USER_LEGACY_ASSET_PACK_DIR="$USER_ASSET_ROOT/${ASSET_PACK_VERSION}"
SYSTEM_VST3_DIR="/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
SYSTEM_AU_DIR="/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
SYSTEM_RELEASE_AU_DIR="/Library/Audio/Plug-Ins/Components/Choroboros.component"
SYSTEM_ASSET_ROOT="/Library/Application Support/Kaizen Strategic AI/Choroboros/Assets"
SYSTEM_ASSET_PACK_DIR="$SYSTEM_ASSET_ROOT/${ASSET_PACK_DIRNAME}"
SYSTEM_LEGACY_ASSET_PACK_DIR="$SYSTEM_ASSET_ROOT/${ASSET_PACK_VERSION}"
REAPER_CACHE_DIR="$HOME/Library/Application Support/REAPER"
BUILD_VST3_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release/VST3/${PRODUCT_NAME}.vst3"
BUILD_AU_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release/AU/${PRODUCT_NAME}.component"
ASSET_PACK_BUILD_ROOT="${CHOROBOROS_CMAKE_BUILD_DIR}/asset-pack"
ASSET_PACK_BUILD_DIR="${ASSET_PACK_BUILD_ROOT}/${ASSET_PACK_DIRNAME}"
BINARYDATA_HEADER="${CHOROBOROS_CMAKE_BUILD_DIR}/juce_binarydata_ChoroborosBinaryData/JuceLibraryCode/BinaryData.h"

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

remove_if_exists() {
    local path="$1"

    if [[ ! -e "$path" ]]; then
        return 0
    fi

    if rm -rf "$path" 2>/dev/null; then
        echo "Removed stale path: $path"
        return 0
    fi

    echo "Warning: could not remove stale path: $path" >&2
    return 0
}

validate_skip_configure_flags() {
    local cache_file="${CHOROBOROS_CMAKE_BUILD_DIR}/CMakeCache.txt"

    if [[ ! -f "$cache_file" ]]; then
        echo "Missing CMake cache: $cache_file" >&2
        echo "Run without --skip-configure so the asset-pack-safe flags can be applied." >&2
        exit 1
    fi

    if ! grep -q '^CHOROBOROS_USE_EXTERNAL_ASSET_PACK:BOOL=ON$' "$cache_file"; then
        echo "CMake cache is not configured for external asset-pack mode." >&2
        echo "Run without --skip-configure so the correct flags are applied." >&2
        exit 1
    fi

    if ! grep -q '^CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK:BOOL=OFF$' "$cache_file"; then
        echo "CMake cache still allows embedded UI fallback." >&2
        echo "Run without --skip-configure so the correct flags are applied." >&2
        exit 1
    fi
}

build_asset_pack() {
    mkdir -p "$ASSET_PACK_BUILD_ROOT"
    rm -rf "$ASSET_PACK_BUILD_DIR"

    python3 "$REPO_ROOT/scripts/build_asset_pack.py" \
        --source-root "$REPO_ROOT/Assets" \
        --output-root "$ASSET_PACK_BUILD_ROOT" \
        --pack-version "$ASSET_PACK_VERSION" >/dev/null

    if [[ ! -f "$ASSET_PACK_BUILD_DIR/manifest.json" ]]; then
        echo "Built asset pack manifest not found: $ASSET_PACK_BUILD_DIR/manifest.json" >&2
        exit 1
    fi
}

install_user_asset_pack() {
    mkdir -p "$USER_ASSET_ROOT"
    rm -rf "$USER_ASSET_PACK_DIR" "$USER_LEGACY_ASSET_PACK_DIR"
    ditto "$ASSET_PACK_BUILD_DIR" "$USER_ASSET_PACK_DIR"
    xattr -cr "$USER_ASSET_PACK_DIR" 2>/dev/null || true
}

cleanup_stale_installs() {
    local stale_paths=(
        "$USER_RELEASE_VST3_DIR"
        "$USER_RELEASE_AU_DIR"
        "$USER_DISABLED_RELEASE_VST3"
        "$USER_DISABLED_BETA_VST3"
        "$USER_DISABLED_BETA_AU"
        "$SYSTEM_VST3_DIR"
        "$SYSTEM_AU_DIR"
        "$SYSTEM_RELEASE_AU_DIR"
        "$SYSTEM_ASSET_PACK_DIR"
        "$SYSTEM_LEGACY_ASSET_PACK_DIR"
    )

    for stale_path in "${stale_paths[@]}"; do
        remove_if_exists "$stale_path"
    done
}

assert_text_absent() {
    local file_path="$1"
    local needle="$2"

    python3 - "$file_path" "$needle" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
needle = sys.argv[2]

if not path.exists():
    print(f"Missing file for verification: {path}", file=sys.stderr)
    sys.exit(1)

text = path.read_text(errors="ignore")
if needle in text:
    print(f"Unexpected embedded asset marker '{needle}' found in {path}", file=sys.stderr)
    sys.exit(1)
PY
}

assert_bytes_absent() {
    local file_path="$1"
    local needle="$2"

    python3 - "$file_path" "$needle" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
needle = sys.argv[2].encode("utf-8")

if not path.exists():
    print(f"Missing file for verification: {path}", file=sys.stderr)
    sys.exit(1)

data = path.read_bytes()
if needle in data:
    print(f"Unexpected embedded asset marker '{needle.decode()}' found in {path}", file=sys.stderr)
    sys.exit(1)
PY
}

verify_no_embedded_ui_art() {
    local markers=(
        "green_light_off_backpanel_png"
        "blue_1_on_png"
        "switch_a_spritesheet_png"
    )

    for marker in "${markers[@]}"; do
        assert_text_absent "$BINARYDATA_HEADER" "$marker"
        assert_bytes_absent "$BUILD_VST3_DIR/Contents/MacOS/${PRODUCT_NAME}" "$marker"
        assert_bytes_absent "$BUILD_AU_DIR/Contents/MacOS/${PRODUCT_NAME}" "$marker"
        assert_bytes_absent "$USER_VST3_DIR/Contents/MacOS/${PRODUCT_NAME}" "$marker"
        assert_bytes_absent "$USER_AU_DIR/Contents/MacOS/${PRODUCT_NAME}" "$marker"
    done
}

echo "Refreshing ${PRODUCT_NAME} for REAPER..."
echo "Build directory: ${CHOROBOROS_CMAKE_BUILD_DIR}"
echo ""

require_reaper_closed

if [[ "$SKIP_CONFIGURE" -eq 0 ]]; then
    cmake -S "$REPO_ROOT" -B "$CHOROBOROS_CMAKE_BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCHOROBOROS_USE_EXTERNAL_ASSET_PACK=ON \
        -DCHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK=OFF
else
    validate_skip_configure_flags
fi

build_asset_pack

rm -rf "$BUILD_VST3_DIR" "$BUILD_AU_DIR"
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
cleanup_stale_installs
install_user_asset_pack
rm -rf "$USER_VST3_DIR"
ditto "$BUILD_VST3_DIR" "$USER_VST3_DIR"
xattr -cr "$USER_VST3_DIR" 2>/dev/null || true

mkdir -p "$(dirname "$USER_AU_DIR")"
rm -rf "$USER_AU_DIR"
ditto "$BUILD_AU_DIR" "$USER_AU_DIR"
xattr -cr "$USER_AU_DIR" 2>/dev/null || true

verify_no_embedded_ui_art

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
echo "Installed asset pack: $USER_ASSET_PACK_DIR"
echo "REAPER cache backups: $BACKUP_DIR"
echo "Next REAPER launch should rescan only ${PRODUCT_NAME}."
