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
    // Thiran allpass coefficient formula (Thiran 1971):
    //
    //   a_k = (-1)^k * C(N, k) * product_{n=0}^{N} (D - N + n) / (D - N + k + n)
    //
    // where N = ORDER, D = total fractional delay, C(N,k) = binomial coefficient.
    //
    // We clamp D to [ORDER + 0.01, ORDER + 0.99] for numerical stability.
    // At exactly integer delay the denominator terms can vanish.

    const float Df = juce::jlimit(static_cast<float>(ORDER) + 0.01f,
                                  static_cast<float>(ORDER) + 0.99f, D);
    const int N = ORDER;

    a[0] = 1.0f;

    for (int k = 1; k <= N; ++k)
    {
        // Binomial coefficient C(N, k) via iterative multiplication
        double binom = 1.0;
        for (int j = 1; j <= k; ++j)
            binom *= static_cast<double>(N - k + j) / static_cast<double>(j);

        // Product term: product_{n=0}^{N} (D - N + n) / (D - N + k + n)
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

        // Sign: (-1)^k
        const double sign = (k & 1) ? -1.0 : 1.0;

        a[static_cast<size_t>(k)] = static_cast<float>(sign * binom * prod);
    }
}

float ChorusCoreThiran::processAllpass(ThiranChannel& ch, float input)
{
    // Direct form II transposed implementation of the Nth-order allpass:
    //
    //   H(z) = (a_N + a_{N-1}*z^{-1} + ... + a_0*z^{-N}) /
    //          (a_0 + a_1*z^{-1}   + ... + a_N*z^{-N})
    //
    // Using direct form II transposed for numerical stability:
    //   output = a[N]*input + state[0]
    //   state[k] = a[N-1-k]*input - a[k+1]*output + state[k+1]   for k=0..N-2
    //   state[N-1] = a[0]*input - a[N]*output
    //
    // Since a[0] = 1.0, this simplifies slightly.

    const auto& a = ch.a;
    auto& s = ch.state;

    // Output
    const float output = a[ORDER] * input + s[0];

    // Update state registers
    for (int k = 0; k < ORDER - 1; ++k)
    {
        s[static_cast<size_t>(k)] = a[static_cast<size_t>(ORDER - 1 - k)] * input
                                   - a[static_cast<size_t>(k + 1)] * output
                                   + s[static_cast<size_t>(k + 1)];
    }

    // Last state
    s[ORDER - 1] = a[0] * input - a[ORDER] * output;

    return output;
}

void ChorusCoreThiran::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;

    // Calculate maximum delay needed
    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = ORDER + 2;

    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;

    // Round up to next power of 2
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + ORDER + 2)
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;

    // Allocate per-channel structures
    channels.resize(static_cast<size_t>(spec.numChannels));
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));

    // Gentle one-pole lowpass on output to tame content the allpass passes
    // at unity gain. Cutoff ~16 kHz gives ~6 dB/oct roll-off — comparable
    // to the natural attenuation of a 3rd-order polynomial interpolator.
    {
        constexpr float outputLpCutoffHz = 16000.0f;
        const float w = 2.0f * juce::MathConstants<float>::pi * outputLpCutoffHz
                        / static_cast<float>(spec.sampleRate);
        outputLpAlpha = std::exp(-w);  // y[n] = (1-a)*x[n] + a*y[n-1]
    }

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

    // Access LFO buffers from ChorusDSP
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        auto& thiranCh = channels[static_cast<size_t>(ch)];

        // Initialize delay smoothing on first block
        if (!thiranCh.delayInitialized)
        {
            float initialDelay = centreDelaySamples + depthSamples * channelLfo[0];
            initialDelay = juce::jlimit(guardSamples, maxDelay, initialDelay);
            thiranCh.smoothedDelay = initialDelay;
            thiranCh.delayInitialized = true;

            // Initialize coefficients for the starting delay
            const int intDelay = static_cast<int>(std::floor(initialDelay));
            const float fracDelay = initialDelay - static_cast<float>(intDelay);
            const float thiranD = static_cast<float>(ORDER) + fracDelay;
            computeCoefficients(thiranD, thiranCh.a);
            thiranCh.aTarget = thiranCh.a;
            thiranCh.aStep.fill(0.0f);
            thiranCh.interpCounter = ThiranChannel::INTERP_LENGTH;
        }

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];

            // Calculate target delay from LFO
            float targetDelay = centreDelaySamples + depthSamples * channelLfo[i];
            targetDelay = juce::jlimit(guardSamples, maxDelay, targetDelay);

            // Smooth delay to prevent coefficient jumps.
            // Moderately stronger than typical (τ≈14ms @ 48k, fc≈11Hz) because 5th-order
            // Thiran allpass is sensitive to rapid coefficient changes.
            // Combined with the 32-sample coefficient interpolation below, this provides
            // adequate stability while still tracking LFO rates up to ~10Hz with <3dB loss.
            constexpr float delaySmoothingCoeff = 0.9985f;
            thiranCh.smoothedDelay = delaySmoothingCoeff * thiranCh.smoothedDelay
                                   + (1.0f - delaySmoothingCoeff) * targetDelay;

            // Decompose into integer delay (handled by buffer) and fractional (handled by allpass)
            const float totalDelay = thiranCh.smoothedDelay;
            const int intDelay = static_cast<int>(std::floor(totalDelay));
            const float fracDelay = totalDelay - static_cast<float>(intDelay);

            // Periodically recompute target Thiran coefficients and set up a linear
            // interpolation ramp. This prevents the DFII-T state from seeing abrupt
            // coefficient jumps that cause zippering and noise in 5th-order allpass.
            if (--thiranCh.interpCounter <= 0)
            {
                const float thiranD = static_cast<float>(ORDER) + fracDelay;
                computeCoefficients(thiranD, thiranCh.aTarget);

                constexpr float invLen = 1.0f / static_cast<float>(ThiranChannel::INTERP_LENGTH);
                for (int k = 0; k <= ORDER; ++k)
                    thiranCh.aStep[static_cast<size_t>(k)] =
                        (thiranCh.aTarget[static_cast<size_t>(k)] - thiranCh.a[static_cast<size_t>(k)]) * invLen;

                thiranCh.interpCounter = ThiranChannel::INTERP_LENGTH;
            }

            // Linearly interpolate coefficients toward target (smooth 1/32 step per sample)
            for (int k = 0; k <= ORDER; ++k)
                thiranCh.a[static_cast<size_t>(k)] += thiranCh.aStep[static_cast<size_t>(k)];

            // Write input to delay buffer
            buffer[static_cast<size_t>(writePos)] = in;

            // Read from integer delay position (behind write head, accounting for allpass group delay)
            // The allpass provides ORDER samples of delay, so we subtract ORDER from the integer part.
            // Floor at 0 to handle the case where intDelay < ORDER (very short delays).
            const int readOffset = std::max(0, intDelay - ORDER);
            const int readPos = (writePos - readOffset) & bufferMask;
            const float delayedSample = buffer[static_cast<size_t>(readPos)];

            // Apply Thiran allpass for fractional delay
            const float allpassOut = processAllpass(thiranCh, delayedSample);

            // Gentle one-pole lowpass (~16 kHz) — unlike polynomial interpolators
            // that naturally attenuate above Nyquist, allpass passes everything.
            const float out = (1.0f - outputLpAlpha) * allpassOut
                            + outputLpAlpha * thiranCh.outputLpState;
            thiranCh.outputLpState = out;

            writePos = (writePos + 1) & bufferMask;
            outputSamples[i] = out;
        }
    }
}
