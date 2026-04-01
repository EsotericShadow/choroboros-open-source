cmake_minimum_required(VERSION 3.22)

project(Choroboros VERSION 2.0.41)

# Version string for feedback/reports.
# Keep this as a normal variable so the checked-in source of truth wins over any
# stale value left behind in an existing CMake cache.
set(CHOROBOROS_VERSION_STRING "Beta v2.04.1")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for IDE language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Look for JUCE in the repo first, then as a sibling directory (common layout
# when multiple repos live under a shared parent like CHOROS_MASTER/).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    set(_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    get_filename_component(_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" ABSOLUTE)
endif()

if(DEFINED _JUCE_DIR)
    message(STATUS "Using local JUCE directory: ${_JUCE_DIR}")
    add_subdirectory("${_JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
    message(STATUS "Local JUCE not found. Fetching JUCE 6.0.8...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        29396c22c93392d6738e021b83196283d6e4d850
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Choroboros
    COMPANY_NAME "Kaizen DSP"
    COMPANY_COPYRIGHT "Copyright (C) 2026 Kaizen Strategic AI Inc. All rights reserved."
    BUNDLE_ID "com.kaizenstrategicai.Choroboros"
    PLUGIN_MANUFACTURER_CODE KzDp
    PLUGIN_CODE ChBr
    FORMATS AU VST3 AAX Standalone
    VST3_CATEGORIES Fx Modulation
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Modulation
    PRODUCT_NAME "Choroboros Beta"
    DESCRIPTION "A chorus that eats its own tail - Five colors, ten algorithms (Beta)"
    VERSION 2.0.41
)

target_sources(Choroboros PRIVATE
    # Plugin core
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Plugin/FeedbackCollector.cpp
    Source/Plugin/FeedbackCollector.h
    Source/Plugin/FeedbackDialog.cpp
    Source/Plugin/FeedbackDialog.h
    Source/Plugin/AboutDialog.cpp
    Source/Plugin/AboutDialog.h
    Source/Plugin/HelpDialog.cpp
    Source/Plugin/HelpDialog.h
    Source/Plugin/PresetManager.cpp
    Source/Plugin/PresetManager.h
    Source/Plugin/PresetState.cpp
    Source/Plugin/PresetState.h
    Source/Plugin/ApplyContext.h
    Source/Plugin/DspConfig.h
    Source/Plugin/DspConfigManager.h
    Source/Plugin/DspConfigManager.cpp
    Source/UI/TopHeaderBar.cpp
    Source/UI/TopHeaderBar.h
    Source/Plugin/SessionLog.cpp
    Source/Plugin/SessionLog.h
    Source/Plugin/CrashReporter.cpp
    Source/Plugin/CrashReporter.h

    # DSP engine
    Source/DSP/ChorusDSP.cpp
    Source/DSP/ChorusDSP.h
    Source/DSP/ChorusDSPPrepare.cpp
    Source/DSP/ChorusDSPPrepare.h
    Source/DSP/ChorusDSPProcess.cpp
    Source/DSP/ChorusDSPProcess.h
    
    # Chorus cores
    Source/DSP/CoreAssignments.h
    Source/Cores/ChorusCore.h
    Source/Cores/green_engine_classic/ChorusCoreLagrange3rd.cpp
    Source/Cores/green_engine_classic/ChorusCoreLagrange3rd.h
    Source/Cores/green_engine_classic/ChorusCoreLagrange5th.cpp
    Source/Cores/green_engine_classic/ChorusCoreLagrange5th.h
    Source/Cores/blue_engine_modern/ChorusCoreCubic.cpp
    Source/Cores/blue_engine_modern/ChorusCoreCubic.h
    Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp
    Source/Cores/blue_engine_modern/ChorusCoreThiran.h
    Source/Cores/red_engine_vintage/ChorusCoreBBD.cpp
    Source/Cores/red_engine_vintage/ChorusCoreBBD.h
    Source/Cores/red_engine_vintage/ChorusCoreTape.cpp
    Source/Cores/red_engine_vintage/ChorusCoreTape.h
    Source/Cores/purple_engine_experimental/ChorusCorePhaseWarped.cpp
    Source/Cores/purple_engine_experimental/ChorusCorePhaseWarped.h
    Source/Cores/purple_engine_experimental/ChorusCoreOrbit.cpp
    Source/Cores/purple_engine_experimental/ChorusCoreOrbit.h
    Source/Cores/black_engine_linear/ChorusCoreLinear.cpp
    Source/Cores/black_engine_linear/ChorusCoreLinear.h
    Source/Cores/black_engine_linear/ChorusCoreLinearEnsemble.cpp
    Source/Cores/black_engine_linear/ChorusCoreLinearEnsemble.h
    
    # Config
    Source/Config/DefaultsPersistence.cpp
    
    # UI components
    Source/UI/CustomLookAndFeel.cpp
    Source/UI/CustomLookAndFeel.h
    Source/UI/LabelWithContainer.cpp
    Source/UI/LabelWithContainer.h
    Source/UI/SmoothedSlider.cpp
    Source/UI/SmoothedSlider.h
    Source/UI/RateSyncOverlay.cpp
    Source/UI/RateSyncOverlay.h
    Source/UI/AnimatedToggleButton.cpp
    Source/UI/AnimatedToggleButton.h
    Source/UI/DevPanel.cpp
    Source/UI/DevPanel.h
    Source/UI/DevPanelBuildContext.h
    Source/UI/DevPanelRuntime.cpp
    Source/UI/DevPanelConsole.cpp
    Source/UI/DevPanelTabBuilders.cpp
    Source/UI/DevPanelPersistence.cpp
    Source/UI/DevPanelSupport.h
    Source/UI/PluginEditorSetup.cpp
    Source/UI/PluginEditorSetup.h
    Source/UI/TopBarDrawer.cpp
    Source/UI/TopBarDrawer.h
)

target_compile_definitions(Choroboros
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Disable JUCE 8's Direct2D renderer on Windows — D3D11/WARP device
        # teardown during DLL unload deadlocks the message thread in Cubase.
        JUCE_DIRECT2D=0
        "CHOROBOROS_VERSION_STRING=\"${CHOROBOROS_VERSION_STRING}\""
)

juce_add_binary_data(ChoroborosBinaryData
    SOURCES
        Assets/green/green_light_off_backpanel.png
        Assets/green/green_light_on_backpanel.png
        Assets/green/rate_knob_spritesheet.png
        Assets/green/depth_knob_spritesheet.png
        Assets/green/offset_knob_spritesheet.png
        Assets/green/width_knob_spritesheet.png
        Assets/green/mix_knob_spritesheet.png
        Assets/green/green_slider_thumb.png
        Assets/blue/blue_light_off_backpanel.png
        Assets/blue/blue_light_on_backpanel.png
        Assets/blue/blue_slider_thumb.png
        Assets/blue/Blue_knob_spritesheet.png
        Assets/blue/blue_mix_knob_spritesheet.png
        Assets/red/red_light_off_backpanel.png
        Assets/red/red_light_on_backpanel.png
        Assets/red/red_rate_knob_spritesheet.png
        Assets/red/red_depth_knob_spritesheet.png
        Assets/red/red_offset_knob_spritesheet.png
        Assets/red/red_width_knob_spritesheet.png
        Assets/red/red_slider_thumb.png
        Assets/red/red_mix_knob_spritesheet.png
        Assets/purple/purple_light_off_backpanel.png
        Assets/purple/purple_light_on_backpanel.png
        Assets/purple/purple_knob_spritesheet.png
        Assets/purple/purple_mix_knob_spritesheet.png
        Assets/purple/purple_slider_thumb.png
        Assets/black/black_light_off_backpanel.png
        Assets/black/black_light_on_backpanel.png
        Assets/black/black_Knob_spritesheet.png
        Assets/black/black_mix_knob_spritesheet.png
        Assets/black/black__slider_thumb.png
        Assets/switch_a_spritesheet.png
        Assets/gui_icons/lock.svg
        Assets/gui_icons/unlock.svg
        Assets/gui_icons/about.png
        Assets/gui_icons/dev.png
        Assets/gui_icons/help.png
        Assets/gui_icons/bug_feedback_button.png
        Assets/gui_icons/kaizen_logo.png
        Kaizen_logo.svg
        Assets/fonts/Technology.ttf
        Assets/fonts/Retroica.ttf
        Assets/fonts/labels/JetBrainsMonoNL-Thin.ttf
        Assets/fonts/labels/JetBrainsMono-ThinItalic.ttf
        Assets/fonts/titles/JetBrainsMono-ExtraBold.ttf
        Assets/fonts/tooltips/JetBrainsMonoNL-ExtraLight.ttf
        Assets/fonts/tooltips/JetBrainsMonoNL-ExtraLightItalic.ttf
        json_defaults_dump.json
        windows/windows_factory_defaults.json
        EULA.md
)

target_link_libraries(Choroboros
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        ChoroborosBinaryData
)

# Regression test harness + build gate
# Run: cmake --build build --target ChoroborosRegressionTests && ./build/ChoroborosRegressionTests
add_executable(ChoroborosRegressionTests Tests/ChoroborosRegressionTests.cpp)
target_include_directories(ChoroborosRegressionTests PRIVATE Source)
target_link_libraries(ChoroborosRegressionTests PRIVATE
    Choroboros
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    ChoroborosBinaryData
)

# JUCE's initialization chain (audio plugin client + AAX) exceeds Windows'
# default 1 MB stack. Raise to 8 MB to prevent STATUS_STACK_OVERFLOW (0xC00000FD)
# on Windows runners and machines.
if(MSVC)
    target_link_options(ChoroborosRegressionTests PRIVATE "/STACK:8388608")
endif()

# Build gate: regression harness
# Run directly: ./build/ChoroborosRegressionTests
# Or: cmake --build build --target ChoroborosRegressionTests && ./build/ChoroborosRegressionTests

# After each VST3 build, install to user Plug-Ins so Reaper loads the latest build
if(APPLE)
    add_custom_command(TARGET Choroboros_VST3 POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - --deep "$<TARGET_BUNDLE_DIR:Choroboros_VST3>"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        COMMAND ${CMAKE_COMMAND} -E echo "Installing Choroboros.vst3 to user VST3 folder..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:Choroboros_VST3>" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
        COMMENT "Copy Choroboros.vst3 to ~/Library/Audio/Plug-Ins/VST3 for Reaper")
    # After AAX build, copy to Avid Plug-Ins (Pro Tools Developer loads from user path if present)
    add_custom_command(TARGET Choroboros_AAX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/Library/Application Support/Avid/Audio/Plug-Ins"
        COMMAND ${CMAKE_COMMAND} -E echo "Installing Choroboros.aaxplugin to Avid Plug-Ins..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{HOME}/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros.aaxplugin"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:Choroboros_AAX>" "$ENV{HOME}/Library/Application Support/Avid/Audio/Plug-Ins/Choroboros.aaxplugin"
        COMMENT "Copy Choroboros.aaxplugin to ~/Library/Application Support/Avid/Audio/Plug-Ins for Pro Tools Developer")
endif()
