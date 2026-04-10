#!/bin/bash
# Run the universal Release build with the CMake tree on an external drive (e.g. T7).
# This is a thin wrapper around build_macos_universal.sh — same artefacts layout as the
# signed-installer pipeline (CHOROBOROS_BUILD_DIR derived from CHOROBOROS_CMAKE_BUILD_DIR).
#
# Usage:
#   ./scripts/build_on_external_ssd.sh                    # auto: first writable T7-like volume
#   ./scripts/build_on_external_ssd.sh /Volumes/T7/ChoroborosCMakeBuilds/universal
#
# Full release (sign + pkg + notarize) on the T7:
#   CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal" \
#     ./scripts/release_macos_signed_installer.sh
#
# Or after this script:
#   CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal" \
#     ./scripts/release_macos_signed_installer.sh --no-universal-build

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

resolve_parent_root() {
    if [[ -n "${CHOROBOROS_EXTERNAL_BUILD_PARENT:-}" ]]; then
        echo "${CHOROBOROS_EXTERNAL_BUILD_PARENT}"
        return
    fi
    local cand
    for cand in "/Volumes/T7" "/Volumes/Samsung_T7" "/Volumes/T7 Shield" "/Volumes/PortableSSD"; do
        if [[ -d "$cand" && -w "$cand" ]]; then
            echo "$cand/ChoroborosCMakeBuilds"
            return
        fi
    done
    echo ""
}

if [[ -n "${1:-}" ]]; then
    export CHOROBOROS_CMAKE_BUILD_DIR="$1"
else
    if [[ -n "${CHOROBOROS_CMAKE_BUILD_DIR:-}" ]]; then
        : # user already exported full cmake -B path
    else
        PARENT="$(resolve_parent_root)"
        if [[ -z "$PARENT" ]]; then
            echo "No external volume found. Pass the cmake -B path, e.g.:" >&2
            echo "  $0 /Volumes/T7/ChoroborosCMakeBuilds/universal" >&2
            echo "Or set CHOROBOROS_CMAKE_BUILD_DIR or CHOROBOROS_EXTERNAL_BUILD_PARENT." >&2
            exit 1
        fi
        mkdir -p "$PARENT"
        export CHOROBOROS_CMAKE_BUILD_DIR="${PARENT}/universal"
    fi
fi

mkdir -p "$(dirname "${CHOROBOROS_CMAKE_BUILD_DIR}")"
echo "CHOROBOROS_CMAKE_BUILD_DIR=${CHOROBOROS_CMAKE_BUILD_DIR}"
exec "${REPO_ROOT}/scripts/build_macos_universal.sh"
