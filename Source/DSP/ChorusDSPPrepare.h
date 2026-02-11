#pragma once

#include "ChorusDSP.h"

// Helper class for DSP preparation
class ChorusDSPPrepare
{
public:
    // Note: Delay lines are now prepared by ChorusCore implementations
    static void prepareLFOs(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec);
    static void prepareFilters(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec);
    static void prepareBuffers(ChorusDSP& chorusDSP, const juce::dsp::ProcessSpec& spec);
};
