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

#include "ChorusCoreThiranLegacyInactive.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCoreThiranLegacyInactive::ChorusCoreThiranLegacyInactive()
{
}

void ChorusCoreThiranLegacyInactive::computeCoefficients(float D, std::array<float, ORDER + 1>& a)
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

void ChorusCoreThiranLegacyInactive::smoothCoeffsTowardD(AllpassInstance& ap, float thiranD)
{
    std::array<float, ORDER + 1> target {};
    computeCoefficients(thiranD, target);
    const float c = fractionalCoeffSmoothCoeff_;
    for (int k = 0; k <= ORDER; ++k)
        ap.a[static_cast<size_t>(k)] = c * ap.a[static_cast<size_t>(k)]
                                     + (1.0f - c) * target[static_cast<size_t>(k)];
    ap.a[0] = 1.0f;
}

float ChorusCoreThiranLegacyInactive::processAllpass(AllpassInstance& ap, float input)
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

void ChorusCoreThiranLegacyInactive::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
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

    // One-pole on Thiran wet output — slightly lower than 16 kHz to tame HF grain /
    // “drony” hash that reads as zipper when Blue Focus boosts highs (still air at 48 k).
    {
        constexpr float outputLpCutoffHz = 9500.0f;
        const float w = 2.0f * juce::MathConstants<float>::pi * outputLpCutoffHz
                        / static_cast<float>(spec.sampleRate);
        outputLpAlpha = std::exp(-w);
    }

    // Smoothed delay line (τ) — longer than 14 ms so totalDelay slew matches the LFO
    // a bit more gently; reduces edge energy that excites the allpass wrong-way.
    {
        constexpr float delaySmoothingTauMs = 24.0f;
        delaySmoothingCoeff_ = std::exp(-1.0f / (delaySmoothingTauMs * 0.001f
                                                  * static_cast<float>(spec.sampleRate)));
    }

    // Sample-rate-invariant crossfade length: 6.0ms
    // Originally 1ms (48 samples @ 48k), which leaked boundary-crossing
    // transients at ~1.5% THD.  At 6ms the sin²/cos² envelope masks the
    // state-copy transient fully (THD drops to ~0.14%).  The longer window
    // also means fewer crossings per second (guard prevents overlapping
    // crossfades), which further reduces artifacts.
    xfadeLength_ = std::max(4, static_cast<int>(0.006f * static_cast<float>(spec.sampleRate)));

    // Fractional coefficient smoothing — one-pole toward analytic Thiran coeffs (not
    // per-sample full solve — A1). τ ~14 ms balances zipper vs coeff/state lag.
    {
        constexpr float coeffTauMs = 14.0f;
        fractionalCoeffSmoothCoeff_ = std::exp(-1.0f / (coeffTauMs * 0.001f * static_cast<float>(spec.sampleRate)));
    }

    // Centre-delay → samples smoother (~5 ms, was 3 ms) so block centre moves slightly softer.
    centreDelaySmoothAlpha = 1.0f - std::exp(-1.0f / (0.005f * static_cast<float>(spec.sampleRate)));
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);

    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        channels[ch].resetState();
    }
}

void ChorusCoreThiranLegacyInactive::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    for (auto& ch : channels)
        ch.resetState();
    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

float ChorusCoreThiranLegacyInactive::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreThiranLegacyInactive::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    const int p = dsp.runtimeTuningSnapshot.thiranReductionProbe;
    const int reductionProbe = (p >= 1 && p <= 4) ? p : 0;
    const bool snapInnerCentreProbe = (p == 13);

    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();

    constexpr float maximumDelayModulation = 20.0f;
    // Thiran uses block-constant centre delay only. Per-sample centre ms (used by other
    // cores for zippering) interacts badly with this core’s 3 ms centre smoother + dual
    // allpass / xfade state — it reads as fast stepped modulation and “crush”/alias
    // energy, not chorus. Depth modulation stays per-sample via LFO buffers.
    const float centreDelaySamples = currentCentreDelayMs * static_cast<float>(spec.sampleRate) / 1000.0f;
    const float depthSamples = maximumDelayModulation * static_cast<float>(spec.sampleRate) / 1000.0f;

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    // Precompute crossfade table constants
    const float xfadeInvLen = 1.0f / static_cast<float>(xfadeLength_);

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

        const auto beginIntegerHop = [&] (int hopIntDelay, float hopThiranD)
        {
            thiranCh.aIsActive = !thiranCh.aIsActive;

            auto& newActive = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
            auto& oldActive = thiranCh.aIsActive ? thiranCh.apB : thiranCh.apA;

            newActive.state = oldActive.state;
            computeCoefficients(hopThiranD, newActive.a);
            newActive.intDelay = hopIntDelay;

            thiranCh.xfadeCounter = (reductionProbe == 4) ? 0 : xfadeLength_;
            thiranCh.lastIntDelay = hopIntDelay;
        };

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];

            // Per-sample centre delay smoothing (block-constant target for Thiran — see above).
            // Probe 13 snaps this to the block target (isolates inner centre slew vs delay-line slew / probe 2).
            if (snapInnerCentreProbe)
                smoothedCentreDelay[chIdx] = centreDelaySamples;
            else
                smoothedCentreDelay[chIdx] += centreDelaySmoothAlpha * (centreDelaySamples - smoothedCentreDelay[chIdx]);

            // Calculate target delay from LFO
            float targetDelay = smoothedCentreDelay[chIdx] + depthSamples * channelLfo[i];
            targetDelay = juce::jlimit(guardSamples, maxDelay, targetDelay);

            // Smooth delay for stability (see prepare() τ, sample-rate-invariant)
            if (reductionProbe == 2)
                thiranCh.smoothedDelay = targetDelay;
            else
                thiranCh.smoothedDelay = delaySmoothingCoeff_ * thiranCh.smoothedDelay
                                       + (1.0f - delaySmoothingCoeff_) * targetDelay;

            // Deferred integer hop: pinning smoothedDelay during xfade (A13) fought the LFO
            // and read as low choppy drone. Instead, remember the latest floor while blending
            // and apply one catch-up hop when xfade completes (see below after xfadeCounter==0).
            if (thiranCh.xfadeCounter == 0 && thiranCh.pendingIntHopTarget >= 0)
            {
                const int pending = thiranCh.pendingIntHopTarget;
                thiranCh.pendingIntHopTarget = -1;

                const int liveFloor = static_cast<int>(std::floor(thiranCh.smoothedDelay));
                const int targetFloor = juce::jmax(pending, liveFloor);

                if (targetFloor != thiranCh.lastIntDelay)
                {
                    const float lo = static_cast<float>(targetFloor);
                    constexpr float bucketUpperMargin = 1.0e-4f;
                    const float hi = juce::jmin(lo + 1.0f - bucketUpperMargin, maxDelay);
                    thiranCh.smoothedDelay = juce::jlimit(lo, hi, thiranCh.smoothedDelay);

                    const float fracP = thiranCh.smoothedDelay - static_cast<float>(targetFloor);
                    const float thiranDP = static_cast<float>(ORDER) + fracP;
                    beginIntegerHop(targetFloor, thiranDP);
                }
            }

            float totalDelay = thiranCh.smoothedDelay;
            int intDelay = static_cast<int>(std::floor(totalDelay));
            float fracDelay = totalDelay - static_cast<float>(intDelay);
            float thiranD = static_cast<float>(ORDER) + fracDelay;

            // Integer crossings: dual-allpass sin²/cos² crossfade (below). Fractional D
            // must track on the **fade-in** (aIsActive) branch *during* the blend too — if we
            // freeze coeffs for the whole ~6 ms xfade while frac still moves, you get a
            // repeating modulation error that sounds like droning zipper. The fading branch
            // stays fixed for the crossfade window.
            //
            // Do NOT assign lastIntDelay outside this block (xfade guard).
            if (intDelay != thiranCh.lastIntDelay && thiranCh.xfadeCounter <= 0)
                beginIntegerHop(intDelay, thiranD);
            else if (intDelay != thiranCh.lastIntDelay && thiranCh.xfadeCounter > 0)
                thiranCh.pendingIntHopTarget = intDelay;

            totalDelay = thiranCh.smoothedDelay;
            intDelay = static_cast<int>(std::floor(totalDelay));
            fracDelay = totalDelay - static_cast<float>(intDelay);
            thiranD = static_cast<float>(ORDER) + fracDelay;

            // During sin²/cos² integer xfade, fade-in intDelay is fixed but LFO may already
            // have advanced floor(smoothedDelay). Match coeff tracking to the frozen tap and
            // clamp fractional excursion into [0,1) so smoothCoeffsTowardD is not skipped
            // (stale tap + live D was the zipper/crush path; A13 clamp caused LFO chop/drone).
            int intDelayForCoeffs = intDelay;
            float thiranDForCoeffs = thiranD;
            if (thiranCh.xfadeCounter > 0)
            {
                auto& fadeInAp = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
                intDelayForCoeffs = fadeInAp.intDelay;
                const float fracBetween = thiranCh.smoothedDelay - static_cast<float>(intDelayForCoeffs);
                const float clampedFrac = juce::jlimit(0.0f, 0.9999f, fracBetween);
                thiranDForCoeffs = static_cast<float>(ORDER) + clampedFrac;
            }

            {
                auto& trackAp = thiranCh.aIsActive ? thiranCh.apA : thiranCh.apB;
                if (trackAp.intDelay == intDelayForCoeffs)
                {
                    if (reductionProbe == 3)
                        computeCoefficients(thiranDForCoeffs, trackAp.a);
                    else
                        smoothCoeffsTowardD(trackAp, thiranDForCoeffs);
                }
            }

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
                const float progress = static_cast<float>(xfadeLength_ - thiranCh.xfadeCounter)
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

            // Wet output lowpass (see prepare() fc)
            const float out = (reductionProbe == 1)
                ? allpassOut
                : ((1.0f - outputLpAlpha) * allpassOut + outputLpAlpha * thiranCh.outputLpState);
            thiranCh.outputLpState = out;

            // Causal delay line: read taps above used buffer **before** this write. Writing
            // first then reading with (writePos - offset) is usually OK for offset≥1, but
            // same-index ordering is easy to get subtly wrong across branches / hosts; the
            // canonical order avoids folding the current input into the fractional path.
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            outputSamples[i] = out;
        }
    }
}
