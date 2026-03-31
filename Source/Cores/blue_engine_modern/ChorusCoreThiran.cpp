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

#include "ChorusCoreThiran.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCoreThiran::ChorusCoreThiran()
{
}

void ChorusCoreThiran::computeCoefficients(float D, std::array<float, ORDER + 1>& a)
{
    const float Df = juce::jlimit(static_cast<float>(ORDER) + 0.01f,
                                  static_cast<float>(ORDER) + 0.99f, D);
    const int N = ORDER;

    a[0] = 1.0f;

    for (int k = 1; k <= N; ++k)
    {
        double binom = 1.0;
        for (int j = 1; j <= k; ++j)
            binom *= static_cast<double>(N - k + j) / static_cast<double>(j);

        double prod = 1.0;
        for (int n = 0; n <= N; ++n)
        {
            const double num = static_cast<double>(Df) - static_cast<double>(N) + static_cast<double>(n);
            const double den = static_cast<double>(Df) - static_cast<double>(N) + static_cast<double>(k) + static_cast<double>(n);
            if (std::abs(den) < 1e-12)
            {
                prod = 0.0;
                break;
            }
            prod *= num / den;
        }

        const double sign = (k & 1) ? -1.0 : 1.0;
        a[static_cast<size_t>(k)] = static_cast<float>(sign * binom * prod);
    }
}

float ChorusCoreThiran::processAllpass(AllpassInstance& ap, float input)
{
    const auto& a = ap.a;
    auto& s = ap.state;

    const float output = a[ORDER] * input + s[0];

    for (int k = 0; k < ORDER - 1; ++k)
    {
        s[static_cast<size_t>(k)] = a[static_cast<size_t>(ORDER - 1 - k)] * input
                                   - a[static_cast<size_t>(k + 1)] * output
                                   + s[static_cast<size_t>(k + 1)];
    }

    s[ORDER - 1] = a[0] * input - a[ORDER] * output;

    return output;
}

void ChorusCoreThiran::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;

    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = ORDER + 2;

    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;

    bufferSize = 1;
    while (bufferSize < maxDelaySamples + ORDER + 2)
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;

    channels.resize(static_cast<size_t>(spec.numChannels));
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));

    // Gentle one-pole lowpass on output (~16 kHz)
    {
        constexpr float outputLpCutoffHz = 16000.0f;
        const float w = 2.0f * juce::MathConstants<float>::pi * outputLpCutoffHz
                        / static_cast<float>(spec.sampleRate);
        outputLpAlpha = std::exp(-w);
    }

    // ~3ms centre delay smoothing
    centreDelaySmoothAlpha = 1.0f - std::exp(-1.0f / (0.003f * static_cast<float>(spec.sampleRate)));
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);

    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        channels[ch].resetState();
    }
}

void ChorusCoreThiran::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    for (auto& ch : channels)
        ch.resetState();
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

float ChorusCoreThiran::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreThiran::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());

    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();

    constexpr float maximumDelayModulation = 20.0f;
    const float centreDelaySamples = currentCentreDelayMs * static_cast<float>(spec.sampleRate) / 1000.0f;
    const float depthSamples = maximumDelayModulation * static_cast<float>(spec.sampleRate) / 1000.0f;

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    // Precompute crossfade table constants
    constexpr float xfadeInvLen = 1.0f / static_cast<float>(ThiranChannel::XFADE_LENGTH);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        auto& thiranCh = channels[static_cast<size_t>(ch)];
        const auto chIdx = static_cast<size_t>(ch);

        if (!centreDelayInitialized[chIdx])
        {
            smoothedCentreDelay[chIdx] = centreDelaySamples;
            centreDelayInitialized[chIdx] = true;
        }

        // Initialize delay smoothing on first block
        if (!thiranCh.delayInitialized)
        {
            float initialDelay = centreDelaySamples + depthSamples * channelLfo[0];
            initialDelay = juce::jlimit(guardSamples, maxDelay, initialDelay);
            thiranCh.smoothedDelay = initialDelay;
            thiranCh.delayInitialized = true;

            const int intDelay = static_cast<int>(std::floor(initialDelay));
            const float fracDelay = initialDelay - static_cast<float>(intDelay);
            const float thiranD = static_cast<float>(ORDER) + fracDelay;

            auto& activeAp = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
            computeCoefficients(thiranD, activeAp.a);
            activeAp.intDelay = intDelay;
            thiranCh.lastIntDelay = intDelay;
        }

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];

            // Per-sample centre delay smoothing
            smoothedCentreDelay[chIdx] += centreDelaySmoothAlpha * (centreDelaySamples - smoothedCentreDelay[chIdx]);

            // Calculate target delay from LFO
            float targetDelay = smoothedCentreDelay[chIdx] + depthSamples * channelLfo[i];
            targetDelay = juce::jlimit(guardSamples, maxDelay, targetDelay);

            // Smooth delay for stability (~14ms τ @ 48k)
            constexpr float delaySmoothingCoeff = 0.9985f;
            thiranCh.smoothedDelay = delaySmoothingCoeff * thiranCh.smoothedDelay
                                   + (1.0f - delaySmoothingCoeff) * targetDelay;

            const float totalDelay = thiranCh.smoothedDelay;
            const int intDelay = static_cast<int>(std::floor(totalDelay));
            const float fracDelay = totalDelay - static_cast<float>(intDelay);
            const float thiranD = static_cast<float>(ORDER) + fracDelay;

            // Determine which allpass is active
            auto& activeAp = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
            auto& fadingAp = thiranCh.aIsActive ? thiranCh.apB : thiranCh.apA;

            // Check if integer delay boundary crossed — initiate crossfade
            if (intDelay != thiranCh.lastIntDelay && thiranCh.xfadeCounter <= 0)
            {
                // The currently active allpass becomes the fading-out one.
                // The other allpass gets fresh coefficients for the new integer delay.
                thiranCh.aIsActive = !thiranCh.aIsActive;

                // After the swap, "activeAp" and "fadingAp" have swapped roles
                auto& newActive = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;

                // Reset the new active allpass state so it starts clean
                newActive.state.fill(0.0f);
                computeCoefficients(thiranD, newActive.a);
                newActive.intDelay = intDelay;

                thiranCh.xfadeCounter = ThiranChannel::XFADE_LENGTH;
                thiranCh.lastIntDelay = intDelay;
            }
            else
            {
                // Update active allpass coefficients for fractional changes (no boundary crossing)
                computeCoefficients(thiranD, activeAp.a);
                activeAp.intDelay = intDelay;
                thiranCh.lastIntDelay = intDelay;
            }

            // Write input to delay buffer
            buffer[static_cast<size_t>(writePos)] = in;

            // Re-reference after potential swap
            auto& curActive = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
            auto& curFading = thiranCh.aIsActive ? thiranCh.apB : thiranCh.apA;

            // Read from integer delay position for active allpass
            const int activeReadOffset = std::max(0, curActive.intDelay - ORDER);
            const int activeReadPos = (writePos - activeReadOffset) & bufferMask;
            const float activeDelayed = buffer[static_cast<size_t>(activeReadPos)];
            const float activeOut = processAllpass(curActive, activeDelayed);

            float allpassOut;

            if (thiranCh.xfadeCounter > 0)
            {
                // During crossfade: also process the fading allpass and blend
                const int fadingReadOffset = std::max(0, curFading.intDelay - ORDER);
                const int fadingReadPos = (writePos - fadingReadOffset) & bufferMask;
                const float fadingDelayed = buffer[static_cast<size_t>(fadingReadPos)];
                const float fadingOut = processAllpass(curFading, fadingDelayed);

                // sin²/cos² crossfade — energy-preserving
                const float progress = static_cast<float>(ThiranChannel::XFADE_LENGTH - thiranCh.xfadeCounter)
                                     * xfadeInvLen;
                const float fadeInGain = std::sin(progress * juce::MathConstants<float>::halfPi);
                const float fadeOutGain = std::cos(progress * juce::MathConstants<float>::halfPi);

                allpassOut = activeOut * fadeInGain + fadingOut * fadeOutGain;
                --thiranCh.xfadeCounter;
            }
            else
            {
                allpassOut = activeOut;
            }

            // Gentle one-pole lowpass (~16 kHz)
            const float out = (1.0f - outputLpAlpha) * allpassOut
                            + outputLpAlpha * thiranCh.outputLpState;
            thiranCh.outputLpState = out;

            writePos = (writePos + 1) & bufferMask;
            outputSamples[i] = out;
        }
    }
}
