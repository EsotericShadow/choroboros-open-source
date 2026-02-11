#!/bin/bash

# Reinstall Choroboros Plugin Script
# Removes old installations and installs the new version

set -e

VERSION="1.0.1"
PLUGIN_NAME="Choroboros"
RELEASE_DIR="Release/${PLUGIN_NAME}-v${VERSION}"

echo "🔄 Reinstalling Choroboros v${VERSION}..."
echo ""

# Check if release package exists
if [ ! -d "$RELEASE_DIR" ]; then
    echo "❌ Error: Release directory not found: $RELEASE_DIR"
    echo "   Please run ./package.sh first to create the distribution package."
    exit 1
fi

# Step 1: Remove old installations
echo "🗑️  Removing old plugin installations..."

# User locations
USER_VST3="$HOME/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3"
USER_AU="$HOME/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
USER_STANDALONE="$HOME/Applications/${PLUGIN_NAME}.app"

# System locations
SYSTEM_VST3="/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3"
SYSTEM_AU="/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
SYSTEM_STANDALONE="/Applications/${PLUGIN_NAME}.app"

removed_any=false

if [ -d "$USER_VST3" ]; then
    rm -rf "$USER_VST3"
    echo "  ✅ Removed: $USER_VST3"
    removed_any=true
fi

if [ -d "$USER_AU" ]; then
    rm -rf "$USER_AU"
    echo "  ✅ Removed: $USER_AU"
    removed_any=true
fi

if [ -d "$USER_STANDALONE" ]; then
    rm -rf "$USER_STANDALONE"
    echo "  ✅ Removed: $USER_STANDALONE"
    removed_any=true
fi

if [ -d "$SYSTEM_VST3" ]; then
    echo "  ⚠️  System VST3 found (requires admin): $SYSTEM_VST3"
    echo "     Run: sudo rm -rf $SYSTEM_VST3"
fi

if [ -d "$SYSTEM_AU" ]; then
    echo "  ⚠️  System AU found (requires admin): $SYSTEM_AU"
    echo "     Run: sudo rm -rf $SYSTEM_AU"
fi

if [ -d "$SYSTEM_STANDALONE" ]; then
    echo "  ⚠️  System Standalone found (requires admin): $SYSTEM_STANDALONE"
    echo "     Run: sudo rm -rf $SYSTEM_STANDALONE"
fi

if [ "$removed_any" = false ] && [ ! -d "$SYSTEM_VST3" ] && [ ! -d "$SYSTEM_AU" ]; then
    echo "  ℹ️  No old installations found"
fi

echo ""

# Step 2: Clear plugin caches
echo "🧹 Clearing plugin caches..."
./clear_plugin_cache.sh 2>/dev/null || echo "  ⚠️  Cache clear script not found (continuing anyway)"

echo ""

# Step 3: Install new version
echo "📦 Installing new plugin version..."

# Create directories if they don't exist
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$HOME/Applications"

# Install VST3
if [ -d "$RELEASE_DIR/VST3/${PLUGIN_NAME}.vst3" ]; then
    cp -R "$RELEASE_DIR/VST3/${PLUGIN_NAME}.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
    # Remove quarantine attribute to prevent macOS security warnings
    xattr -d com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3" 2>/dev/null || true
    echo "  ✅ Installed VST3 to: ~/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3"
else
    echo "  ⚠️  VST3 not found in release package"
fi

# Install AU
if [ -d "$RELEASE_DIR/AU/${PLUGIN_NAME}.component" ]; then
    cp -R "$RELEASE_DIR/AU/${PLUGIN_NAME}.component" "$HOME/Library/Audio/Plug-Ins/Components/"
    # Remove quarantine attribute to prevent macOS security warnings
    xattr -d com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component" 2>/dev/null || true
    echo "  ✅ Installed AU to: ~/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
else
    echo "  ⚠️  AU not found in release package"
fi

# Install Standalone
if [ -d "$RELEASE_DIR/Standalone/${PLUGIN_NAME}.app" ]; then
    cp -R "$RELEASE_DIR/Standalone/${PLUGIN_NAME}.app" "$HOME/Applications/"
    # Remove quarantine attribute to prevent macOS security warnings
    xattr -d com.apple.quarantine "$HOME/Applications/${PLUGIN_NAME}.app" 2>/dev/null || true
    echo "  ✅ Installed Standalone to: ~/Applications/${PLUGIN_NAME}.app"
else
    echo "  ⚠️  Standalone not found in release package"
fi

echo ""
echo "✅ Installation complete!"
echo ""
echo "📋 Next Steps:"
echo "1. Quit all DAWs (Reaper, Ableton, GarageBand) if they're running"
echo "2. Restart your DAW"
echo "3. Rescan plugins:"
echo "   - Reaper: Preferences > Plug-ins > VST > Clear cache and rescan"
echo "   - Ableton: Preferences > Plug-Ins > Rescan"
echo "   - GarageBand: Restart the app (auto-rescans)"
echo ""
echo "4. Verify copyright shows: (C) 2026 Kaizen Strategic AI Inc"
echo ""
