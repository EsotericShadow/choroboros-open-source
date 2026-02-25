/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "../DSP/ChorusDSP.h"
#include "FeedbackCollector.h"

//==============================================================================
/**
*/
/** Per-engine user modifier profile (Rate, Depth, Offset, Width, Mix, Color). HQ excluded. */
struct EngineParamProfile
{
    bool valid = false;
    float rate = 0.5f;
    float depth = 0.5f;
    float offset = 90.0f;
    float width = 1.0f;
    float mix = 0.5f;
    float color = 0.5f;
};

class ChoroborosAudioProcessor  : public juce::AudioProcessor,
                                  private juce::Timer,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    ChoroborosAudioProcessor();
    ~ChoroborosAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    struct ParamTuning
    {
        std::atomic<float> min { 0.0f };
        std::atomic<float> max { 1.0f };
        std::atomic<float> curve { 1.0f };
        std::atomic<float> uiSkew { 1.0f };
    };

    struct TuningState
    {
        ParamTuning rate;
        ParamTuning depth;
        ParamTuning offset;
        ParamTuning width;
        ParamTuning color;
        ParamTuning mix;
    };

    const TuningState& getTuningState() const { return tuning; }
    TuningState& getTuningState() { return tuning; }
    ChorusDSP::RuntimeTuning& getDspInternals() { return chorusDSP->getRuntimeTuning(); }
    const ChorusDSP::RuntimeTuning& getDspInternals() const { return chorusDSP->getRuntimeTuning(); }
    ChorusDSP::RuntimeTuning& getEngineDspInternals(int colorIndex, bool hqEnabled = false);
    const ChorusDSP::RuntimeTuning& getEngineDspInternals(int colorIndex, bool hqEnabled = false) const;
    int getCurrentEngineColorIndex() const;
    bool isHqEnabled() const;
    const std::array<EngineParamProfile, 5>& getEngineParamProfiles() const { return engineParamProfiles; }
    void loadEngineParamProfilesFromVar(const juce::var& profilesVar);
    void syncEngineInternalsToActiveDsp(int colorIndex, bool hqEnabled = false);
    float mapParameterValue(const juce::String& paramId, float rawValue) const;
    
    // Feedback collector (public for editor access)
    std::unique_ptr<FeedbackCollector> feedbackCollector;
    
    // Parameter IDs (public for editor access)
    static constexpr const char* RATE_ID = "rate";
    static constexpr const char* DEPTH_ID = "depth";
    static constexpr const char* OFFSET_ID = "offset";
    static constexpr const char* WIDTH_ID = "width";
    static constexpr const char* COLOR_ID = "color"; // Tone/saturation parameter
    static constexpr const char* ENGINE_COLOR_ID = "engineColor"; // 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    static constexpr const char* HQ_ID = "hq"; // Quality toggle
    static constexpr const char* MIX_ID = "mix"; // Dry/wet mix (0.0 = dry, 1.0 = wet)

    static constexpr float RATE_MIN = 0.01f;
    static constexpr float RATE_MAX = 10.0f;
    static constexpr float DEPTH_MIN = 0.0f;
    static constexpr float DEPTH_MAX = 1.0f;
    static constexpr float OFFSET_MIN = 0.0f;
    static constexpr float OFFSET_MAX = 180.0f;
    static constexpr float WIDTH_MIN = 0.0f;
    static constexpr float WIDTH_MAX = 2.0f;
    static constexpr float COLOR_MIN = 0.0f;
    static constexpr float COLOR_MAX = 1.0f;
    static constexpr float MIX_MIN = 0.0f;
    static constexpr float MIX_MAX = 1.0f;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    void updateDSPParameters();
    void timerCallback() override;

    void initTuningDefaults();
    float mapTunedValue(float rawValue, float baseMin, float baseMax, const ParamTuning& tuning) const;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void applyEngineParamProfile(int engineIndex);
    void saveCurrentParamsToEngineProfile(int engineIndex);
    static EngineParamProfile getEngineDefaults(int engineIndex);
    void initializeEngineInternalProfiles();
    void persistActiveEngineInternalsFromDsp();
    void restoreEngineInternalsToDsp(int colorIndex, bool hqEnabled);
    
    juce::CriticalSection dspLock;
    std::unique_ptr<ChorusDSP> chorusDSP;
    TuningState tuning;
    std::array<std::array<ChorusDSP::RuntimeTuning, 2>, 5> engineInternals;
    int activeInternalsEngine = 0;
    bool activeInternalsHQ = false;
    
    int currentProgram = 0; // Current preset index
    std::array<EngineParamProfile, 5> engineParamProfiles;
    int lastEngineIndex = 0;
    std::atomic<bool> presetLoadInProgress { false };
    std::atomic<bool> stateLoadInProgress { false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoroborosAudioProcessor)
};
