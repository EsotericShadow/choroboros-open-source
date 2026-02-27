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

#include "ChorusCoreLagrange3rd.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>

ChorusCoreLagrange3rd::ChorusCoreLagrange3rd()
{
}

void ChorusCoreLagrange3rd::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;
    
    // Calculate maximum delay needed
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
}

void ChorusCoreLagrange3rd::reset()
{
    delayLine.reset();
}

float ChorusCoreLagrange3rd::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreLagrange3rd::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;
    
    // Access LFO buffers from ChorusDSP (friend class)
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            float delaySamp = centreDelaySamples + depthSamples * channelLfo[i];
            delaySamp = juce::jlimit(guardSamples, maxDelaySamples, delaySamp);
            
            const float in = inputSamples[i];
            delayLine.pushSample(ch, in);
            
            const float out = delayLine.popSample(ch, delaySamp, true);
            outputSamples[i] = out;
        }
    }
}
