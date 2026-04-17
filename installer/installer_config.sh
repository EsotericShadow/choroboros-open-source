#!/bin/bash
# =============================================================================
# Single source of truth for installer / signing scripts (plus bundle basename).
# =============================================================================
# Release checklist — keep these aligned when you bump a version:
#
# 1. CMakeLists.txt
#    - project(Choroboros VERSION x.y.z)
#    - juce_add_plugin( ... VERSION x.y.z ... PRODUCT_NAME "..." )
# 2. This file:
#    - CHOROBOROS_VERSION (internal bundle/pkg version)
#    - CHOROBOROS_PUBLIC_VERSION (tester-facing release label)
#    - CHOROBOROS_BUNDLE_BASENAME / CHOROBOROS_PRODUCT_SLUG
# 3. installer/distribution.xml — all pkg-ref version="x.y.z" attributes (five places: Resources, VST3, AU, Standalone, AAX)
#
# One command for the full macOS pipeline (from repo root):
#   ./scripts/release_macos_signed_installer.sh
#
# Put the heavy CMake tree on an external SSD (internal disk almost full):
#   export CHOROBOROS_CMAKE_BUILD_DIR="/Volumes/T7/ChoroborosCMakeBuilds/universal"
#   ./scripts/release_macos_signed_installer.sh
# Or: ./scripts/build_on_external_ssd.sh
#
# Optional signing overrides (same shell session):
#   CHOROBOROS_APP_SIGN_IDENTITY       — full "Developer ID Application: … (TEAM)" string
#   CHOROBOROS_INSTALLER_SIGN_IDENTITY — full "Developer ID Installer: … (TEAM)" string
#   CHOROBOROS_NOTARY_PROFILE          — notarytool keychain profile (default: notarytool-profile)
# Store notary credentials once:
#   xcrun notarytool store-credentials "notarytool-profile" --apple-id "…" --team-id "…" --password "app-specific-password"
# =============================================================================
# shellcheck shell=bash
export CHOROBOROS_BUNDLE_BASENAME="Choroboros Beta"
export CHOROBOROS_PRODUCT_SLUG="Choroboros-Beta"
export CHOROBOROS_PUBLIC_VERSION="v2.05"
export CHOROBOROS_VERSION="2.0.50"
export CHOROBOROS_COMPANY_ID="com.kaizenstrategicai"

# Directory passed to cmake -B (objects, FetchContent, Choroboros_artefacts/...).
# Export before sourcing this file to override. Relative paths are from repo root.
: "${CHOROBOROS_CMAKE_BUILD_DIR:=Universal-Build}"
export CHOROBOROS_CMAKE_BUILD_DIR

if [[ "${CHOROBOROS_CMAKE_BUILD_DIR}" = /* ]]; then
    export CHOROBOROS_BUILD_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release"
else
    export CHOROBOROS_BUILD_DIR="${CHOROBOROS_CMAKE_BUILD_DIR}/Choroboros_artefacts/Release"
fi
