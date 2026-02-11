#pragma once

#include "../ChorusCore.h"
#include <vector>

// Bucket-Brigade Device (BBD) emulation chorus core
// Red Normal mode
class ChorusCoreBBD : public ChorusCore
{
public:
    ChorusCoreBBD();
    ~ChorusCoreBBD() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 1.0f; }
    float getMaxDelaySamples() const override;
    
private:
    static constexpr int BBD_STAGES = 1024; // Number of BBD stages
    
    struct BBDChannel
    {
        std::vector<float> stages; // BBD stage buffer
        int head = 0;
        double clockPhase = 0.0;
        float heldOutput = 0.0f;
        float heldPrev = 0.0f;  // Previous held output for time interpolation
        float heldNext = 0.0f;  // Next held output for time interpolation
        
        // One-pole lowpass filters (no coefficient swapping, click-free)
        float inputLPState = 0.0f;   // One-pole filter state
        float outputLPState = 0.0f;  // One-pole filter state
        
        // Smoothed delay and clock frequency to prevent crackling
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelayMs; // Smooth delay changes
        float smoothedClockFreq = 5000.0f;
        float smoothedFilterCutoff = 5000.0f; // Smoothly interpolated filter cutoff
    };
    
    std::vector<BBDChannel> channels;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    // Process one channel's BBD
    float processBBDChannel(int channel, float input, float clockFreq);
};
