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

// Orbit Chorus core (2D LFO with rotating axis)
// Purple HQ mode
// Uses 2D oscillator (x, y) projected onto rotating axis
// HQ version uses dual taps for ensemble density
class ChorusCoreOrbit : public ChorusCore
{
public:
    ChorusCoreOrbit();
    ~ChorusCoreOrbit() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
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
    
    // Per-channel orbit state
    struct OrbitState
    {
        float phase = 0.0f;      // Fast phase (rate)
        float theta = 0.0f;      // Slow rotation phase (0.01-0.1 Hz)
        float theta2 = 0.0f;     // Second tap rotation phase (for HQ dual taps)
        float smoothedDelay1 = 0.0f; // Smoothed delay for tap 1
        float smoothedDelay2 = 0.0f; // Smoothed delay for tap 2
        bool initialized = false;
    };
    std::vector<OrbitState> orbitStates;
    
    // Delay smoothing (10-30ms ramp) - one per tap
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>> delaySmoothers1;
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>> delaySmoothers2;
    
    // Cubic interpolation read (same as Blue Normal)
    float readCubic(int channel, float delaySamples) const;
    
    // Compute orbit modulation
    // Returns modulation signal u in range [-1, 1] for given phase, theta, and eccentricity
    float computeOrbitModulation(float phase, float theta, float eccentricity) const;
};
