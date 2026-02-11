#pragma once

#include "../ChorusCore.h"
#include <juce_dsp/juce_dsp.h>

// Lagrange 3rd order interpolation chorus core
class ChorusCoreLagrange3rd : public ChorusCore
{
public:
    ChorusCoreLagrange3rd();
    ~ChorusCoreLagrange3rd() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 2.0f; }
    float getMaxDelaySamples() const override;
    
private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
};
