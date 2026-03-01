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

#include "ChorusCoreLinear.h"
#include "../../DSP/ChorusDSP.h"
#include <algorithm>
#include <cmath>

void ChorusCoreLinear::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
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

    delayLine.setMaximumDelayInSamples(maxDelaySamples);
    delayLine.prepare(spec);

    const float delayGlideMs = 2.5f;
    if (spec.sampleRate > 0.0 && delayGlideMs > 0.0f)
        delaySmoothingAlpha = 1.0f - std::exp(-1.0f / (delayGlideMs * 0.001f * static_cast<float>(spec.sampleRate)));
    else
        delaySmoothingAlpha = 1.0f;
    lastDelayGlideMs = delayGlideMs;

    const auto numChannels = static_cast<size_t>(spec.numChannels);
    smoothedDelaySamplesByChannel.assign(numChannels, 0.0f);
    smoothedDelayInitializedByChannel.assign(numChannels, false);
}

void ChorusCoreLinear::reset()
{
    delayLine.reset();
    std::fill(smoothedDelaySamplesByChannel.begin(), smoothedDelaySamplesByChannel.end(), 0.0f);
    std::fill(smoothedDelayInitializedByChannel.begin(), smoothedDelayInitializedByChannel.end(), false);
    lastDelayGlideMs = -1.0f;
}

float ChorusCoreLinear::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreLinear::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());

    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();
    constexpr float maximumDelayModulation = 20.0f;

    const auto& tuning = dsp.runtimeTuningSnapshot;
    const float colour = juce::jlimit(0.0f, 1.0f, dsp.smoothedColor.getCurrentValue());
    const float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    // In Black normal mode, Color controls modulation intensity.
    const float depthScale = juce::jmax(0.0f, tuning.blackNqDepthBase)
                           + juce::jmax(0.0f, tuning.blackNqDepthScale) * colour;
    const float depthSamples = maximumDelayModulation * depthScale * spec.sampleRate / 1000.0f;

    const float delayGlideMs = juce::jmax(0.0f, tuning.blackNqDelayGlideMs);
    if (std::abs(delayGlideMs - lastDelayGlideMs) > 1.0e-3f)
    {
        if (spec.sampleRate > 0.0 && delayGlideMs > 0.0f)
            delaySmoothingAlpha = 1.0f - std::exp(-1.0f / (delayGlideMs * 0.001f * static_cast<float>(spec.sampleRate)));
        else
            delaySmoothingAlpha = 1.0f;
        lastDelayGlideMs = delayGlideMs;
    }

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = channelSamples[i];
            delayLine.pushSample(ch, in);

            float delaySamp = centreDelaySamples + depthSamples * channelLfo[i];
            if (static_cast<size_t>(ch) < smoothedDelaySamplesByChannel.size())
            {
                if (!smoothedDelayInitializedByChannel[static_cast<size_t>(ch)])
                {
                    smoothedDelaySamplesByChannel[static_cast<size_t>(ch)] = delaySamp;
                    smoothedDelayInitializedByChannel[static_cast<size_t>(ch)] = true;
                }
                else
                {
                    const float previous = smoothedDelaySamplesByChannel[static_cast<size_t>(ch)];
                    smoothedDelaySamplesByChannel[static_cast<size_t>(ch)] = previous + delaySmoothingAlpha * (delaySamp - previous);
                }
                delaySamp = smoothedDelaySamplesByChannel[static_cast<size_t>(ch)];
            }
            delaySamp = juce::jlimit(guardSamples, maxDelay, delaySamp);
            channelSamples[i] = delayLine.popSample(ch, delaySamp, true);
        }
    }
}
