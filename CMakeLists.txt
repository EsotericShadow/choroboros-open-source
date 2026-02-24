cmake_minimum_required(VERSION 3.22)

project(Choroboros VERSION 1.0.1)

# Beta version string for feedback/reports (change to "2.01" for release)
set(CHOROBOROS_VERSION_STRING "2.01-beta" CACHE STRING "Version string for feedback and display")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for IDE language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(JUCE)

juce_add_plugin(Choroboros
    COMPANY_NAME "Kaizen Strategic AI"
    COMPANY_COPYRIGHT "Copyright (C) 2026 Kaizen Strategic AI Inc. All rights reserved."
    BUNDLE_ID "com.kaizenstrategicai.Choroboros"
    PLUGIN_MANUFACTURER_CODE ChBr
    PLUGIN_CODE ChBr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Choroboros"
    DESCRIPTION "A chorus that eats its own tail - Five colors, ten algorithms"
    VERSION 1.0.1
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
    
    # DSP engine
    Source/DSP/ChorusDSP.cpp
    Source/DSP/ChorusDSP.h
    Source/DSP/ChorusDSPPrepare.cpp
    Source/DSP/ChorusDSPPrepare.h
    Source/DSP/ChorusDSPProcess.cpp
    Source/DSP/ChorusDSPProcess.h
    
    # Chorus cores
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
    Source/UI/AnimatedToggleButton.cpp
    Source/UI/AnimatedToggleButton.h
    Source/UI/DevPanel.cpp
    Source/UI/DevPanel.h
    Source/UI/PluginEditorSetup.cpp
    Source/UI/PluginEditorSetup.h
)

target_compile_definitions(Choroboros
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        "CHOROBOROS_VERSION_STRING=\"${CHOROBOROS_VERSION_STRING}\""
)

juce_add_binary_data(ChoroborosBinaryData
    SOURCES
        Assets/green/green_backpanel.png
        Assets/green/rate_knob_spritesheet.png
        Assets/green/depth_knob_spritesheet.png
        Assets/green/offset_knob_spritesheet.png
        Assets/green/width_knob_spritesheet.png
        Assets/green/mix_knob_spritesheet.png
        Assets/green/green_slider_thumb.png
        Assets/blue/blue_backpanel.png
        Assets/blue/blue_slider_thumb.png
        Assets/blue/Blue_knob_spritesheet.png
        Assets/blue/blue_knob_base.png
        Assets/blue/blue_mix_knob_spritesheet.png
        Assets/red/red_backpanel.png
        Assets/red/red_dragon_knob.png
        Assets/red/red_knob_base.png
        Assets/red/red_slider_thumb.png
        Assets/red/red_mix_knob_spritesheet.png
        Assets/purple/purple_backpanel.png
        Assets/purple/purple_knob_spritesheet.png
        Assets/purple/purple_knob_base.png
        Assets/purple/purple_mix_knob_spritesheet.png
        Assets/purple/purple_slider_thumb.png
        Assets/blue/blue_mix_knob.png
        Assets/blue/indicator_north_facing.png
        Assets/blue/indicator_shadow_overlay.png
        Assets/red/red_mix_knob.png
        Assets/red/worn_indicator_north_facing.png
        Assets/red/indicator_shadow_overlay.png
        Assets/purple/purple_mix_knob.png
        Assets/purple/indicator_north_facing.png
        Assets/purple/indicator_shadow_overlay.png
        Assets/black/black_backpanel.png
        Assets/black/black_knob_base.png
        Assets/black/black_Knob_spritesheet.png
        Assets/black/black_mix_knob_spritesheet.png
        Assets/black/black__slider_thumb.png
        Assets/switch_a_spritesheet.png
        Assets/fonts/Technology.ttf
        Assets/fonts/Retroica.ttf
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

# Build gate: regression harness
# Run directly: ./build/ChoroborosRegressionTests
# Or: cmake --build build --target ChoroborosRegressionTests && ./build/ChoroborosRegressionTests

# After each VST3 build, install to user Plug-Ins so Reaper loads the latest build
if(APPLE)
    add_custom_command(TARGET Choroboros_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Installing Choroboros.vst3 to user VST3 folder..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/Choroboros_artefacts/Release/VST3/Choroboros.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Choroboros.vst3"
        COMMENT "Copy Choroboros.vst3 to ~/Library/Audio/Plug-Ins/VST3 for Reaper")
endif()
