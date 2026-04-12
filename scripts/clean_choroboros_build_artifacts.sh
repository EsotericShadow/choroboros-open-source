#!/bin/bash
# Remove regenerable build / installer junk under choroboros-open-source (frees disk).
# Does not delete source, JUCE submodule, or git state.
#
# Usage (from repo root):
#   ./scripts/clean_choroboros_build_artifacts.sh           # print sizes only (dry run)
#   ./scripts/clean_choroboros_build_artifacts.sh --yes     # delete listed paths
#
# Optional: same CHOROBOROS_CMAKE_BUILD_DIR as your builds (default Universal-Build).
#   CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal" \
#     ./scripts/clean_choroboros_build_artifacts.sh --yes

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

: "${CHOROBOROS_CMAKE_BUILD_DIR:=Universal-Build}"

if [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" = /* ]]; then
    CMAKE_BIN_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}"
else
    CMAKE_BIN_DIR="${REPO_ROOT}/${CHOROBOROS_CMAKE_BUILD_DIR}"
fi

paths=(
    "${CMAKE_BIN_DIR}"
    "${REPO_ROOT}/build"
    "${REPO_ROOT}/Build"
    "${REPO_ROOT}/installer/staging"
    "${REPO_ROOT}/installer/components"
    "${REPO_ROOT}/installer/au-payload"
    "${REPO_ROOT}/installer/vst3-payload"
    "${REPO_ROOT}/installer/standalone-payload"
    "${REPO_ROOT}/dist"
)

# Optional glob build-*
shopt -s nullglob
for d in "${REPO_ROOT}"/build-*; do
    paths+=("$d")
done
shopt -u nullglob

echo "Choroboros open-source artifact cleanup"
echo "Repo: ${REPO_ROOT}"
echo ""

zip_list=()
sha_list=()
shopt -s nullglob
for f in "${REPO_ROOT}/Release"/Choroboros-*-macOS-Universal.zip; do
    zip_list+=("$f")
done
for f in "${REPO_ROOT}/Release"/Choroboros-*-macOS-Universal.zip.sha256; do
    sha_list+=("$f")
done
shopt -u nullglob

for p in "${paths[@]}"; do
    if [[ -e "$p" ]]; then
        sz=$(du -sh "$p" 2>/dev/null | cut -f1 || echo "?")
        echo "  $sz  $p"
    fi
done
if ((${#zip_list[@]} > 0)); then
    for p in "${zip_list[@]}"; do
        [[ -f "$p" ]] || continue
        sz=$(du -sh "$p" 2>/dev/null | cut -f1)
        echo "  $sz  $p"
    done
fi
if ((${#sha_list[@]} > 0)); then
    for p in "${sha_list[@]}"; do
        [[ -f "$p" ]] || continue
        echo "  (small)  $p"
    done
fi

echo ""
if [[ "${1:-}" != "--yes" ]]; then
    echo "Dry run only. To delete the paths above, run:"
    echo "  $0 --yes"
    echo ""
    echo "Tip: internal disk full — set builds to T7, then clean both:"
    echo "  CHOROBOROS_CMAKE_BUILD_DIR=/Volumes/T7/ChoroborosCMakeBuilds/universal $0 --yes"
    exit 0
fi

for p in "${paths[@]}"; do
    if [[ -e "$p" ]]; then
        echo "Removing: $p"
        rm -rf "$p"
    fi
done
((${#zip_list[@]})) && rm -f "${zip_list[@]}"
((${#sha_list[@]})) && rm -f "${sha_list[@]}"

echo ""
echo "Done. df -h . for free space on this volume."
