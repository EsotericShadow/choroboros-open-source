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

#pragma once

#include "../../ChorusCore.h"
#include <array>
#include <vector>

// Thiran allpass fractional delay chorus core
// Blue HQ mode — 5th-order maximally flat group delay allpass filter
//
// Uses a dual-allpass crossfade strategy to handle integer delay boundary
// crossings cleanly. When the integer part of the delay changes, the active
// allpass (with settled DFII-T state) becomes the "fading out" instance while
// a fresh allpass (with coefficients for the new integer delay) fades in.
// A sin²/cos² crossfade (~6ms, sample-rate-invariant) masks the coefficient discontinuity
// that would otherwise produce scratchy artifacts.
//
// Reference: J.-P. Thiran, "Recursive digital filters with maximally
// flat group delay," IEEE Trans. Circuit Theory, vol. CT-18, no. 6,
// pp. 659-664, Nov. 1971.

class ChorusCoreThiranLegacyInactive : public ChorusCore
{
public:
    ChorusCoreThiranLegacyInactive();
    ~ChorusCoreThiranLegacyInactive() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;

    float getGuardSamples() const override { return static_cast<float>(ORDER) + 1.0f; }
    float getMaxDelaySamples() const override;

private:
    // 5th-order Thiran allpass for HQ fractional delay
    static constexpr int ORDER = 5;

    // Single allpass filter instance
    struct AllpassInstance
    {
        std::array<float, ORDER + 1> a {};
        std::array<float, ORDER> state {};
        int intDelay = 0;

        void resetState()
        {
            a.fill(0.0f);
            a[0] = 1.0f;
            state.fill(0.0f);
            intDelay = 0;
        }
    };

    // Per-channel state with dual-allpass crossfade
    struct ThiranChannel
    {
        AllpassInstance apA;   // Currently active allpass
        AllpassInstance apB;   // Secondary allpass (used during crossfade)

        // Crossfade state: when integer delay changes, we fade from apA to apB
        // over xfadeLength samples using sin²/cos² envelope
        int xfadeCounter = 0;      // Counts down from xfadeLength to 0; 0 = no crossfade active
        bool aIsActive = true;     // Which instance is the "new" one after crossfade

        // Smoothed delay for stability
        float smoothedDelay = 0.0f;
        bool delayInitialized = false;
        int lastIntDelay = 0;

        // Integer floor seen while xfadeCounter > 0 (latest wins). Drained when xfade completes.
        int pendingIntHopTarget = -1;

        // One-pole lowpass on wet output (fc set in prepare(), ~9.5 kHz @ 48 k)
        float outputLpState = 0.0f;

        void resetState()
        {
            apA.resetState();
            apB.resetState();
            xfadeCounter = 0;
            aIsActive = true;
            smoothedDelay = 0.0f;
            delayInitialized = false;
            lastIntDelay = 0;
            pendingIntHopTarget = -1;
            outputLpState = 0.0f;
        }
    };

    // Compute Thiran allpass coefficients for fractional delay D
    // D must satisfy ORDER <= D < ORDER + 1
    static void computeCoefficients(float D, std::array<float, ORDER + 1>& a);

    // One-pole move of ap.a[] toward coefficients for thiranD (DFII-T safe fractional track).
    void smoothCoeffsTowardD(AllpassInstance& ap, float thiranD);

    // Process one sample through Nth-order allpass
    static float processAllpass(AllpassInstance& ap, float input);

    std::vector<ThiranChannel> channels;

    // Integer delay line (circular buffer) for the bulk delay
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0;

    juce::dsp::ProcessSpec spec {};
    int maxDelaySamples = 0;

    // One-pole output lowpass coefficient: y[n] = (1-a)*x[n] + a*y[n-1]
    float outputLpAlpha = 0.0f;

    // Sample-rate-invariant delay smoothing coefficient (τ 24 ms in prepare()).
    float delaySmoothingCoeff_ = 0.9985f;

    // Sample-rate-invariant crossfade length (samples).
    // Derived from a 6.0 ms target duration in prepare().
    // (Was static constexpr 48 — only 1 ms at 48 k, leaked 1.5% THD transients.)
    int xfadeLength_ = 288;

    // One-pole coefficient tracking: smooth a[k] toward the analytic Thiran target on the
    // fade-in branch (including during integer crossfades when floor(delay) still matches).
    // τ set in prepare() (~14 ms @ 48 k). Do not force-quit the 6 ms blend mid-xfade (A7).
    float fractionalCoeffSmoothCoeff_ = 0.0f;

    // Per-channel one-pole centre delay smoothing
    std::array<float, 2> smoothedCentreDelay {{ 0.0f, 0.0f }};
    std::array<bool, 2> centreDelayInitialized {{ false, false }};
    float centreDelaySmoothAlpha = 0.0f;
};
