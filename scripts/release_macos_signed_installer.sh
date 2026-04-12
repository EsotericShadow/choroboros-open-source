#!/bin/bash
# =============================================================================
# Choroboros — one-shot macOS release: universal build → sign bundles → .pkg → notarize
# =============================================================================
# Run from anywhere; the script cds to the repo root.
#
#   ./scripts/release_macos_signed_installer.sh
#
# Flags:
#   --cmake-build-dir PATH cmake -B directory (default: Universal-Build). Use a T7 path when
#                          internal disk is full, e.g. /Volumes/T7/ChoroborosCMakeBuilds/universal
#   --no-universal-build   Skip ./scripts/build_macos_universal.sh (reuse existing artefacts)
#   --to-signed-pkg        Stop after unsigned product .pkg + signed .pkg (no Apple notarize/staple)
#   --notarize-only        Only run installer/sign_and_notarize.sh (components or unsigned pkg must exist)
#   -h, --help             Show this help
#
# Same as env: export CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/.../universal" before running.
#
# Before each release, bump versions in one place and sync the rest — see
# installer/installer_config.sh (checklist at top of that file).
#
# Free disk (dry run): ./scripts/clean_choroboros_build_artifacts.sh
# =============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

INSTALLER_DIR="${REPO_ROOT}/installer"

RUN_UNIVERSAL=true
RUN_SIGN_BUNDLES=true
RUN_BUILD_INSTALLER=true
RUN_NOTARIZE=true

usage() {
    sed -n '1,35p' "$0" | tail -n +2
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --cmake-build-dir)
            if [[ -z "${2:-}" ]]; then
                echo "ERROR: --cmake-build-dir requires a path"
                exit 1
            fi
            export CHOROBOROS_CMAKE_BUILD_DIR="$2"
            shift 2
            ;;
        --no-universal-build)
            RUN_UNIVERSAL=false
            shift
            ;;
        --to-signed-pkg)
            RUN_NOTARIZE=false
            shift
            ;;
        --notarize-only)
            RUN_UNIVERSAL=false
            RUN_SIGN_BUNDLES=false
            RUN_BUILD_INSTALLER=false
            RUN_NOTARIZE=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# shellcheck source=installer/installer_config.sh
source "${INSTALLER_DIR}/installer_config.sh"

# Linker / tool temp on the system volume can ENOSPC when the CMake tree lives on /Volumes/...
if [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" == /Volumes/* ]]; then
    export TMPDIR="${TMPDIR:-$(dirname "${CHOROBOROS_CMAKE_BUILD_DIR}")/tmp}"
    mkdir -p "${TMPDIR}"
fi

if [[ ! -f "${REPO_ROOT}/CMakeLists.txt" ]]; then
    echo "ERROR: CMakeLists.txt not found. Expected repo root at ${REPO_ROOT}"
    exit 1
fi

# --- Version consistency (CMake project / plugin vs installer metadata) -------

verify_release_versions() {
    local project_ver dist_ver
    project_ver=$(sed -n 's/^project([^V]*VERSION \([0-9][0-9.]*\).*/\1/p' "${REPO_ROOT}/CMakeLists.txt" | head -1)
    if [[ -z "$project_ver" ]]; then
        echo "WARNING: Could not parse project() VERSION from CMakeLists.txt; skip sync check."
        return 0
    fi

    if [[ "$project_ver" != "$CHOROBOROS_VERSION" ]]; then
        echo "ERROR: Version mismatch."
        echo "  CMake project() VERSION:     ${project_ver}"
        echo "  installer_config CHOROBOROS_VERSION: ${CHOROBOROS_VERSION}"
        echo "Fix: set CHOROBOROS_VERSION in installer/installer_config.sh (and pkg names in distribution.xml if needed)."
        return 1
    fi

    dist_ver=$(grep '<pkg-ref' "${INSTALLER_DIR}/distribution.xml" | grep -Eo 'version="[0-9][0-9.]*"' | head -1 | grep -Eo '[0-9][0-9.]*' || true)
    if [[ -n "$dist_ver" && "$dist_ver" != "$CHOROBOROS_VERSION" ]]; then
        echo "ERROR: distribution.xml pkg-ref version (${dist_ver}) != CHOROBOROS_VERSION (${CHOROBOROS_VERSION})."
        echo "Update the three version=\"...\" attributes in installer/distribution.xml."
        return 1
    fi

    echo "OK: Release metadata versions agree (${CHOROBOROS_VERSION})."
    return 0
}

echo ""
echo "============================================"
echo "  Choroboros macOS signed installer pipeline"
echo "============================================"
echo ""

verify_release_versions

echo ""
echo "Bundle artefacts: ${CHOROBOROS_BUILD_DIR}"
echo "CMake tree:       ${CHOROBOROS_CMAKE_BUILD_DIR}"
echo ""

if $RUN_UNIVERSAL; then
    echo ">>> [1/4] Universal Release build"
    "${REPO_ROOT}/scripts/build_macos_universal.sh"
    echo ""
else
    echo ">>> [1/4] SKIP universal build (--no-universal-build)"
    echo ""
fi

if $RUN_SIGN_BUNDLES; then
    echo ">>> [2/4] Sign plugin bundles (Developer ID Application)"
    "${INSTALLER_DIR}/sign_bundles.sh"
    echo ""
else
    echo ">>> [2/4] SKIP sign bundles"
    echo ""
fi

if $RUN_BUILD_INSTALLER; then
    echo ">>> [3/4] Build component .pkgs + product installer"
    "${INSTALLER_DIR}/build_installer.sh"
    echo ""
else
    echo ">>> [3/4] SKIP installer build"
    echo ""
fi

if $RUN_NOTARIZE; then
    echo ">>> [4/4] Sign product .pkg, notarize, staple"
    "${INSTALLER_DIR}/sign_and_notarize.sh"
    echo ""
else
    echo ">>> [4/4] SKIP notarize (--to-signed-pkg)"
    echo ""
    echo "When ready for Apple:"
    echo "  ./scripts/release_macos_signed_installer.sh --notarize-only"
    echo "(Keep installer/components/ until then, or re-run steps 2–3.)"
    echo ""
fi

echo "============================================"
echo "  Pipeline step(s) finished"
echo "============================================"
echo ""
echo "Signed installer (after full run):"
echo "  dist/ChoroborosBeta-${CHOROBOROS_VERSION}-Installer-Signed.pkg"
echo ""
