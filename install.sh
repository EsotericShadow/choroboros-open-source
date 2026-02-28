#!/bin/bash
# Choroboros Installation Script
# This script automatically installs Choroboros plugins to the correct locations

set -e

echo "=========================================="
echo "  Choroboros Plugin Installer"
echo "  Version 2.02-beta"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This installer is for macOS only.${NC}"
    echo "For Linux, see INSTALL.txt for manual instructions."
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Check for plugin files
VST3_SOURCE="$SCRIPT_DIR/VST3/Choroboros.vst3"
AU_SOURCE="$SCRIPT_DIR/AU/Choroboros.component"
STANDALONE_SOURCE="$SCRIPT_DIR/Standalone/Choroboros.app"

# Installation directories
VST3_USER="$HOME/Library/Audio/Plug-Ins/VST3"
AU_USER="$HOME/Library/Audio/Plug-Ins/Components"
STANDALONE_USER="/Applications"

# Check if files exist
if [ ! -d "$VST3_SOURCE" ] && [ ! -d "$AU_SOURCE" ] && [ ! -d "$STANDALONE_SOURCE" ]; then
    echo -e "${RED}Error: Plugin files not found!${NC}"
    echo "Make sure you've unzipped the package and run this script from the extracted folder."
    echo ""
    echo "Expected locations:"
    echo "  - VST3/Choroboros.vst3"
    echo "  - AU/Choroboros.component"
    echo "  - Standalone/Choroboros.app"
    exit 1
fi

echo "This will install Choroboros plugins to your system."
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
echo "Installing..."

# Create directories if they don't exist
mkdir -p "$VST3_USER"
mkdir -p "$AU_USER"

# Install VST3
if [ -d "$VST3_SOURCE" ]; then
    echo -n "Installing VST3 plugin... "
    if [ -d "$VST3_USER/Choroboros.vst3" ]; then
        rm -rf "$VST3_USER/Choroboros.vst3"
        echo -e "${YELLOW}(removed old version)${NC}"
    fi
    cp -R "$VST3_SOURCE" "$VST3_USER/"
    echo -e "${GREEN}✓ Done${NC}"
fi

# Install AU
if [ -d "$AU_SOURCE" ]; then
    echo -n "Installing AU plugin... "
    if [ -d "$AU_USER/Choroboros.component" ]; then
        rm -rf "$AU_USER/Choroboros.component"
        echo -e "${YELLOW}(removed old version)${NC}"
    fi
    cp -R "$AU_SOURCE" "$AU_USER/"
    echo -e "${GREEN}✓ Done${NC}"
fi

# Install Standalone
if [ -d "$STANDALONE_SOURCE" ]; then
    echo -n "Installing Standalone app... "
    if [ -d "$STANDALONE_USER/Choroboros.app" ]; then
        rm -rf "$STANDALONE_USER/Choroboros.app"
        echo -e "${YELLOW}(removed old version)${NC}"
    fi
    cp -R "$STANDALONE_SOURCE" "$STANDALONE_USER/"
    echo -e "${GREEN}✓ Done${NC}"
fi

# Remove quarantine attributes (macOS security)
echo ""
echo "Removing macOS quarantine attributes..."
if [ -d "$VST3_USER/Choroboros.vst3" ]; then
    xattr -cr "$VST3_USER/Choroboros.vst3" 2>/dev/null || true
fi
if [ -d "$AU_USER/Choroboros.component" ]; then
    xattr -cr "$AU_USER/Choroboros.component" 2>/dev/null || true
fi
if [ -d "$STANDALONE_USER/Choroboros.app" ]; then
    xattr -cr "$STANDALONE_USER/Choroboros.app" 2>/dev/null || true
fi
echo -e "${GREEN}✓ Done${NC}"

echo ""
echo "=========================================="
echo -e "${GREEN}Installation Complete!${NC}"
echo "=========================================="
echo ""
echo "Next steps:"
echo "1. Open your DAW (Logic Pro, Ableton, Reaper, etc.)"
echo "2. Rescan plugins in your DAW preferences"
echo "3. Look for 'Choroboros' in your plugin list"
echo ""
echo "If the plugin doesn't appear:"
echo "- Make sure you rescanned plugins in your DAW"
echo "- Some DAWs require a full restart"
echo "- Check System Preferences > Security & Privacy if macOS blocks it"
echo ""
echo "For help: Greenalderson@gmail.com"
echo ""
read -p "Press Enter to exit..."
