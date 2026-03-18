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
#include <juce_dsp/juce_dsp.h>

// Lagrange 3rd order interpolation chorus core
class ChorusCoreLagrange3rd : public ChorusCore
{
public:
    ChorusCoreLagrange3rd();
    ~ChorusCoreLagrange3rd() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 2.0f; }
    float getMaxDelaySamples() const override;
    
private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
};
