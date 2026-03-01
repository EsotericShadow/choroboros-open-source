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

#include "ChorusCorePhaseWarped.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCorePhaseWarped::ChorusCorePhaseWarped()
{
}

void ChorusCorePhaseWarped::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP* dsp)
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
    phaseStates.resize(static_cast<size_t>(spec.numChannels));
    delaySmoothers.resize(static_cast<size_t>(spec.numChannels));

    const float delaySmoothingMs = (dsp != nullptr)
        ? juce::jmax(0.0f, dsp->getRuntimeTuning().purpleWarpDelaySmoothingMs.load())
        : 20.0f;
    const float delaySmoothingSec = delaySmoothingMs * 0.001f;
    lastDelaySmoothingMs = delaySmoothingMs;
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        phaseStates[ch].phase = 0.0f;
        phaseStates[ch].smoothedDelay = 0.0f;
        phaseStates[ch].initialized = false;
        
        delaySmoothers[ch].reset(spec.sampleRate, delaySmoothingSec);
        delaySmoothers[ch].setCurrentAndTargetValue(0.0f);
    }
}

void ChorusCorePhaseWarped::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    
    for (size_t ch = 0; ch < phaseStates.size(); ++ch)
    {
        phaseStates[ch].phase = 0.0f;
        phaseStates[ch].smoothedDelay = 0.0f;
        phaseStates[ch].initialized = false;
        delaySmoothers[ch].setCurrentAndTargetValue(0.0f);
    }
    lastDelaySmoothingMs = -1.0f;
}

float ChorusCorePhaseWarped::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCorePhaseWarped::computeWarpedModulation(float phase, float a, float b, float k) const
{
    // Convert phase to radians (0..2π)
    const float phi = phase * juce::MathConstants<float>::twoPi;
    
    // Compute warped phase: φw = φ + a*sin(k*φ + b*sin(φ))
    const float warpedPhase = phi + a * std::sin(k * phi + b * std::sin(phi));
    
    // Modulation signal: m = sin(φw)
    return std::sin(warpedPhase);
}

float ChorusCorePhaseWarped::readCubic(int channel, float delaySamples) const
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

void ChorusCorePhaseWarped::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;
    
    // Get smoothed rate and color (computed once per block)
    float currentRate = dsp.smoothedRate.getCurrentValue();
    float currentColor = dsp.smoothedColor.getCurrentValue();
    
    // Phase increment per sample
    float phaseInc = currentRate / spec.sampleRate;
    
    const auto& tuning = dsp.runtimeTuningSnapshot;
    const float warpAmount = juce::jlimit(0.0f, 1.0f, currentColor);
    const float warpA = juce::jmax(0.0f, tuning.purpleWarpA) * warpAmount;
    const float warpB = juce::jmax(0.0f, tuning.purpleWarpB) * warpAmount;
    const float warpK = juce::jmax(0.1f, tuning.purpleWarpKBase + tuning.purpleWarpKScale * warpAmount);

    const float delaySmoothingMs = juce::jmax(0.0f, tuning.purpleWarpDelaySmoothingMs);
    if (std::abs(delaySmoothingMs - lastDelaySmoothingMs) > 1.0e-3f)
    {
        const float delaySmoothingSec = delaySmoothingMs * 0.001f;
        for (size_t ch = 0; ch < delaySmoothers.size(); ++ch)
        {
            const float current = delaySmoothers[ch].getCurrentValue();
            delaySmoothers[ch].reset(spec.sampleRate, delaySmoothingSec);
            delaySmoothers[ch].setCurrentAndTargetValue(current);
        }
        lastDelaySmoothingMs = delaySmoothingMs;
    }
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        auto& state = phaseStates[static_cast<size_t>(ch)];
        auto& delaySmoother = delaySmoothers[static_cast<size_t>(ch)];
        
        // Initialize delay on first sample if needed
        if (!state.initialized)
        {
            float initialMod = computeWarpedModulation(state.phase, warpA, warpB, warpK);
            float initialDelay = centreDelaySamples + depthSamples * initialMod;
            initialDelay = juce::jlimit(guardSamples, maxDelaySamples, initialDelay);
            state.smoothedDelay = initialDelay;
            delaySmoother.setCurrentAndTargetValue(initialDelay);
            state.initialized = true;
        }
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];
            
            // Write to buffer
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            
            // Advance phase
            state.phase += phaseInc;
            if (state.phase >= 1.0f)
                state.phase -= 1.0f;
            
            // Compute warped modulation
            float mod = computeWarpedModulation(state.phase, warpA, warpB, warpK);
            
            // Calculate target delay
            float targetDelay = centreDelaySamples + depthSamples * mod;
            targetDelay = juce::jlimit(guardSamples, maxDelaySamples, targetDelay);
            
            // Smooth delay (20ms ramp)
            delaySmoother.setTargetValue(targetDelay);
            float delaySamp = delaySmoother.getNextValue();
            
            // Read with cubic interpolation
            const float out = readCubic(ch, delaySamp);
            outputSamples[i] = out;
        }
    }
}
