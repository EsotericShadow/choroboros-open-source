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

#include "ChorusCoreLinearEnsemble.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>

void ChorusCoreLinearEnsemble::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;

    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 4;

    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;

    delayLineA.setMaximumDelayInSamples(maxDelaySamples);
    delayLineB.setMaximumDelayInSamples(maxDelaySamples);
    delayLineA.prepare(spec);
    delayLineB.prepare(spec);

    // ~5ms colour smoothing, ~3ms centre delay smoothing
    colourSmoothAlpha = 1.0f - std::exp(-1.0f / (0.005f * static_cast<float>(spec.sampleRate)));
    centreDelaySmoothAlpha = 1.0f - std::exp(-1.0f / (0.003f * static_cast<float>(spec.sampleRate)));
    smoothedColour.fill(0.0f);
    colourInitialized.fill(false);
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

void ChorusCoreLinearEnsemble::reset()
{
    delayLineA.reset();
    delayLineB.reset();
    smoothedColour.fill(0.0f);
    colourInitialized.fill(false);
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

float ChorusCoreLinearEnsemble::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreLinearEnsemble::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());

    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();
    constexpr float maximumDelayModulation = 20.0f;

    const float* const cdPerMs = dsp.getCentreDelayMsPerSample(blockNumSamples);
    const float centreDelaySamplesBlock = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    const float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    const auto& tuning = dsp.runtimeTuningSnapshot;
    const float targetColour = juce::jlimit(0.0f, 1.0f, dsp.smoothedColor.getCurrentValue());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelSamples = block.getChannelPointer(ch);
        const float* primaryLfo = (ch == 0) ? lfoLeft : lfoRight;
        const float* oppositeLfo = (ch == 0) ? lfoRight : lfoLeft;
        const auto chIdx = static_cast<size_t>(ch);

        if (!colourInitialized[chIdx])
        {
            smoothedColour[chIdx] = targetColour;
            colourInitialized[chIdx] = true;
        }

        if (!centreDelayInitialized[chIdx])
        {
            smoothedCentreDelay[chIdx] = centreDelaySamplesBlock;
            centreDelayInitialized[chIdx] = true;
        }

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float centreMsThis = cdPerMs != nullptr ? cdPerMs[i] : currentCentreDelayMs;
            const float centreDelaySamplesThis = centreMsThis * spec.sampleRate / 1000.0f;
            // Per-sample colour and centre delay smoothing (now per-channel to prevent L/R crosstalk)
            smoothedColour[chIdx] += colourSmoothAlpha * (targetColour - smoothedColour[chIdx]);
            smoothedCentreDelay[chIdx] += centreDelaySmoothAlpha * (centreDelaySamplesThis - smoothedCentreDelay[chIdx]);

            const float col = smoothedColour[chIdx];
            const float centre = smoothedCentreDelay[chIdx];

            const float tap2Mix = juce::jlimit(0.0f, 1.0f,
                tuning.blackHqTap2MixBase + tuning.blackHqTap2MixScale * col);
            const float tap1Mix = 1.0f - tap2Mix;
            const float secondTapDepthScale = juce::jmax(0.0f,
                tuning.blackHqSecondTapDepthBase + tuning.blackHqSecondTapDepthScale * col);
            const float secondTapDelayOffsetSamples = juce::jmax(0.0f,
                tuning.blackHqSecondTapDelayOffsetBase + tuning.blackHqSecondTapDelayOffsetScale * col);

            const float in = channelSamples[i];
            delayLineA.pushSample(ch, in);
            delayLineB.pushSample(ch, in);

            float delayTap1 = centre + depthSamples * primaryLfo[i];
            const float spreadLfo = juce::jmap(col, primaryLfo[i], oppositeLfo[i]);
            float delayTap2 = centre
                            + depthSamples * secondTapDepthScale * spreadLfo
                            + secondTapDelayOffsetSamples;

            delayTap1 = juce::jlimit(guardSamples, maxDelay, delayTap1);
            delayTap2 = juce::jlimit(guardSamples, maxDelay, delayTap2);

            const float wet1 = delayLineA.popSample(ch, delayTap1, true);
            const float wet2 = delayLineB.popSample(ch, delayTap2, true);
            channelSamples[i] = tap1Mix * wet1 + tap2Mix * wet2;
        }
    }
}
