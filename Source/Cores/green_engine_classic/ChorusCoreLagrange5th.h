#pragma once

#include "../ChorusCore.h"
#include <vector>

// Lagrange 5th order interpolation chorus core
// Green HQ mode (manual implementation since JUCE doesn't provide it)
class ChorusCoreLagrange5th : public ChorusCore
{
public:
    ChorusCoreLagrange5th();
    ~ChorusCoreLagrange5th() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 3.0f; } // Need 3 samples before and after
    float getMaxDelaySamples() const override;
    
private:
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    // Lagrange 5th order interpolation (6-point)
    float readLagrange5th(int channel, float delaySamples) const;
};
