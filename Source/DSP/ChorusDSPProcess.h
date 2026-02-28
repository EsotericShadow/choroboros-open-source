/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "ChorusDSP.h"

// Helper class for DSP processing
class ChorusDSPProcess
{
public:
    static void processPreEmphasis(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processPreChorusSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processWetCharacter(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processPostChorusSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    static void processChorus(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block);
    
private:
    static void processChorusParameters(ChorusDSP& chorusDSP, int blockNumSamples, float& currentDepth, float& currentRate, float& currentCentreDelayMs);
    static void processChorusLFO(ChorusDSP& chorusDSP, int blockNumSamples, int numChannels, float currentRate, float currentDepth);
    static void processChorusDelay(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs);
};
