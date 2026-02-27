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

#include "ChorusCoreTape.h"
#include "../../DSP/ChorusDSP.h"
#include <algorithm>
#include <cmath>

ChorusCoreTape::ChorusCoreTape() {}

void ChorusCoreTape::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;

    constexpr float maxCentreDelayMs = 100.0f;
    maxDelaySamples = static_cast<int>(std::ceil(maxCentreDelayMs * static_cast<float>(spec.sampleRate) / 1000.0f)) + 16;

    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 8)
        bufferSize <<= 1;

    bufferMask = bufferSize - 1;

    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels), 0);
    resamplers.resize(static_cast<size_t>(spec.numChannels));
    tapeMod.resize(static_cast<size_t>(spec.numChannels));
    toneLPState.resize(static_cast<size_t>(spec.numChannels));

    for (size_t ch = 0; ch < static_cast<size_t>(spec.numChannels); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);

        auto& mod = tapeMod[ch];
        mod.wowFreq = 0.33f + 0.03f * static_cast<float>(ch);
        mod.flutterFreq = 5.8f + 0.2f * static_cast<float>(ch);
        mod.wowDepth = 0.0022f + 0.0002f * static_cast<float>(ch);
        mod.flutterDepth = 0.0011f + 0.0001f * static_cast<float>(ch);
    }

    reset();
}

void ChorusCoreTape::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);

    std::fill(writePositions.begin(), writePositions.end(), 0);

    for (auto& resampler : resamplers)
    {
        resampler.phaseOffset = 0.0f;
        resampler.smoothedRatio = 1.0f;
        resampler.smoothedLfoMod = 0.0f;
    }

    for (auto& mod : tapeMod)
    {
        mod.wowPhase = 0.0f;
        mod.flutterPhase = 0.0f;
    }

    for (auto& s : toneLPState)
    {
        s.state1 = 0.0f;
        s.state2 = 0.0f;
    }

    currentFixedDelay = -1.0f;
    smoothedToneCutoff = 14000.0f;
}

float ChorusCoreTape::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreTape::resampleHermite(const float* buf, int mask, float realPos, float tension) const
{
    const int i1 = static_cast<int>(std::floor(realPos));
    const float t = realPos - static_cast<float>(i1);

    const float p0 = buf[(i1 - 1) & mask];
    const float p1 = buf[i1 & mask];
    const float p2 = buf[(i1 + 1) & mask];
    const float p3 = buf[(i1 + 2) & mask];

    const float m1 = (p2 - p0) * tension;
    const float m2 = (p3 - p1) * tension;

    const float a = 2.0f * p1 - 2.0f * p2 + m1 + m2;
    const float b = -3.0f * p1 + 3.0f * p2 - 2.0f * m1 - m2;
    const float c = m1;
    const float d = p1;

    return ((a * t + b) * t + c) * t + d;
}

float ChorusCoreTape::tapeSaturate(float sample, float drive) const
{
    if (drive <= 1.0f)
        return sample;

    // Simple tanh soft clipping
    return std::tanh(sample * drive) / drive;
}

void ChorusCoreTape::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());
    const auto& tuning = dsp.runtimeTuningSnapshot;

    if (numChannels <= 0 || numSamples <= 0)
        return;

    const float sampleRate = static_cast<float>(spec.sampleRate);
    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();

    // Use a longer base delay for the tape mode so modulation reads as chorus, not subtle combing.
    const float remappedCentreDelayMs = tuning.tapeCentreBaseMs + (currentCentreDelayMs - 8.0f) * tuning.tapeCentreScale;
    const float targetDelay = juce::jlimit(guardSamples, maxDelay, remappedCentreDelayMs * sampleRate / 1000.0f);

    if (currentFixedDelay < 0.0f)
    {
        currentFixedDelay = targetDelay;
    }
    else
    {
        // Smooth changes to delay time (e.g. from UI) to prevent zippers.
        // Intentionally slower so rapid depth moves don't produce stepping artifacts.
        const float delaySmoothingMs = juce::jmax(0.001f, tuning.tapeDelaySmoothingMs);
        const float blockMs = 1000.0f * static_cast<float>(numSamples) / sampleRate;
        const float a = std::exp(-blockMs / delaySmoothingMs);
        currentFixedDelay = a * currentFixedDelay + (1.0f - a) * targetDelay;
    }

    const float depth = juce::jlimit(0.0f, 1.0f, dsp.smoothedDepthValue);
    const float color = juce::jlimit(0.0f, 1.0f, dsp.smoothedColor.getCurrentValue());

    float toneMax = tuning.tapeToneMaxHz;
    float toneMin = tuning.tapeToneMinHz;
    if (toneMin > toneMax)
        std::swap(toneMin, toneMax);
    const float targetToneCutoff = toneMax - (toneMax - toneMin) * color;
    smoothedToneCutoff += tuning.tapeToneSmoothingCoeff * (targetToneCutoff - smoothedToneCutoff);

    constexpr float cutoffRecomputeThresholdHz = 5.0f;

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    // If stereo, use cosBuffer for right channel (quadrature LFO)
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    // Drive increases with Color knob
    const float drive = 1.0f + tuning.tapeDriveScale * color;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = block.getChannelPointer(ch);
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        auto& resampler = resamplers[static_cast<size_t>(ch)];
        auto& mod = tapeMod[static_cast<size_t>(ch)];
        auto& toneState = toneLPState[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        mod.wowFreq = tuning.tapeWowFreqBase + tuning.tapeWowFreqSpread * static_cast<float>(ch);
        mod.flutterFreq = tuning.tapeFlutterFreqBase + tuning.tapeFlutterFreqSpread * static_cast<float>(ch);
        mod.wowDepth = tuning.tapeWowDepthBase + tuning.tapeWowDepthSpread * static_cast<float>(ch);
        mod.flutterDepth = tuning.tapeFlutterDepthBase + tuning.tapeFlutterDepthSpread * static_cast<float>(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = samples[i];

            // 1. Calculate Modulation (Tape Wow/Flutter)
            mod.wowPhase += mod.wowFreq / sampleRate;
            mod.flutterPhase += mod.flutterFreq / sampleRate;
            if (mod.wowPhase >= 1.0f) mod.wowPhase -= 1.0f;
            if (mod.flutterPhase >= 1.0f) mod.flutterPhase -= 1.0f;

            const float wow = std::sin(juce::MathConstants<float>::twoPi * mod.wowPhase) * mod.wowDepth * depth;
            const float flutter = std::sin(juce::MathConstants<float>::twoPi * mod.flutterPhase) * mod.flutterDepth * depth;

            // 2. LFO Modulation
            // The LFO buffer from ChorusDSP is typically scaled to ±0.5 max amplitude (controlled by depth).
            // We need to scale this to a pitch ratio deviation.
            // A classic chorus might have ±1% speed variation (~16 cents).
            // 0.5 * 0.02 = 0.01 (1%).
            // Increased from 0.006 to 0.02 to ensure audible classic chorus effect.
            const float targetLfoMod = channelLfo[i] * tuning.tapeLfoRatioScale;
            // Very slow response to suppress zipper when Depth changes quickly.
            resampler.smoothedLfoMod += tuning.tapeLfoModSmoothingCoeff * (targetLfoMod - resampler.smoothedLfoMod);

            // Target Ratio: 1.0 = normal speed
            float targetRatio = 1.0f + resampler.smoothedLfoMod + wow + flutter;
            
            // Limit ratio to prevent extreme pitch shifts or instability
            // ±2% is plenty for even extreme chorus
            float ratioMin = tuning.tapeRatioMin;
            float ratioMax = tuning.tapeRatioMax;
            if (ratioMin > ratioMax)
                std::swap(ratioMin, ratioMax);
            targetRatio = juce::jlimit(ratioMin, ratioMax, targetRatio);

            // Smooth the ratio to avoid zipper noise from rapid LFO/Wow changes
            // Slower ratio tracking to further de-emphasize fast control transients.
            resampler.smoothedRatio += tuning.tapeRatioSmoothingCoeff * (targetRatio - resampler.smoothedRatio);

            // 3. Integrate Varispeed to get Position Offset
            // If ratio > 1.0, we consume samples faster, so read head moves closer to write head (delay decreases).
            // Integration: Position += Velocity * dt.
            // Our "Velocity" relative to write head is (1.0 - ratio).
            resampler.phaseOffset += (1.0f - resampler.smoothedRatio);

            // Leaky Integrator / Spring
            // This pulls the read head back to the center delay time.
            // If too strong, it kills the LFO drift. If too weak, it drifts too far.
            // 0.99998 lets it drift ~50x more than 0.999.
            // Tuned for ~1-2 Hz LFOs to allow sufficient excursion.
            resampler.phaseOffset *= tuning.tapePhaseDamping;

            // 4. Calculate Read Pulse
            float effectiveDelay = currentFixedDelay + resampler.phaseOffset;
            
            // Clamp delay to buffer bounds (safety)
            // If the integrator allows too much drift, this hard limit saves us.
            effectiveDelay = juce::jlimit(guardSamples, maxDelay, effectiveDelay);

            // Update phaseOffset to reflect the clampling (anti-windup)
            resampler.phaseOffset = effectiveDelay - currentFixedDelay;

            float readPos = static_cast<float>(writePos) - effectiveDelay;
            while (readPos < 0.0f)
                readPos += static_cast<float>(bufferSize);
            while (readPos >= static_cast<float>(bufferSize))
                readPos -= static_cast<float>(bufferSize);

            // 5. Read & Interpolate
            float wet = resampleHermite(buffer.data(), bufferMask, readPos, tuning.tapeHermiteTension);

            // Apply Tape Tone (2-pole cascaded one-pole LP, no allocation)
            const float cutoff = juce::jlimit(20.0f, 0.49f * sampleRate, smoothedToneCutoff);
            if (std::abs(cutoff - toneState.cachedCutoffHz) > cutoffRecomputeThresholdHz || toneState.cachedCutoffHz < 0.0f)
            {
                toneState.cachedG = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / sampleRate);
                toneState.cachedCutoffHz = cutoff;
            }
            const float g = toneState.cachedG;
            toneState.state1 = g * toneState.state1 + (1.0f - g) * wet;
            toneState.state2 = g * toneState.state2 + (1.0f - g) * toneState.state1;
            wet = toneState.state2;
            
            // Mild makeup gain so tape modulation remains present at lower wet mixes.
            samples[i] = wet * tuning.tapeWetGain;

            // 6. Write to Buffer (Saturate input)
            // Tape saturation happens on record (write)
            buffer[static_cast<size_t>(writePos)] = tapeSaturate(in, drive);
            
            writePos = (writePos + 1) & bufferMask;
        }
    }
}
