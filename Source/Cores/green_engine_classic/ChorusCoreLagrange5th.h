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

// Lagrange 5th order interpolation chorus core
// Green HQ mode (manual implementation since JUCE doesn't provide it)
class ChorusCoreLagrange5th : public ChorusCore
{
public:
    ChorusCoreLagrange5th();
    ~ChorusCoreLagrange5th() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 3.0f; } // Need 3 samples before and after
    float getMaxDelaySamples() const override;
    
private:
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    // Lagrange 5th order interpolation (6-point)
    float readLagrange5th(int channel, float delaySamples) const;
};
