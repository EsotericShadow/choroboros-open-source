#pragma once

#include "ChorusDSP.h"

// Helper class for DSP processing
class ChorusDSPProcess
{
public:
    static void processPreEmphasis(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processChorus(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    
private:
    static void processChorusParameters(ChorusDSP& chorusDSP, int blockNumSamples, float& currentDepth, float& currentRate, float& currentCentreDelayMs);
    static void processChorusLFO(ChorusDSP& chorusDSP, int blockNumSamples, int numChannels, float currentRate, float currentDepth);
    static void processChorusDelay(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs);
};
