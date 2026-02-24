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

// Tape chorus with varispeed-style modulation:
// fixed center delay + resampled read path (no moving delay tap interpolation).
class ChorusCoreTape : public ChorusCore
{
public:
    ChorusCoreTape();
    ~ChorusCoreTape() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;

    float getGuardSamples() const override { return 4.0f; }
    float getMaxDelaySamples() const override;

private:
    struct ResamplerState
    {
        float phaseOffset = 0.0f;
        float smoothedRatio = 1.0f;
        float smoothedLfoMod = 0.0f;
    };

    struct TapeModState
    {
        float wowPhase = 0.0f;
        float flutterPhase = 0.0f;
        float wowFreq = 0.35f;
        float flutterFreq = 6.0f;
        float wowDepth = 0.0012f;
        float flutterDepth = 0.0004f;
    };

    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    std::vector<ResamplerState> resamplers;
    std::vector<TapeModState> tapeMod;

    // One-pole LP state per channel (no heap allocation in process)
    struct ToneLPState
    {
        float state1 = 0.0f;
        float state2 = 0.0f;
        float cachedG = 0.0f;
        float cachedCutoffHz = -1.0f;
    };
    std::vector<ToneLPState> toneLPState;

    juce::dsp::ProcessSpec spec;
    int bufferSize = 0;
    int bufferMask = 0;
    int maxDelaySamples = 0;
    float currentFixedDelay = -1.0f;
    float smoothedToneCutoff = 14000.0f;

    float resampleHermite(const float* buf, int mask, float realPos, float tension) const;
    float tapeSaturate(float sample, float drive) const;
};
