#pragma once

#include "../ChorusCore.h"
#include <vector>

// Phase-Warped Chorus core
// Purple Normal mode
// Uses warped phase modulation: φw = φ + a*sin(k*φ + b*sin(φ))
class ChorusCorePhaseWarped : public ChorusCore
{
public:
    ChorusCorePhaseWarped();
    ~ChorusCorePhaseWarped() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 2.0f; } // Cubic interpolation needs 2 samples
    float getMaxDelaySamples() const override;
    
private:
    std::vector<std::vector<float>> delayBuffers; // Per-channel circular buffers
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0; // For power-of-2 size
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    // Per-channel phase state
    struct PhaseState
    {
        float phase = 0.0f; // Phase accumulator (0..1)
        float smoothedDelay = 0.0f; // Smoothed delay value
        bool initialized = false;
    };
    std::vector<PhaseState> phaseStates;
    
    // Delay smoothing (10-30ms ramp)
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>> delaySmoothers;
    
    // Cubic interpolation read (same as Blue Normal)
    float readCubic(int channel, float delaySamples) const;
    
    // Compute warped phase modulation
    // Returns modulation signal m in range [-1, 1]
    float computeWarpedModulation(float phase, float warpAmount) const;
};
