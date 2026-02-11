#pragma once

#include "../ChorusCore.h"
#include <vector>

// Cubic (Catmull-Rom) interpolation chorus core
// Blue Normal mode
class ChorusCoreCubic : public ChorusCore
{
public:
    ChorusCoreCubic();
    ~ChorusCoreCubic() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 2.0f; } // Need 2 samples before and after
    float getMaxDelaySamples() const override;
    
private:
    std::vector<std::vector<float>> delayBuffers; // Per-channel circular buffers
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0; // For power-of-2 size
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    // Cubic interpolation read
    float readCubic(int channel, float delaySamples) const;
};
