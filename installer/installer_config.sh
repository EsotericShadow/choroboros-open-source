#!/bin/bash
# =============================================================================
# Single source of truth for installer / signing scripts (plus bundle basename).
# =============================================================================
# Release checklist — keep these aligned when you bump a version:
#
# 1. CMakeLists.txt
#    - project(Choroboros VERSION x.y.z)
#    - juce_add_plugin( ... VERSION x.y.z ... PRODUCT_NAME "..." )
# 2. This file: CHOROBOROS_VERSION and CHOROBOROS_BUNDLE_BASENAME
# 3. installer/distribution.xml — all pkg-ref version="x.y.z" attributes (three places)
#
# One command for the full macOS pipeline (from repo root):
#   ./scripts/release_macos_signed_installer.sh
# =============================================================================
# shellcheck shell=bash
export CHOROBOROS_BUNDLE_BASENAME="Choroboros Beta"
export CHOROBOROS_VERSION="2.0.41"
export CHOROBOROS_COMPANY_ID="com.kaizenstrategicai"
export CHOROBOROS_BUILD_DIR="Universal-Build/Choroboros_artefacts/Release"
