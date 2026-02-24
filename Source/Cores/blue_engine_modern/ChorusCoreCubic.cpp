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

#include "ChorusCoreCubic.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCoreCubic::ChorusCoreCubic()
{
}

void ChorusCoreCubic::prepare(const juce::dsp::ProcessSpec& processSpec)
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
    
    // Round up to next power of 2 for efficient masking
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 4) // +4 for cubic interpolation
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;
    
    // Allocate buffers for each channel
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
    }
}

void ChorusCoreCubic::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
}

float ChorusCoreCubic::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreCubic::readCubic(int channel, float delaySamples) const
{
    const auto& buf = delayBuffers[static_cast<size_t>(channel)];
    const int writePos = writePositions[static_cast<size_t>(channel)];
    
    // Calculate read position (behind write head)
    float readPos = static_cast<float>(writePos) - delaySamples;
    
    // Wrap to positive range
    while (readPos < 0.0f)
        readPos += static_cast<float>(bufferSize);
    
    // Get integer and fractional parts
    int i1 = static_cast<int>(readPos);
    float u = readPos - static_cast<float>(i1); // Fractional part in [0,1)
    
    // Get indices for 4-point cubic (p_{-1}, p_0, p_{+1}, p_{+2})
    int im1 = (i1 - 1) & bufferMask;
    int i0  = (i1 + 0) & bufferMask;
    int ip1 = (i1 + 1) & bufferMask;
    int ip2 = (i1 + 2) & bufferMask;
    
    // Get samples
    float pm1 = buf[static_cast<size_t>(im1)];
    float p0  = buf[static_cast<size_t>(i0)];
    float p1  = buf[static_cast<size_t>(ip1)];
    float p2  = buf[static_cast<size_t>(ip2)];
    
    // Catmull-Rom cubic weights
    float u2 = u * u;
    float u3 = u2 * u;
    
    float w_m1 = 0.5f * (-u3 + 2.0f * u2 - u);
    float w_0  = 0.5f * ( 3.0f * u3 - 5.0f * u2 + 2.0f);
    float w_1  = 0.5f * (-3.0f * u3 + 4.0f * u2 + u);
    float w_2  = 0.5f * ( u3 - u2);
    
    return w_m1 * pm1 + w_0 * p0 + w_1 * p1 + w_2 * p2;
}

void ChorusCoreCubic::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
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
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            float delaySamp = centreDelaySamples + depthSamples * channelLfo[i];
            delaySamp = juce::jlimit(guardSamples, maxDelaySamples, delaySamp);
            
            const float in = inputSamples[i];
            
            // Write to buffer
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            
            // Read with cubic interpolation
            const float out = readCubic(ch, delaySamp);
            outputSamples[i] = out;
        }
    }
}
