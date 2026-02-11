#!/bin/bash

# Clear Plugin Cache Script for macOS
# Clears cached plugin information in Reaper, Ableton Live, and GarageBand

echo "🧹 Clearing plugin caches for Reaper, Ableton Live, and GarageBand..."
echo ""

# Reaper cache locations
REAPER_CACHE="$HOME/Library/Application Support/REAPER"
REAPER_PLUGIN_CACHE="$REAPER_CACHE/reaper-vstplugins64.ini"
REAPER_AU_CACHE="$REAPER_CACHE/reaper-auplugins-bc.txt"

echo "📦 Reaper:"
if [ -f "$REAPER_PLUGIN_CACHE" ]; then
    rm -f "$REAPER_PLUGIN_CACHE"
    echo "  ✅ Cleared VST3 cache"
else
    echo "  ℹ️  VST3 cache not found (may not exist yet)"
fi

if [ -f "$REAPER_AU_CACHE" ]; then
    rm -f "$REAPER_AU_CACHE"
    echo "  ✅ Cleared AU cache"
else
    echo "  ℹ️  AU cache not found (may not exist yet)"
fi

# Clear Reaper's plugin blacklist if it exists
if [ -f "$REAPER_CACHE/reaper-vstplugins64-blacklist.ini" ]; then
    rm -f "$REAPER_CACHE/reaper-vstplugins64-blacklist.ini"
    echo "  ✅ Cleared plugin blacklist"
fi

# Ableton Live cache locations
# Ableton stores cache in multiple locations depending on version
ABLETON_CACHE_DIRS=(
    "$HOME/Library/Caches/Ableton"
    "$HOME/Library/Application Support/Ableton"
    "$HOME/Library/Preferences/Ableton"
)

echo ""
echo "📦 Ableton Live:"
for dir in "${ABLETON_CACHE_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        # Find and remove plugin cache files
        find "$dir" -name "*plugin*cache*" -o -name "*vst*cache*" -o -name "*au*cache*" 2>/dev/null | while read -r file; do
            if [ -f "$file" ]; then
                rm -f "$file"
                echo "  ✅ Removed: $(basename "$file")"
            fi
        done
        
        # Remove specific cache files if they exist
        if [ -f "$dir/Live 11*/Preferences/PluginCache.cfg" ] || [ -f "$dir/Live 12*/Preferences/PluginCache.cfg" ]; then
            find "$dir" -name "PluginCache.cfg" -delete 2>/dev/null
            echo "  ✅ Cleared PluginCache.cfg"
        fi
    fi
done

# GarageBand cache
GARAGEBAND_CACHE="$HOME/Library/Caches/com.apple.garageband10"
GARAGEBAND_PREFS="$HOME/Library/Preferences/com.apple.garageband10.plist"

echo ""
echo "📦 GarageBand:"
if [ -d "$GARAGEBAND_CACHE" ]; then
    rm -rf "$GARAGEBAND_CACHE"
    echo "  ✅ Cleared GarageBand cache directory"
else
    echo "  ℹ️  GarageBand cache not found"
fi

# macOS Audio Unit cache (affects all DAWs)
AUDIO_UNIT_CACHE="$HOME/Library/Caches/AudioUnitCache"
AUDIO_COMPONENT_CACHE="$HOME/Library/Caches/com.apple.audiounits.cache"

echo ""
echo "📦 macOS Audio Unit Cache (affects all DAWs):"
if [ -f "$AUDIO_UNIT_CACHE" ]; then
    rm -f "$AUDIO_UNIT_CACHE"
    echo "  ✅ Cleared AudioUnitCache"
fi

if [ -f "$AUDIO_COMPONENT_CACHE" ]; then
    rm -f "$AUDIO_COMPONENT_CACHE"
    echo "  ✅ Cleared AudioUnit component cache"
fi

# VST3 cache (system-wide, affects all DAWs)
VST3_CACHE="/Library/Caches/VST3"
if [ -d "$VST3_CACHE" ]; then
    echo "  ⚠️  System VST3 cache found (requires admin to clear)"
    echo "     Run: sudo rm -rf $VST3_CACHE"
fi

echo ""
echo "✅ Plugin caches cleared!"
echo ""
echo "📋 Next Steps:"
echo "1. Quit all DAWs (Reaper, Ableton, GarageBand) if they're running"
echo "2. Reinstall the plugin from the new package:"
echo "   Release/Choroboros-v1.0.1-macOS.zip"
echo "3. Restart your DAW"
echo "4. Rescan plugins in your DAW"
echo ""
echo "⚠️  Note: You may need to manually remove old plugin installations first:"
echo "   - VST3: ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
echo "   - AU: ~/Library/Audio/Plug-Ins/Components/Choroboros.component"
echo ""
