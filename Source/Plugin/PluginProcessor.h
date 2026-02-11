#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/ChorusDSP.h"
#include "FeedbackCollector.h"

//==============================================================================
/**
*/
class ChoroborosAudioProcessor  : public juce::AudioProcessor
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
    
    // Feedback collector (public for editor access)
    std::unique_ptr<FeedbackCollector> feedbackCollector;
    
    // Parameter IDs (public for editor access)
    static constexpr const char* RATE_ID = "rate";
    static constexpr const char* DEPTH_ID = "depth";
    static constexpr const char* OFFSET_ID = "offset";
    static constexpr const char* WIDTH_ID = "width";
    static constexpr const char* COLOR_ID = "color"; // Tone/saturation parameter
    static constexpr const char* ENGINE_COLOR_ID = "engineColor"; // 0=Green, 1=Blue, 2=Red, 3=Purple
    static constexpr const char* HQ_ID = "hq"; // Quality toggle
    static constexpr const char* MIX_ID = "mix"; // Dry/wet mix (0.0 = dry, 1.0 = wet)

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    void updateDSPParameters();
    
    std::unique_ptr<ChorusDSP> chorusDSP;
    
    int currentProgram = 0; // Current preset index
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoroborosAudioProcessor)
};
