#!/bin/bash
# Choroboros Beta v2.05 — Clean Reinstall Script
# Removes ALL old Choroboros installations (both "Choroboros" and "Choroboros Beta")
# then installs the correct v2.05 "Choroboros Beta" builds from the open-source build.
#
# Covers: Reaper, Ableton, Pro Tools, Cubase, FL Studio, Waveform 13, GarageBand, Logic
# (All of these read from the standard macOS plugin paths below.)

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo ""
echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}  Choroboros Beta v2.05 — Clean Reinstall${NC}"
echo -e "${CYAN}================================================${NC}"
echo ""

# ── Source builds (from the open-source macos-universal-release build) ──
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_ROOT/build/macos-universal-release/Choroboros_artefacts/Release"

VST3_SRC="$BUILD_DIR/VST3/Choroboros Beta.vst3"
AU_SRC="$BUILD_DIR/AU/Choroboros Beta.component"
AAX_SRC="$BUILD_DIR/AAX/Choroboros Beta.aaxplugin"
STANDALONE_SRC="$BUILD_DIR/Standalone/Choroboros Beta.app"

# Verify builds exist
missing=false
for src in "$VST3_SRC" "$AU_SRC" "$AAX_SRC" "$STANDALONE_SRC"; do
    if [ ! -d "$src" ]; then
        echo -e "${RED}Missing build: $src${NC}"
        missing=true
    fi
done
if $missing; then
    echo ""
    echo -e "${RED}Build artifacts not found. Run the build first.${NC}"
    exit 1
fi

# Verify this is actually v2.05
VST3_PLIST="$VST3_SRC/Contents/Info.plist"
if [ -f "$VST3_PLIST" ]; then
    BUNDLE_VER=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$VST3_PLIST" 2>/dev/null || echo "unknown")
    BUNDLE_NAME=$(/usr/libexec/PlistBuddy -c "Print :CFBundleName" "$VST3_PLIST" 2>/dev/null || echo "unknown")
    echo -e "Source build: ${GREEN}${BUNDLE_NAME}${NC} version ${GREEN}${BUNDLE_VER}${NC}"
else
    echo -e "${YELLOW}Warning: Could not read Info.plist from VST3 build${NC}"
fi
echo ""

# ── Target install paths ──
USER_VST3="$HOME/Library/Audio/Plug-Ins/VST3"
USER_AU="$HOME/Library/Audio/Plug-Ins/Components"
SYS_VST3="/Library/Audio/Plug-Ins/VST3"
SYS_AU="/Library/Audio/Plug-Ins/Components"
USER_AAX="$HOME/Library/Application Support/Avid/Audio/Plug-Ins"
SYS_AAX="/Library/Application Support/Avid/Audio/Plug-Ins"

# ────────────────────────────────────────────
# PHASE 1: NUKE all old Choroboros installations
# ────────────────────────────────────────────
echo -e "${CYAN}Phase 1: Removing ALL old Choroboros installations...${NC}"
echo ""

nuke_count=0

nuke() {
    local path="$1"
    if [ -d "$path" ] || [ -f "$path" ]; then
        echo -e "  ${YELLOW}Removing:${NC} $path"
        rm -rf "$path"
        nuke_count=$((nuke_count + 1))
    fi
}

sys_nuke() {
    local path="$1"
    if [ -d "$path" ] || [ -f "$path" ]; then
        echo -e "  ${YELLOW}Removing (system):${NC} $path"
        sudo rm -rf "$path" 2>/dev/null || {
            echo -e "  ${RED}Could not remove (needs sudo): $path${NC}"
            SUDO_NEEDED+=("$path")
        }
        nuke_count=$((nuke_count + 1))
    fi
}

SUDO_NEEDED=()

# User-level removals (no sudo)
nuke "$USER_VST3/Choroboros.vst3"
nuke "$USER_VST3/Choroboros Beta.vst3"
nuke "$USER_AU/Choroboros.component"
nuke "$USER_AU/Choroboros Beta.component"
nuke "$USER_AAX/Choroboros.aaxplugin"
nuke "$USER_AAX/Choroboros Beta.aaxplugin"
nuke "$HOME/Applications/Choroboros.app"
nuke "$HOME/Applications/Choroboros Beta.app"

# System-level removals (may need sudo)
sys_nuke "$SYS_VST3/Choroboros.vst3"
sys_nuke "$SYS_VST3/Choroboros Beta.vst3"
sys_nuke "$SYS_AU/Choroboros.component"
sys_nuke "$SYS_AU/Choroboros Beta.component"
sys_nuke "$SYS_AAX/Choroboros.aaxplugin"
sys_nuke "$SYS_AAX/Choroboros Beta.aaxplugin"
sys_nuke "/Applications/Choroboros.app"
sys_nuke "/Applications/Choroboros Beta.app"

if [ $nuke_count -eq 0 ]; then
    echo -e "  ${GREEN}No old installations found${NC}"
else
    echo ""
    echo -e "  ${GREEN}Removed $nuke_count old installation(s)${NC}"
fi

if [ ${#SUDO_NEEDED[@]} -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Some system-level files could not be removed. Run manually:${NC}"
    for p in "${SUDO_NEEDED[@]}"; do
        echo "  sudo rm -rf \"$p\""
    done
fi
echo ""

# ────────────────────────────────────────────
# PHASE 2: Clear AU cache (forces Logic/GarageBand to rescan)
# ────────────────────────────────────────────
echo -e "${CYAN}Phase 2: Clearing plugin caches...${NC}"

# Kill AudioComponentRegistrar so AU cache gets rebuilt
killall -9 AudioComponentRegistrar 2>/dev/null && echo "  Killed AudioComponentRegistrar" || true

# Remove AU cache
AU_CACHE="$HOME/Library/Caches/AudioUnitCache"
if [ -d "$AU_CACHE" ]; then
    rm -rf "$AU_CACHE"
    echo "  Cleared AudioUnitCache"
fi

# Remove VST3 cache used by some DAWs
VST3_CACHE="$HOME/Library/Caches/com.steinberg.vst3"
if [ -d "$VST3_CACHE" ]; then
    rm -rf "$VST3_CACHE"
    echo "  Cleared Steinberg VST3 cache"
fi

echo -e "  ${GREEN}Caches cleared${NC}"
echo ""

# ────────────────────────────────────────────
# PHASE 3: Install v2.05 "Choroboros Beta" builds
# ────────────────────────────────────────────
echo -e "${CYAN}Phase 3: Installing Choroboros Beta v2.05...${NC}"
echo ""

# Create directories
mkdir -p "$USER_VST3"
mkdir -p "$USER_AU"
mkdir -p "$USER_AAX"

# VST3
echo -n "  VST3 → $USER_VST3/ ... "
cp -R "$VST3_SRC" "$USER_VST3/"
xattr -cr "$USER_VST3/Choroboros Beta.vst3" 2>/dev/null || true
echo -e "${GREEN}done${NC}"

# AU (user only — no need for system-level duplicate)
echo -n "  AU   → $USER_AU/ ... "
cp -R "$AU_SRC" "$USER_AU/"
xattr -cr "$USER_AU/Choroboros Beta.component" 2>/dev/null || true
echo -e "${GREEN}done${NC}"

# AAX (user location for dev/testing — Pro Tools checks both)
echo -n "  AAX  → $USER_AAX/ ... "
cp -R "$AAX_SRC" "$USER_AAX/"
xattr -cr "$USER_AAX/Choroboros Beta.aaxplugin" 2>/dev/null || true
echo -e "${GREEN}done${NC}"

# Standalone (install to ~/Applications if /Applications needs sudo)
echo -n "  App  → /Applications/ ... "
cp -R "$STANDALONE_SRC" "/Applications/" 2>/dev/null || {
    mkdir -p "$HOME/Applications"
    cp -R "$STANDALONE_SRC" "$HOME/Applications/"
    xattr -cr "$HOME/Applications/Choroboros Beta.app" 2>/dev/null || true
    echo -e "${GREEN}done (~/Applications)${NC}"
    STANDALONE_FALLBACK=true
}
if [ -z "$STANDALONE_FALLBACK" ]; then
    xattr -cr "/Applications/Choroboros Beta.app" 2>/dev/null || true
    echo -e "${GREEN}done${NC}"
fi

echo ""

# ────────────────────────────────────────────
# PHASE 4: Verify installation
# ────────────────────────────────────────────
echo -e "${CYAN}Phase 4: Verifying installation...${NC}"
echo ""

verify() {
    local path="$1"
    local label="$2"
    if [ -d "$path" ]; then
        local plist="$path/Contents/Info.plist"
        if [ -f "$plist" ]; then
            local ver=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$plist" 2>/dev/null || echo "?")
            local name=$(/usr/libexec/PlistBuddy -c "Print :CFBundleName" "$plist" 2>/dev/null || echo "?")
            echo -e "  ${GREEN}OK${NC} $label: $name v$ver"
        else
            echo -e "  ${YELLOW}??${NC} $label: installed but no Info.plist"
        fi
    else
        echo -e "  ${RED}MISSING${NC} $label: $path"
    fi
}

verify "$USER_VST3/Choroboros Beta.vst3" "VST3"
verify "$USER_AU/Choroboros Beta.component" "AU"
verify "$USER_AAX/Choroboros Beta.aaxplugin" "AAX"
if [ -d "/Applications/Choroboros Beta.app" ]; then
    verify "/Applications/Choroboros Beta.app" "Standalone"
else
    verify "$HOME/Applications/Choroboros Beta.app" "Standalone"
fi

echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Installation complete!${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo "DAW-specific notes:"
echo ""
echo "  Reaper:      Preferences > Plug-ins > VST > Re-scan"
echo "  Ableton:     Preferences > Plug-Ins > Rescan"
echo "  Pro Tools:   Restart PT (auto-rescans AAX folder)"
echo "  Cubase:      Studio > VST Plug-in Manager > Rescan All"
echo "  FL Studio:   Options > Manage Plugins > Start Scan"
echo "  Waveform 13: Settings > Plugins > Re-scan"
echo "  GarageBand:  Restart (auto-rescans AU components)"
echo "  Logic Pro:   Restart (auto-rescans; or Logic > Preferences > Plug-in Manager)"
echo ""
echo "  The plugin should now appear as: Choroboros Beta"
echo "  Version should read: Beta v2.05"
echo ""
