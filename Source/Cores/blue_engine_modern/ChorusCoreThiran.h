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

#include "../ChorusCore.h"
#include <array>
#include <vector>

// Thiran allpass fractional delay chorus core
// Blue HQ mode — 5th-order maximally flat group delay allpass filter
//
// A Thiran allpass filter of order N provides fractional delay D where
// N <= D < N+1. The transfer function is:
//
//     H(z) = z^{-N} * A(z^{-1}) / A(z)
//
// where A(z) = sum_{k=0}^{N} a_k * z^{-k} and the coefficients a_k are
// computed to maximise group delay flatness at DC (Thiran 1971).
//
// For time-varying chorus modulation, the allpass coefficients are
// recomputed every INTERP_LENGTH samples and linearly ramped to
// prevent DFII-T state transients (zippering / noise).
// A 5th-order filter provides excellent phase accuracy across the
// audible band while keeping CPU cost well below polyphase FIR.
//
// Reference: J.-P. Thiran, "Recursive digital filters with maximally
// flat group delay," IEEE Trans. Circuit Theory, vol. CT-18, no. 6,
// pp. 659-664, Nov. 1971.

class ChorusCoreThiran : public ChorusCore
{
public:
    ChorusCoreThiran();
    ~ChorusCoreThiran() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;

    float getGuardSamples() const override { return static_cast<float>(ORDER) + 1.0f; }
    float getMaxDelaySamples() const override;

private:
    // 5th-order Thiran allpass for HQ fractional delay
    static constexpr int ORDER = 5;

    // Per-channel allpass state
    struct ThiranChannel
    {
        // Current (smoothly interpolated) allpass coefficients a[0..ORDER] where a[0] = 1.0
        std::array<float, ORDER + 1> a {};

        // Target coefficients — recomputed every INTERP_LENGTH samples
        std::array<float, ORDER + 1> aTarget {};

        // Per-sample coefficient increment for linear interpolation from a → aTarget
        std::array<float, ORDER + 1> aStep {};

        // Filter state (direct form II transposed)
        std::array<float, ORDER> state {};

        // Smoothed delay for stability
        float smoothedDelay = 0.0f;
        bool delayInitialized = false;

        // Coefficient interpolation: recompute targets every INTERP_LENGTH samples
        // and linearly ramp current coefficients toward them. This prevents the
        // DFII-T state from seeing abrupt coefficient jumps that cause zippering.
        int interpCounter = 0;
        static constexpr int INTERP_LENGTH = 32;

        void resetState()
        {
            state.fill(0.0f);
            a.fill(0.0f);
            a[0] = 1.0f;
            aTarget.fill(0.0f);
            aTarget[0] = 1.0f;
            aStep.fill(0.0f);
            smoothedDelay = 0.0f;
            delayInitialized = false;
            interpCounter = 0;
        }
    };

    // Compute Thiran allpass coefficients for fractional delay D
    // D must satisfy ORDER <= D < ORDER + 1
    static void computeCoefficients(float D, std::array<float, ORDER + 1>& a);

    // Process one sample through Nth-order allpass
    static float processAllpass(ThiranChannel& ch, float input);

    std::vector<ThiranChannel> channels;

    // Integer delay line (circular buffer) for the bulk delay
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0;

    juce::dsp::ProcessSpec spec {};
    int maxDelaySamples = 0;
};
