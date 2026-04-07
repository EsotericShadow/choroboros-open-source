/*
 * Choroboros - .kzn preset importer
 */

#pragma once

#include <juce_core/juce_core.h>

class ChoroborosAudioProcessor;

namespace choroboros
{

struct KznImportResult
{
    bool success = false;
    juce::String errorMessage;
    juce::String warningMessage;
    juce::String presetName;
    juce::String type;
};

KznImportResult importPresetKzn(ChoroborosAudioProcessor& processor,
                                const juce::File& inputFile);

KznImportResult importEngineKzn(ChoroborosAudioProcessor& processor,
                                const juce::File& inputFile);

KznImportResult importKzn(ChoroborosAudioProcessor& processor,
                          const juce::File& inputFile);

} // namespace choroboros
