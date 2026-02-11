#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

// Forward declaration
class ChorusDSP;

// Base interface for chorus delay cores
// Each core implements a different interpolation method
class ChorusCore
{
public:
    virtual ~ChorusCore() = default;
    
    // Prepare the delay line with the given spec
    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    
    // Reset the delay line
    virtual void reset() = 0;
    
    // Process the delay effect
    // dsp: Reference to the main ChorusDSP (for accessing buffers and parameters)
    // block: Audio block to process
    // currentCentreDelayMs: Current centre delay in milliseconds
    virtual void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) = 0;
    
    // Get the guard samples needed for this interpolation type
    virtual float getGuardSamples() const = 0;
    
    // Get the maximum delay in samples (for clamping)
    virtual float getMaxDelaySamples() const = 0;
};
