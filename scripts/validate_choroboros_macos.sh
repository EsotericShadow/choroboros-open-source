#!/bin/bash
# ============================================================
# Choroboros macOS Validation Script
# Run this from Terminal on your Mac AFTER building the plugin
# ============================================================

set -e
# 1 = auval/pluginval reported failures (exit with this at end).
VALIDATION_FAILED=0

echo "========================================="
echo "  CHOROBOROS macOS VALIDATION"
echo "  $(date)"
echo "========================================="
echo ""

# --- STEP 0: Find the plugin builds ---
# Adjust these paths if your build output is somewhere else
AU_PATH="$HOME/Library/Audio/Plug-Ins/Components/Choroboros Beta.component"
VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3/Choroboros Beta.vst3"

# Also check the build folder
BUILD_VST3="$(find . -name '*.vst3' -path '*/Release/*' 2>/dev/null | head -1)"
BUILD_AU="$(find . -name '*.component' -path '*/Release/*' 2>/dev/null | head -1)"

echo "Looking for plugin builds..."
echo ""

if [ -d "$AU_PATH" ]; then
    echo "  AU found:   $AU_PATH"
else
    echo "  AU not found at: $AU_PATH"
    if [ -n "$BUILD_AU" ]; then
        echo "  AU found in build: $BUILD_AU"
        echo "  Copying to plugin folder..."
        cp -R "$BUILD_AU" "$HOME/Library/Audio/Plug-Ins/Components/"
        echo "  Copied. Restarting audio daemon..."
        killall -9 coreaudiod 2>/dev/null || true
        sleep 2
    else
        echo "  WARNING: No AU build found. Skipping AU validation."
    fi
fi

if [ -d "$VST3_PATH" ]; then
    echo "  VST3 found: $VST3_PATH"
elif [ -n "$BUILD_VST3" ]; then
    VST3_PATH="$BUILD_VST3"
    echo "  VST3 found: $VST3_PATH"
else
    echo "  WARNING: No VST3 build found."
fi

echo ""
echo "========================================="
echo "  PHASE 1: auval (Audio Unit Validation)"
echo "========================================="
echo ""

# AU codes from CMakeLists.txt:
#   AU_MAIN_TYPE = kAudioUnitType_Effect = aufx
#   PLUGIN_CODE = ChBr
#   PLUGIN_MANUFACTURER_CODE = KzDp

AUVAL_LOG="auval_results_$(date +%Y%m%d_%H%M%S).txt"

echo "Running: auval -v aufx ChBr KzDp"
echo "This takes 1 to 5 minutes. Do not interrupt."
echo "Logging to: $AUVAL_LOG"
echo ""

if command -v auval &> /dev/null; then
    set +e
    auval -v aufx ChBr KzDp 2>&1 | tee "$AUVAL_LOG"
    AUVAL_EXIT=${PIPESTATUS[0]}
    set -e

    echo ""
    if [ $AUVAL_EXIT -eq 0 ]; then
        echo "*** auval: ALL TESTS PASSED ***"
    else
        echo "*** auval: SOME TESTS FAILED (exit code: $AUVAL_EXIT) ***"
        echo "Check $AUVAL_LOG for details."
        VALIDATION_FAILED=1
    fi
else
    echo "auval not found. Install Xcode Command Line Tools:"
    echo "  xcode-select --install"
fi

echo ""
echo "========================================="
echo "  PHASE 2: pluginval (VST3 Validation)"
echo "========================================="
echo ""

# Check if pluginval is installed
PLUGINVAL=""
if command -v pluginval &> /dev/null; then
    PLUGINVAL="pluginval"
elif [ -f "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    PLUGINVAL="/Applications/pluginval.app/Contents/MacOS/pluginval"
elif [ -f "$HOME/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    PLUGINVAL="$HOME/Applications/pluginval.app/Contents/MacOS/pluginval"
fi

if [ -z "$PLUGINVAL" ]; then
    echo "pluginval not found."
    echo ""
    echo "Download it from:"
    echo "  https://github.com/Tracktion/pluginval/releases"
    echo ""
    echo "After downloading, either:"
    echo "  1. Move pluginval.app to /Applications/"
    echo "  2. Or run this script again"
    echo ""
else
    PLUGINVAL_LOG="pluginval_results_$(date +%Y%m%d_%H%M%S).txt"

    echo "Using pluginval at: $PLUGINVAL"
    echo ""

    if [ -d "$VST3_PATH" ]; then
        # Level 5 first (minimum for release)
        echo "--- VST3 Validation: Strictness Level 5 (release minimum) ---"
        echo "Running: $PLUGINVAL --validate \"$VST3_PATH\" --strictness-level 5"
        echo "This takes 2 to 10 minutes. Do not interrupt."
        echo ""

        set +e
        "$PLUGINVAL" --validate "$VST3_PATH" \
            --strictness-level 5 \
            --timeout-ms 120000 \
            --verbose 2>&1 | tee "$PLUGINVAL_LOG"
        PV_EXIT=${PIPESTATUS[0]}
        set -e

        echo ""
        if [ $PV_EXIT -eq 0 ]; then
            echo "*** pluginval Level 5: ALL TESTS PASSED ***"
            echo ""

            # If level 5 passes, try level 10
            echo "--- VST3 Validation: Strictness Level 10 (comprehensive) ---"
            PLUGINVAL_LOG10="pluginval_level10_$(date +%Y%m%d_%H%M%S).txt"

            set +e
            "$PLUGINVAL" --validate "$VST3_PATH" \
                --strictness-level 10 \
                --timeout-ms 300000 \
                --verbose 2>&1 | tee "$PLUGINVAL_LOG10"
            PV10_EXIT=${PIPESTATUS[0]}
            set -e

            echo ""
            if [ $PV10_EXIT -eq 0 ]; then
                echo "*** pluginval Level 10: ALL TESTS PASSED ***"
            else
                echo "*** pluginval Level 10: SOME TESTS FAILED ***"
                echo "Check $PLUGINVAL_LOG10 for details."
                VALIDATION_FAILED=1
            fi
        else
            echo "*** pluginval Level 5: TESTS FAILED (exit code: $PV_EXIT) ***"
            echo "Check $PLUGINVAL_LOG for details."
            VALIDATION_FAILED=1
            echo ""
            echo "Common exit codes:"
            echo "  1 = One or more tests failed"
            echo "  2 = Plugin not found or could not load"
            echo "  3 = Timeout exceeded"
        fi
    else
        echo "No VST3 found to validate."
    fi

    # Also validate AU through pluginval if available
    if [ -d "$AU_PATH" ]; then
        echo ""
        echo "--- AU Validation via pluginval (Strictness Level 5) ---"
        PLUGINVAL_AU_LOG="pluginval_au_$(date +%Y%m%d_%H%M%S).txt"

        set +e
        "$PLUGINVAL" --validate "$AU_PATH" \
            --strictness-level 5 \
            --timeout-ms 120000 \
            --verbose 2>&1 | tee "$PLUGINVAL_AU_LOG"
        PV_AU_EXIT=${PIPESTATUS[0]}
        set -e
        echo ""
        if [ $PV_AU_EXIT -eq 0 ]; then
            echo "*** pluginval AU: ALL TESTS PASSED ***"
        else
            echo "*** pluginval AU: TESTS FAILED (exit code: $PV_AU_EXIT) ***"
            echo "Check $PLUGINVAL_AU_LOG for details."
            VALIDATION_FAILED=1
        fi
    fi
fi

echo ""
echo "========================================="
echo "  SUMMARY"
echo "========================================="
echo ""
echo "Log files generated:"
shopt -s nullglob
_log_files=( *_results_*.txt *_level10_*.txt *_au_*.txt )
if [ "${#_log_files[@]}" -eq 0 ]; then
    echo "  (none)"
else
    ls -la "${_log_files[@]}"
fi
shopt -u nullglob
echo ""
echo "NEXT STEPS:"
echo "  1. If everything passed: you are in great shape for release"
echo "  2. If auval failed: the AU format has issues (Logic/GarageBand affected)"
echo "  3. If pluginval failed: copy the log file contents for diagnosis"
echo "  4. Save these logs. They are your validation evidence."
echo ""
if [ "$VALIDATION_FAILED" -ne 0 ]; then
    echo "STATUS: FAILED (see messages above)."
    exit 1
fi
echo "STATUS: OK."
echo "Done."
