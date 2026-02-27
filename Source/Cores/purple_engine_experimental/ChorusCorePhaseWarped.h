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
    
    void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) override;
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
