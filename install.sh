#!/bin/bash
# Choroboros Beta Installation Script
# This script automatically installs Choroboros Beta plugin formats to the correct locations

set -e

PRODUCT_NAME="Choroboros Beta"
LEGACY_PRODUCT_NAME="Choroboros"
PUBLIC_VERSION="v2.05"

echo "=========================================="
echo "  ${PRODUCT_NAME} Installer"
echo "  Version ${PUBLIC_VERSION}"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This installer is for macOS only.${NC}"
    echo "For Windows, copy the VST3 folder contents to:"
    echo "  C:\\Program Files\\Common Files\\VST3\\"
    exit 1
fi

# Check macOS version (minimum 10.13 High Sierra)
MACOS_VERSION=$(sw_vers -productVersion 2>/dev/null || echo "0.0")
MACOS_MAJOR=$(echo "$MACOS_VERSION" | cut -d. -f1)
MACOS_MINOR=$(echo "$MACOS_VERSION" | cut -d. -f2)
if [[ "$MACOS_MAJOR" -lt 11 ]] && [[ "$MACOS_MAJOR" -eq 10 ]] && [[ "$MACOS_MINOR" -lt 13 ]]; then
    echo -e "${RED}Error: ${PRODUCT_NAME} requires macOS 10.13 (High Sierra) or later.${NC}"
    echo "Detected: macOS $MACOS_VERSION"
    echo "The JUCE 8 framework does not support earlier macOS versions."
    exit 1
fi

echo -e "${CYAN}System:${NC} macOS $MACOS_VERSION"
echo ""

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Check for plugin files
VST3_SOURCE="$SCRIPT_DIR/VST3/${PRODUCT_NAME}.vst3"
AU_SOURCE="$SCRIPT_DIR/AU/${PRODUCT_NAME}.component"
STANDALONE_SOURCE="$SCRIPT_DIR/Standalone/${PRODUCT_NAME}.app"

# Installation directories
VST3_USER="$HOME/Library/Audio/Plug-Ins/VST3"
AU_USER="$HOME/Library/Audio/Plug-Ins/Components"
STANDALONE_USER="/Applications"
VST3_TARGET="$VST3_USER/${PRODUCT_NAME}.vst3"
AU_TARGET="$AU_USER/${PRODUCT_NAME}.component"
STANDALONE_TARGET="$STANDALONE_USER/${PRODUCT_NAME}.app"
LEGACY_VST3_TARGET="$VST3_USER/${LEGACY_PRODUCT_NAME}.vst3"
LEGACY_AU_TARGET="$AU_USER/${LEGACY_PRODUCT_NAME}.component"
LEGACY_STANDALONE_TARGET="$STANDALONE_USER/${LEGACY_PRODUCT_NAME}.app"

# Check if files exist
if [ ! -d "$VST3_SOURCE" ] && [ ! -d "$AU_SOURCE" ] && [ ! -d "$STANDALONE_SOURCE" ]; then
    echo -e "${RED}Error: Plugin files not found!${NC}"
    echo "Make sure you've unzipped the package and run this script from the extracted folder."
    echo ""
    echo "Expected locations:"
    echo "  - VST3/${PRODUCT_NAME}.vst3"
    echo "  - AU/${PRODUCT_NAME}.component"
    echo "  - Standalone/${PRODUCT_NAME}.app"
    exit 1
fi

echo "This will install ${PRODUCT_NAME} plugin formats to your system."
echo ""
echo "Installation locations:"
if [ -d "$VST3_SOURCE" ]; then
    echo -e "  ${GREEN}✓${NC} VST3: $VST3_USER"
fi
if [ -d "$AU_SOURCE" ]; then
    echo -e "  ${GREEN}✓${NC} AU: $AU_USER"
fi
if [ -d "$STANDALONE_SOURCE" ]; then
    echo -e "  ${GREEN}✓${NC} Standalone: $STANDALONE_USER"
fi
echo ""

# Ask for confirmation
read -p "Continue? (y/n) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Installation cancelled."
    exit 0
fi

echo ""

# Step 1: Remove quarantine from SOURCE files before copying
# This is the key step that prevents Gatekeeper from blocking the plugin
echo -e "${CYAN}Step 1: Removing macOS quarantine from source files...${NC}"
echo "  (This prevents Gatekeeper from blocking the plugin in your DAW)"
QUARANTINE_OK=true
for SRC in "$VST3_SOURCE" "$AU_SOURCE" "$STANDALONE_SOURCE"; do
    if [ -d "$SRC" ]; then
        if ! xattr -cr "$SRC" 2>/dev/null; then
            echo -e "  ${YELLOW}Warning: Could not clear quarantine on $(basename "$SRC")${NC}"
            QUARANTINE_OK=false
        fi
    fi
done
if $QUARANTINE_OK; then
    echo -e "  ${GREEN}✓ Quarantine cleared${NC}"
fi
echo ""

# Step 2: Install plugin files
echo -e "${CYAN}Step 2: Installing plugin files...${NC}"

# Create directories if they don't exist
mkdir -p "$VST3_USER"
mkdir -p "$AU_USER"

# Install VST3
if [ -d "$VST3_SOURCE" ]; then
    echo -n "  Installing VST3 plugin... "
    if [ -d "$VST3_TARGET" ]; then
        rm -rf "$VST3_TARGET"
        echo -ne "${YELLOW}(replacing current beta) ${NC}"
    elif [ -d "$LEGACY_VST3_TARGET" ]; then
        rm -rf "$LEGACY_VST3_TARGET"
        echo -ne "${YELLOW}(removing legacy bundle) ${NC}"
    fi
    cp -R "$VST3_SOURCE" "$VST3_USER/"
    echo -e "${GREEN}✓${NC}"
fi

# Install AU
if [ -d "$AU_SOURCE" ]; then
    echo -n "  Installing AU plugin... "
    if [ -d "$AU_TARGET" ]; then
        rm -rf "$AU_TARGET"
        echo -ne "${YELLOW}(replacing current beta) ${NC}"
    elif [ -d "$LEGACY_AU_TARGET" ]; then
        rm -rf "$LEGACY_AU_TARGET"
        echo -ne "${YELLOW}(removing legacy bundle) ${NC}"
    fi
    cp -R "$AU_SOURCE" "$AU_USER/"
    echo -e "${GREEN}✓${NC}"
fi

# Install Standalone
if [ -d "$STANDALONE_SOURCE" ]; then
    echo -n "  Installing Standalone app... "
    if [ -d "$STANDALONE_TARGET" ]; then
        rm -rf "$STANDALONE_TARGET"
        echo -ne "${YELLOW}(replacing current beta) ${NC}"
    elif [ -d "$LEGACY_STANDALONE_TARGET" ]; then
        rm -rf "$LEGACY_STANDALONE_TARGET"
        echo -ne "${YELLOW}(removing legacy bundle) ${NC}"
    fi
    cp -R "$STANDALONE_SOURCE" "$STANDALONE_USER/"
    echo -e "${GREEN}✓${NC}"
fi
echo ""

# Step 3: Verify quarantine is cleared on installed files
echo -e "${CYAN}Step 3: Verifying quarantine status on installed files...${NC}"
NEEDS_ATTENTION=false
for INSTALLED in "$VST3_TARGET" "$AU_TARGET" "$STANDALONE_TARGET"; do
    if [ -d "$INSTALLED" ]; then
        # Check if quarantine attribute is still present
        if xattr -l "$INSTALLED" 2>/dev/null | grep -q "com.apple.quarantine"; then
            echo -e "  ${YELLOW}Quarantine still present on $(basename "$INSTALLED"), removing...${NC}"
            xattr -cr "$INSTALLED" 2>/dev/null || true
            # Verify again
            if xattr -l "$INSTALLED" 2>/dev/null | grep -q "com.apple.quarantine"; then
                echo -e "  ${RED}Could not remove quarantine from $(basename "$INSTALLED")${NC}"
                NEEDS_ATTENTION=true
            else
                echo -e "  ${GREEN}✓ Cleared${NC}"
            fi
        else
            echo -e "  ${GREEN}✓ $(basename "$INSTALLED") — clean${NC}"
        fi
    fi
done

if $NEEDS_ATTENTION; then
    echo ""
    echo -e "${YELLOW}Some files still have quarantine attributes.${NC}"
    echo "Try running this script with sudo, or manually run:"
    echo "  sudo xattr -cr ~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
    echo "  sudo xattr -cr ~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
    echo "  sudo xattr -cr /Applications/${PRODUCT_NAME}.app"
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Installation Complete!${NC}"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Open your DAW (Logic Pro, Ableton, Reaper, etc.)"
echo "  2. Rescan plugins in your DAW preferences"
echo "  3. Look for '${PRODUCT_NAME}' in your plugin list"
echo ""
echo -e "${CYAN}Troubleshooting:${NC}"
echo ""
echo "  Plugin doesn't appear in DAW?"
echo "    → Rescan plugins in your DAW, or fully restart it"
echo ""
echo "  macOS still blocks the plugin?"
echo "    → System Settings > Privacy & Security > scroll down > 'Open Anyway'"
echo "    → If that button isn't visible, run in Terminal:"
echo "      xattr -cr ~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
echo "      xattr -cr ~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
echo ""
echo "  AU validation fails in Logic Pro?"
echo "    → Requires macOS 10.13+. Older Macs (Pre-2012) may not be supported."
echo "    → Try: killall -9 AudioComponentRegistrar"
echo "    → Then rescan in Logic"
echo ""
echo "  Need help? info@kaizenstrategic.ai"
echo ""
read -p "Press Enter to exit..."
