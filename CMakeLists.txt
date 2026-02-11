cmake_minimum_required(VERSION 3.22)

project(Choroboros VERSION 1.0.1)

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
    DESCRIPTION "A chorus that eats its own tail - Four colors, eight algorithms"
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
    
    # UI components
    Source/UI/CustomLookAndFeel.cpp
    Source/UI/CustomLookAndFeel.h
    Source/UI/LabelWithContainer.cpp
    Source/UI/LabelWithContainer.h
    Source/UI/SmoothedSlider.cpp
    Source/UI/SmoothedSlider.h
    Source/UI/AnimatedToggleButton.cpp
    Source/UI/AnimatedToggleButton.h
    Source/UI/PluginEditorSetup.cpp
    Source/UI/PluginEditorSetup.h
)

target_compile_definitions(Choroboros
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(ChoroborosBinaryData
    SOURCES
        Assets/green/green_panel_background.png
        Assets/green/green_knob_base.png
        Assets/green/green_slider_track.png
        Assets/green/green_slider_thumb.png
        Assets/blue/blue_panel_background.png
        Assets/blue/blue_knob_base.png
        Assets/blue/blue_slider_track.png
        Assets/blue/blue_slider_thumb.png
        Assets/red/red_panel_background.png
        Assets/red/red_knob_base.png
        Assets/red/red_slider_track.png
        Assets/red/red_slider_thumb.png
        Assets/purple/purple_panel_background.png
        Assets/purple/purple_knob_base.png
        Assets/purple/purple_slider_track.png
        Assets/purple/purple_slider_thumb.png
        Assets/green/green_mix_knob.png
        Assets/blue/blue_mix_knob.png
        Assets/red/red_mix_knob.png
        Assets/purple/purple_mix_knob.png
        EULA.md
)

target_link_libraries(Choroboros
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        ChoroborosBinaryData
)
