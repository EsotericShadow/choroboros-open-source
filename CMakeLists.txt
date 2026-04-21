cmake_minimum_required(VERSION 3.22)

project(Choroboros VERSION 2.0.50)

# Version string for feedback/reports.
# Keep this as a normal variable so the checked-in source of truth wins over any
# stale value left behind in an existing CMake cache.
set(CHOROBOROS_VERSION_STRING "Beta v2.05")
set(CHOROBOROS_PRODUCT_NAME "Choroboros Beta")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CHOROBOROS_USE_EXTERNAL_ASSET_PACK "Load shared external asset packs before BinaryData fallback" ON)
option(CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK "Allow BinaryData fallback when the shared asset pack is missing" OFF)
set(CHOROBOROS_ASSET_PACK_VERSION "${PROJECT_VERSION}" CACHE STRING "Expected shared asset pack version")

# ---------- Sanitizer support (development only) ----------
# Applied per-target via choroboros_apply_sanitizers() so JUCE juceaide/helpers stay clean.
option(CHOROBOROS_ENABLE_ASAN "Enable AddressSanitizer + LeakSanitizer (Debug-oriented builds)" OFF)
option(CHOROBOROS_ENABLE_TSAN "Enable ThreadSanitizer (mutually exclusive with ASAN)" OFF)

if(CHOROBOROS_ENABLE_ASAN AND CHOROBOROS_ENABLE_TSAN)
    message(FATAL_ERROR "CHOROBOROS_ENABLE_ASAN and CHOROBOROS_ENABLE_TSAN cannot both be ON.")
endif()

if(CHOROBOROS_ENABLE_ASAN)
    message(STATUS "AddressSanitizer ENABLED (shared code, plugin format targets, regression tests)")
endif()

if(CHOROBOROS_ENABLE_TSAN)
    message(STATUS "ThreadSanitizer ENABLED (shared code, plugin format targets, regression tests)")
    if(MSVC)
        message(WARNING "ThreadSanitizer is not supported with MSVC; use Clang on Windows for TSAN.")
    endif()
endif()

# Generate compile_commands.json for IDE language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Look for JUCE in this repo first, then a sibling ../JUCE checkout; otherwise FetchContent.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    set(_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    get_filename_component(_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" ABSOLUTE)
endif()

if(DEFINED _JUCE_DIR)
    message(STATUS "Using local JUCE directory: ${_JUCE_DIR}")
    add_subdirectory("${_JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
    message(STATUS "Local JUCE not found. Fetching JUCE 8.0.12...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        29396c22c93392d6738e021b83196283d6e4d850  # 8.0.12
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------- Melatonin Inspector (development UI debugging) ----------
option(CHOROBOROS_ENABLE_INSPECTOR "Enable Melatonin Inspector for UI debugging" OFF)

if(CHOROBOROS_ENABLE_INSPECTOR)
    message(STATUS "Melatonin Inspector ENABLED")
    include(FetchContent)
    FetchContent_Declare(
        melatonin_inspector
        GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(melatonin_inspector)
endif()

# ---------- Melatonin Perfetto (performance tracing) ----------
option(CHOROBOROS_ENABLE_PERFETTO "Enable Perfetto performance tracing (CMake 3.24+)" OFF)

if(CHOROBOROS_ENABLE_PERFETTO)
    if(CMAKE_VERSION VERSION_LESS "3.24")
        message(FATAL_ERROR "CHOROBOROS_ENABLE_PERFETTO requires CMake 3.24+ (melatonin_perfetto).")
    endif()
    message(STATUS "Melatonin Perfetto ENABLED")
    set(PERFETTO ON CACHE BOOL "Enable Perfetto in melatonin_perfetto module" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        melatonin_perfetto
        GIT_REPOSITORY https://github.com/sudara/melatonin_perfetto.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(melatonin_perfetto)
endif()

set(LIBKZN_BUILD_TESTS OFF CACHE BOOL "Disable libkzn tests" FORCE)
add_subdirectory(libs/libkzn)

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
    PRODUCT_NAME "${CHOROBOROS_PRODUCT_NAME}"
    DESCRIPTION "A chorus that eats its own tail - Five colors, ten algorithms (Beta)"
    VERSION 2.0.50
)

target_sources(Choroboros PRIVATE
    # Plugin core
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Assets/AssetPackVersion.h
    Source/Assets/AssetManifest.cpp
    Source/Assets/AssetManifest.h
    Source/Assets/AssetLocator.cpp
    Source/Assets/AssetLocator.h
    Source/Assets/AssetRepository.cpp
    Source/Assets/AssetRepository.h
    Source/Assets/EmbeddedAssetFallback.cpp
    Source/Assets/EmbeddedAssetFallback.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Plugin/ConsentService.cpp
    Source/Plugin/ConsentService.h
    Source/Plugin/FeedbackCollector.cpp
    Source/Plugin/FeedbackCollector.h
    Source/Plugin/FeedbackDialog.cpp
    Source/Plugin/FeedbackDialog.h
    Source/Plugin/AboutDialog.cpp
    Source/Plugin/AboutDialog.h
    Source/Plugin/ConfirmDialog.cpp
    Source/Plugin/ConfirmDialog.h
    Source/Plugin/HelpDialog.cpp
    Source/Plugin/HelpDialog.h
    Source/Plugin/ManagedDialogStyle.h
    Source/Plugin/MessageDialog.cpp
    Source/Plugin/MessageDialog.h
    Source/Plugin/TextEntryDialog.cpp
    Source/Plugin/TextEntryDialog.h
    Source/Plugin/WindowsRenderPolicy.cpp
    Source/Plugin/WindowsRenderPolicy.h
    Source/Plugin/PresetManager.cpp
    Source/Plugin/PresetManager.h
    Source/Plugin/PresetState.cpp
    Source/Plugin/PresetState.h
    Source/Plugin/ApplyContext.h
    Source/Plugin/DspConfig.h
    Source/Plugin/DspConfigManager.h
    Source/Plugin/DspConfigManager.cpp
    Source/Plugin/ConsoleEngine.cpp
    Source/Plugin/ConsoleEngine.h
    Source/UI/TopHeaderBar.cpp
    Source/UI/TopHeaderBar.h
    Source/Plugin/SessionLog.cpp
    Source/Plugin/SessionLog.h
    Source/Plugin/CrashReporter.cpp
    Source/Plugin/CrashReporter.h
    Source/Engine/CustomEngineManager.cpp
    Source/Engine/CustomEngineManager.h
    Source/KZN/ChoroborosKznSchema.h
    Source/KZN/ChoroborosKznImporter.h
    Source/KZN/ChoroborosKznImporter.cpp

    # DSP engine
    Source/DSP/ChorusDSP.cpp
    Source/DSP/ChorusDSP.h
    Source/DSP/ChorusDSPPrepare.cpp
    Source/DSP/ChorusDSPPrepare.h
    Source/DSP/ChorusDSPProcess.cpp
    Source/DSP/ChorusDSPProcess.h
    Source/DSP/TuningConfigManager.h
    Source/DSP/TuningConfigManager.cpp
    
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
    Source/Config/FactoryDefaults.cpp
    Source/Config/FactoryDefaults.h
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
        "CHOROBOROS_ASSET_PACK_VERSION=\"${CHOROBOROS_ASSET_PACK_VERSION}\""
        CHOROBOROS_USE_EXTERNAL_ASSET_PACK=$<BOOL:${CHOROBOROS_USE_EXTERNAL_ASSET_PACK}>
        CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK=$<BOOL:${CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK}>
)

set(CHOROBOROS_BINARY_DATA_CORE_SOURCES
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
    linux/linux_factory_defaults.json
    windows/windows_factory_defaults.json
    EULA.md
)

set(CHOROBOROS_BINARY_DATA_HEAVY_FALLBACK_SOURCES
    Assets/green/green_light_off_backpanel.png
    Assets/green/green_light_on_backpanel.png
    Assets/green/rate/green_1_off.png
    Assets/green/rate/green_1_on.png
    Assets/green/depth/green_2_off.png
    Assets/green/depth/green_2_on.png
    Assets/green/offset/green_3_off.png
    Assets/green/offset/green_3_on.png
    Assets/green/width/green_4_off.png
    Assets/green/width/green_4_on.png
    Assets/green/green_mix_knob_spritesheet.png
    Assets/green/green_slider_thumb.png
    Assets/blue/blue_light_off_backpanel.png
    Assets/blue/blue_light_on_backpanel.png
    Assets/blue/blue_slider_thumb.png
    Assets/blue/rate/blue_1_off.png
    Assets/blue/rate/blue_1_on.png
    Assets/blue/depth/blue_2_off.png
    Assets/blue/depth/blue_2_on.png
    Assets/blue/offset/blue_3_off.png
    Assets/blue/offset/blue_3_on.png
    Assets/blue/width/blue_4_off.png
    Assets/blue/width/blue_4_on.png
    Assets/blue/blue_mix_knob_spritesheet.png
    Assets/red/red_light_off_backpanel.png
    Assets/red/red_light_on_backpanel.png
    Assets/red/rate/red_1_off.png
    Assets/red/rate/red_1_on.png
    Assets/red/depth/red_2_off.png
    Assets/red/depth/red_2_on.png
    Assets/red/offset/red_3_off.png
    Assets/red/offset/red_3_on.png
    Assets/red/width/red_4_off.png
    Assets/red/width/red_4_on.png
    Assets/red/red_slider_thumb.png
    Assets/red/red_mix_knob_spritesheet.png
    Assets/purple/purple_light_off_backpanel.png
    Assets/purple/purple_light_on_backpanel.png
    Assets/purple/rate/purple_1_off.png
    Assets/purple/rate/purple_1_on.png
    Assets/purple/depth/purple_2_off.png
    Assets/purple/depth/purple_2_on.png
    Assets/purple/offset/purple_3_off.png
    Assets/purple/offset/purple_3_on.png
    Assets/purple/width/purple_4_off.png
    Assets/purple/width/purple_4_on.png
    Assets/purple/purple_mix_knob_spritesheet.png
    Assets/purple/purple_slider_thumb.png
    Assets/black/black_light_off_backpanel.png
    Assets/black/black_light_on_backpanel.png
    Assets/black/rate/black_1_off.png
    Assets/black/rate/black_1_on.png
    Assets/black/depth/black_2_off.png
    Assets/black/depth/black_2_on.png
    Assets/black/offset/black_3_off.png
    Assets/black/offset/black_3_on.png
    Assets/black/width/black_4_off.png
    Assets/black/width/black_4_on.png
    Assets/black/black_mix_knob_spritesheet.png
    Assets/black/black__slider_thumb.png
    Assets/switch_a_spritesheet.png
)

set(CHOROBOROS_BINARY_DATA_SOURCES ${CHOROBOROS_BINARY_DATA_CORE_SOURCES})
if(CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK)
    list(APPEND CHOROBOROS_BINARY_DATA_SOURCES ${CHOROBOROS_BINARY_DATA_HEAVY_FALLBACK_SOURCES})
    message(STATUS "Choroboros: embedded heavy asset fallback enabled")
else()
    message(STATUS "Choroboros: embedded heavy asset fallback disabled; release binaries require the shared asset pack")
endif()

juce_add_binary_data(ChoroborosBinaryData
    SOURCES
        ${CHOROBOROS_BINARY_DATA_SOURCES}
)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_target(ChoroborosAssetPack
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_asset_pack.py
                --source-root ${CMAKE_CURRENT_SOURCE_DIR}/Assets
                --output-root ${CMAKE_CURRENT_BINARY_DIR}/asset-pack
                --pack-version ${CHOROBOROS_ASSET_PACK_VERSION}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Build versioned Choroboros shared asset pack")
endif()

target_link_libraries(Choroboros
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        ChoroborosBinaryData
        libkzn
)

if(CHOROBOROS_ENABLE_INSPECTOR)
    target_link_libraries(Choroboros PRIVATE melatonin_inspector)
    target_compile_definitions(Choroboros PRIVATE CHOROBOROS_INSPECTOR_ENABLED=1)
endif()

if(CHOROBOROS_ENABLE_PERFETTO)
    target_link_libraries(Choroboros PRIVATE melatonin_perfetto)
    target_compile_definitions(Choroboros PRIVATE CHOROBOROS_PERFETTO_ENABLED=1)
endif()

function(choroboros_apply_sanitizers target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()
    if(CHOROBOROS_ENABLE_ASAN)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()
    endif()
    if(CHOROBOROS_ENABLE_TSAN AND NOT MSVC)
        target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()
endfunction()

choroboros_apply_sanitizers(Choroboros)
# JUCE format targets perform the final link; they must carry sanitizer link flags
# or instrumented SharedCode leaves undefined ___asan_* symbols.
foreach(_choroboros_fmt Choroboros_Standalone Choroboros_VST3 Choroboros_AU Choroboros_AAX)
    if(TARGET ${_choroboros_fmt})
        choroboros_apply_sanitizers(${_choroboros_fmt})
    endif()
endforeach()

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

choroboros_apply_sanitizers(ChoroborosRegressionTests)

# ---------- DSP Chain Topology Tester ----------
# Isolated simulation environment that processes audio through the real DSP chain
# with measurement taps at every stage. Use to diagnose gain staging, verify the
# safety limiter proposal, and stress-test presets for clipping.
#
# Build & run:
#   cmake --build build --target ChoroborosDspChainTest
#   ./build/ChoroborosDspChainTest                          # Psychedelic preset
#   ./build/ChoroborosDspChainTest --limiter                # with safety limiter
#   ./build/ChoroborosDspChainTest --all                    # all presets comparison
#   ./build/ChoroborosDspChainTest --preset MaxEverything   # stress test
add_executable(ChoroborosDspChainTest Tests/DspChainTest.cpp)
target_include_directories(ChoroborosDspChainTest PRIVATE Source)
target_link_libraries(ChoroborosDspChainTest PRIVATE
    Choroboros
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    ChoroborosBinaryData
)
choroboros_apply_sanitizers(ChoroborosDspChainTest)

# ---------- Per-Core DSP Benchmark ----------
# Isolated test harness that exercises each of the 10 chorus cores independently,
# measuring: peak/true-peak, NaN/Inf, noise floor, stereo balance, CPU time,
# and optionally sweeps all param combos for worst-case headroom.
#
# Build & run:
#   cmake --build build --target ChoroborosCoreTest
#   ./build/ChoroborosCoreTest --all                     # all 10 cores
#   ./build/ChoroborosCoreTest --all --sweep             # + 81-combo param sweep each
#   ./build/ChoroborosCoreTest --core Blue_HQ --verbose  # single core, detailed
add_executable(ChoroborosCoreTest Tests/CoreBenchmark.cpp)
target_include_directories(ChoroborosCoreTest PRIVATE Source)
target_link_libraries(ChoroborosCoreTest PRIVATE
    Choroboros
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    ChoroborosBinaryData
)
choroboros_apply_sanitizers(ChoroborosCoreTest)

# Isolated knob physics lab (3×3 green knobs, compare drag curves)
#   cmake --build build --target ChoroborosKnobPhysicsLab && ./build/ChoroborosKnobPhysicsLab
add_executable(ChoroborosKnobPhysicsLab
    Tests/KnobPhysicsLabMain.cpp
    Source/UI/KnobPhysicsLabSlider.cpp
)
target_include_directories(ChoroborosKnobPhysicsLab PRIVATE Source)
target_link_libraries(ChoroborosKnobPhysicsLab PRIVATE
    Choroboros
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    ChoroborosBinaryData
)
choroboros_apply_sanitizers(ChoroborosKnobPhysicsLab)

# JUCE's initialization chain (audio plugin client + AAX) exceeds Windows'
# default 1 MB stack. Raise to 8 MB to prevent STATUS_STACK_OVERFLOW (0xC00000FD)
# on Windows runners and machines.
if(MSVC)
    target_link_options(ChoroborosRegressionTests PRIVATE "/STACK:8388608")
    target_link_options(ChoroborosDspChainTest PRIVATE "/STACK:8388608")
    target_link_options(ChoroborosCoreTest PRIVATE "/STACK:8388608")
    target_link_options(ChoroborosKnobPhysicsLab PRIVATE "/STACK:8388608")
endif()

# Build gate: regression harness
# Run directly: ./build/ChoroborosRegressionTests
# Or: cmake --build build --target ChoroborosRegressionTests && ./build/ChoroborosRegressionTests

# After each VST3 build, install to user Plug-Ins so Reaper loads the latest build
if(APPLE)
    add_custom_command(TARGET Choroboros_VST3 POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - --deep "$<TARGET_BUNDLE_DIR:Choroboros_VST3>"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        COMMAND ${CMAKE_COMMAND} -E echo "Installing ${CHOROBOROS_PRODUCT_NAME}.vst3 to user VST3 folder..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/${CHOROBOROS_PRODUCT_NAME}.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:Choroboros_VST3>" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/${CHOROBOROS_PRODUCT_NAME}.vst3"
        COMMENT "Copy ${CHOROBOROS_PRODUCT_NAME}.vst3 to ~/Library/Audio/Plug-Ins/VST3 for local host testing")
    # Do not auto-copy AAX after build. Pro Tools scans the system Avid folder,
    # which requires an explicit install step and should stay aligned with the
    # signed installer / install_aax_pro_tools_system.sh workflow.
endif()

# Release hygiene: mac / linux / windows / dump factory JSON must stay byte-identical
# to avoid cross-build behavior drift. Run: cmake --build build --target verify_factory_json_sync
if(UNIX)
    add_custom_target(verify_factory_json_sync
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_factory_json_sync.sh"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Verify factory defaults JSON copies match Assets/defaults_factory_mac.json")
endif()
