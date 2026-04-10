#!/bin/bash
# Configure/build Choroboros CMake trees on an external drive (e.g. Samsung T7).
# Keeps object files and FetchContent caches off the internal SSD.
#
# Usage:
#   ./scripts/build_on_external_ssd.sh [BUILD_ROOT]
#
# If BUILD_ROOT is omitted, uses $CHOROBOROS_CMAKE_BUILD_ROOT, else the first
# writable match among common macOS mount names:
#   /Volumes/T7, /Volumes/Samsung_T7, /Volumes/T7 Shield, /Volumes/PortableSSD
#
# Example:
#   ./scripts/build_on_external_ssd.sh "/Volumes/T7/ChoroborosBuilds"

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

resolve_build_root() {
    if [[ -n "${1:-}" ]]; then
        echo "$1"
        return
    fi
    if [[ -n "${CHOROBOROS_CMAKE_BUILD_ROOT:-}" ]]; then
        echo "$CHOROBOROS_CMAKE_BUILD_ROOT"
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

ROOT="$(resolve_build_root "${1:-}")"
if [[ -z "$ROOT" ]]; then
    echo "No external build root found. Pass a path, e.g.:" >&2
    echo "  $0 /Volumes/T7/ChoroborosCMakeBuilds" >&2
    echo "or set CHOROBOROS_CMAKE_BUILD_ROOT." >&2
    exit 1
fi

mkdir -p "$ROOT"
echo "Using CMake build root: $ROOT"

run_build() {
    local name="$1"
    shift
    local bdir="$ROOT/$name"
    echo "========== $name =========="
    cmake -S "$REPO_ROOT" -B "$bdir" -G Ninja "$@"
    cmake --build "$bdir" --parallel
}

# Default shipping configuration
run_build release -DCMAKE_BUILD_TYPE=Release

echo "Done. Other presets (run manually with same pattern):"
echo "  ASAN:    -DCMAKE_BUILD_TYPE=Debug -DCHOROBOROS_ENABLE_ASAN=ON"
echo "  TSAN:    -DCMAKE_BUILD_TYPE=Debug -DCHOROBOROS_ENABLE_TSAN=ON"
echo "  Inspector: -DCMAKE_BUILD_TYPE=Debug -DCHOROBOROS_ENABLE_INSPECTOR=ON"
echo "  Perfetto:  -DCMAKE_BUILD_TYPE=Release -DCHOROBOROS_ENABLE_PERFETTO=ON  (CMake 3.24+)"
