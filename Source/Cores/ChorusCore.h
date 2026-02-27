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
    // dsp: Optional pointer for cores that need tuning (e.g. BBD reads bbdStages)
    virtual void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) = 0;
    
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
