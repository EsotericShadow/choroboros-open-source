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

#include "ChorusCoreBBD.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCoreBBD::ChorusCoreBBD()
{
}

void ChorusCoreBBD::prepare(const juce::dsp::ProcessSpec& processSpec)
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
    
    // Allocate channels
    channels.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& chan = channels[ch];
        // Fix #1: Allocate full BBD_STAGES (1024) to match clock formula
        chan.stages.assign(BBD_STAGES, 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldOutput = 0.0f;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        
        // Initialize one-pole filter states
        chan.inputLPState = 0.0f;
        chan.inputLPState2 = 0.0f;
        chan.outputLPState = 0.0f;
        chan.outputLPState2 = 0.0f;
        
        // Keep delay responsive enough to feel "chorus-like" while avoiding zipper.
        chan.smoothedDelayMs.reset(spec.sampleRate, 0.02f); // 20ms ramp
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f); // Default 20ms
    }
}

void ChorusCoreBBD::reset()
{
    for (auto& chan : channels)
    {
        std::fill(chan.stages.begin(), chan.stages.end(), 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldOutput = 0.0f;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        chan.inputLPState = 0.0f;
        chan.inputLPState2 = 0.0f;
        chan.outputLPState = 0.0f;
        chan.outputLPState2 = 0.0f;
        chan.smoothedClockFreq = 5000.0f;
        chan.smoothedFilterCutoff = 5000.0f;
        chan.cachedCutoffForG = -1.0f;
        chan.cachedOnePoleG = 0.0f;
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f);
    }
}

float ChorusCoreBBD::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreBBD::processBBDChannel(int channel, float input, float clockFreq, float clockSmoothCoeff,
                                       float filterSmoothCoeff, float filterCutoffScale,
                                       float filterCutoffMinHz, float filterCutoffMaxHz)
{
    auto& chan = channels[static_cast<size_t>(channel)];
    
    // Smooth clock frequency changes to prevent crackling
    chan.smoothedClockFreq = clockSmoothCoeff * chan.smoothedClockFreq + (1.0f - clockSmoothCoeff) * clockFreq;
    const float smoothClockFreq = chan.smoothedClockFreq;
    
    // Calculate filter cutoff from clock frequency.
    // A slightly wider range keeps the BBD character but avoids overly muffled output.
    float targetCutoffRaw = smoothClockFreq * filterCutoffScale;
    float targetCutoff = juce::jlimit(filterCutoffMinHz, filterCutoffMaxHz, targetCutoffRaw);
    chan.smoothedFilterCutoff = filterSmoothCoeff * chan.smoothedFilterCutoff + (1.0f - filterSmoothCoeff) * targetCutoff;
    
    // Use a cascaded one-pole LPF (two poles total) for stronger anti-alias/reconstruction.
    // One-pole: y[n] = g * y[n-1] + (1-g) * x[n], where g = exp(-2π * cutoff / sampleRate)
    const float maxSafeCutoffHz = 0.22f * static_cast<float>(spec.sampleRate);
    const float effectiveCutoffHz = juce::jlimit(20.0f, maxSafeCutoffHz, chan.smoothedFilterCutoff);
    constexpr float cutoffRecomputeThresholdHz = 0.5f;
    if (std::abs(effectiveCutoffHz - chan.cachedCutoffForG) > cutoffRecomputeThresholdHz || chan.cachedCutoffForG < 0.0f)
    {
        chan.cachedOnePoleG = std::exp(-2.0f * juce::MathConstants<float>::pi * effectiveCutoffHz / static_cast<float>(spec.sampleRate));
        chan.cachedCutoffForG = effectiveCutoffHz;
    }
    const float g = chan.cachedOnePoleG;
    
    // Input filter (anti-aliasing before BBD)
    chan.inputLPState = g * chan.inputLPState + (1.0f - g) * input;
    chan.inputLPState2 = g * chan.inputLPState2 + (1.0f - g) * chan.inputLPState;
    float filteredInput = chan.inputLPState2;
    
    // Fix #1: Use consistent stage count - tap at BBD_STAGES/2 (512)
    const int delayStages = BBD_STAGES / 2;
    
    chan.clockPhase += static_cast<double>(smoothClockFreq) / static_cast<double>(spec.sampleRate);
    
    // Fix #2: Time interpolation - update held outputs on clock ticks
    // Handle multiple ticks per sample if clock > sample rate
    while (chan.clockPhase >= 1.0)
    {
        chan.clockPhase -= 1.0;
        
        // Shift register: write input to current head
        chan.stages[static_cast<size_t>(chan.head)] = filteredInput;
        chan.head = (chan.head + 1) % static_cast<int>(chan.stages.size());
        
        // Read from position offset by delayStages (BBD delay = N/2 stages)
        int readPos = (chan.head - delayStages + static_cast<int>(chan.stages.size())) % static_cast<int>(chan.stages.size());
        
        // Update held outputs for time interpolation
        chan.heldPrev = chan.heldNext;
        chan.heldNext = chan.stages[static_cast<size_t>(readPos)];
    }
    
    // Interpolate between held outputs over time (not across stages)
    // This removes bitcrush artifacts by smoothing the sample-and-hold staircase
    float t = static_cast<float>(chan.clockPhase);  // 0..1 phase within current tick
    float held = chan.heldPrev + t * (chan.heldNext - chan.heldPrev);
    
    // Output filter (reconstruction after BBD) - reuse same g from input filter
    chan.outputLPState = g * chan.outputLPState + (1.0f - g) * held;
    chan.outputLPState2 = g * chan.outputLPState2 + (1.0f - g) * chan.outputLPState;

    return chan.outputLPState2;
}

void ChorusCoreBBD::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    const auto& tuning = dsp.runtimeTuningSnapshot;
    
    // CRITICAL FIX: LFO is already scaled by depth*0.5 in ChorusDSP
    // So channelLfo values are in range [-depth*0.5, +depth*0.5]
    // Don't multiply by 0.5 again - use full modulation range
    // Red BBD needs a longer operating delay than the global center-delay rule to stay in chorus
    // territory (instead of drifting into flanger/phaser-like combing).
    const float remappedCentreDelayMs = tuning.bbdCentreBaseMs + (currentCentreDelayMs - 8.0f) * tuning.bbdCentreScale;
    const float depthMs = tuning.bbdDepthMs; // LFO already has depth scaling, final swing is depth-dependent

    const float delaySmoothingMs = juce::jmax(0.0f, tuning.bbdDelaySmoothingMs);
    if (delaySmoothingMs != lastDelaySmoothingMs)
    {
        const float rampSeconds = delaySmoothingMs * 0.001f;
        for (auto& chan : channels)
        {
            const float current = chan.smoothedDelayMs.getCurrentValue();
            chan.smoothedDelayMs.reset(spec.sampleRate, rampSeconds);
            chan.smoothedDelayMs.setCurrentAndTargetValue(current);
        }
        lastDelaySmoothingMs = delaySmoothingMs;
    }

    const float clockSmoothMs = juce::jmax(0.001f, tuning.bbdClockSmoothingMs);
    const float filterSmoothMs = juce::jmax(0.001f, tuning.bbdFilterSmoothingMs);
    const float clockSmoothCoeff = std::exp(-1.0f / (clockSmoothMs * 0.001f * static_cast<float>(spec.sampleRate)));
    const float filterSmoothCoeff = std::exp(-1.0f / (filterSmoothMs * 0.001f * static_cast<float>(spec.sampleRate)));
    const float filterCutoffScale = juce::jmax(0.0f, tuning.bbdFilterCutoffScale);
    const float filterCutoffMinHz = juce::jmax(20.0f, tuning.bbdFilterCutoffMinHz);
    const float filterCutoffMaxHz = juce::jmax(filterCutoffMinHz, tuning.bbdFilterCutoffMaxHz);
    const float clockMinHz = juce::jmax(20.0f, tuning.bbdClockMinHz);
    const float clockMaxRatio = juce::jlimit(0.05f, 1.0f, tuning.bbdClockMaxRatio);
    
    // Access LFO buffers from ChorusDSP
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        
        auto& chan = channels[static_cast<size_t>(ch)];
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            // Calculate target delay time in milliseconds
            float targetDelayMs = remappedCentreDelayMs + depthMs * channelLfo[i];
            
            // CRITICAL FIX: Remove hard 50ms clamp - match UI range (up to 100ms)
            // BBD can handle longer delays, just need appropriate clock frequency
            targetDelayMs = juce::jlimit(tuning.bbdDelayMinMs, tuning.bbdDelayMaxMs, targetDelayMs);
            
            // CRITICAL FIX: Smooth delay with SmoothedValue (control-rate style)
            // This gives "analog knob" behavior and kills zipper
            chan.smoothedDelayMs.setTargetValue(targetDelayMs);
            float delayMs = chan.smoothedDelayMs.getNextValue();
            
            // Convert smoothed delay time to BBD clock frequency
            // T_delay = N / (2 * f_clk) => f_clk = N / (2 * T_delay)
            float delaySeconds = delayMs * 0.001f;
            float clockFreq = static_cast<float>(BBD_STAGES) / (2.0f * delaySeconds);
            
            // Clamp to a safer anti-alias clock range.
            // Even if user pushes ratio high, keep a Nyquist safety margin.
            const float nyquistSafeClock = 0.45f * static_cast<float>(spec.sampleRate);
            const float ratioClockLimit = static_cast<float>(spec.sampleRate) * clockMaxRatio;
            float maxClockFreq = juce::jmax(clockMinHz + 1.0f, juce::jmin(nyquistSafeClock, ratioClockLimit));
            clockFreq = juce::jlimit(clockMinHz, maxClockFreq, clockFreq);
            
            const float in = inputSamples[i];
            const float out = processBBDChannel(ch, in, clockFreq, clockSmoothCoeff, filterSmoothCoeff,
                                                filterCutoffScale, filterCutoffMinHz, filterCutoffMaxHz);
            outputSamples[i] = out;
        }
    }
}
