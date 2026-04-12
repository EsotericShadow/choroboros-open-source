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

#include <array>
#include <atomic>
#include <cstdint>

class ChorusDSP;

//==============================================================================
/**
    Fixed-size, lock-free tuning snapshot struct.

    Stores precomputed filter coefficients as raw float arrays (6 coefficients per IIR filter)
    plus all runtime tuning parameters. Safe to copy and assign without allocation.

    Used by TuningConfigManager double-buffer to pass precomputed filter parameters
    from message thread to audio thread without locks or allocations.
*/
struct TuningSnapshot
{
    // All ~100 tuning parameters from RuntimeTuningSnapshot
    float rateSmoothingMs = 20.0f;
    float depthSmoothingMs = 50.0f;
    float depthRateLimit = 2.0f;
    float centreDelaySmoothingMs = 60.0f;
    float colorSmoothingMs = 20.0f;
    float widthSmoothingMs = 20.0f;
    float centreDelayBaseMs = 8.0f;
    float centreDelayScale = 10.0f;

    float hpfCutoffHz = 30.0f;
    float hpfQ = 0.707f;
    float lpfCutoffHz = 20000.0f;
    float lpfQ = 0.707f;
    float preEmphasisFreqHz = 3000.0f;
    float preEmphasisQ = 0.707f;
    float preEmphasisGain = 1.2f;
    float preEmphasisLevelSmoothing = 0.208f;  // τ in seconds (was raw α = 0.95)
    float preEmphasisQuietThreshold = 0.125f;
    float preEmphasisMaxAmount = 0.5f;
    float compressorAttackMs = 50.0f;
    float compressorReleaseMs = 200.0f;
    float compressorThresholdDb = -6.0f;
    float compressorRatio = 4.0f;
    float saturationDriveScale = 3.0f;

    float greenBloomExponent = 1.6f;
    float greenBloomDepthScale = 0.12f;
    float greenBloomCentreOffsetMs = 0.60f;
    float greenBloomCutoffMaxHz = 18000.0f;
    float greenBloomCutoffMinHz = 2600.0f;
    float greenBloomWetBlend = 0.48f;
    float greenBloomGain = 0.05f;

    float blueFocusExponent = 1.35f;
    float blueFocusHpMinHz = 70.0f;
    float blueFocusHpMaxHz = 520.0f;
    float blueFocusLpMaxHz = 18000.0f;
    float blueFocusLpMinHz = 7200.0f;
    float bluePresenceFreqMinHz = 2200.0f;
    float bluePresenceFreqMaxHz = 3600.0f;
    float bluePresenceQMin = 0.75f;
    float bluePresenceQMax = 1.10f;
    float bluePresenceGainMaxDb = 4.8f;
    float blueFocusWetBlend = 0.68f;
    float blueFocusOutputGain = 0.04f;

    float purpleWarpA = 0.35f;
    float purpleWarpB = 0.18f;
    float purpleWarpKBase = 2.0f;
    float purpleWarpKScale = 1.0f;
    float purpleWarpDelaySmoothingMs = 20.0f;

    float purpleOrbitEccentricity = 0.6f;
    float purpleOrbitThetaRateBaseHz = 0.01f;
    float purpleOrbitThetaRateScaleHz = 0.09f;
    float purpleOrbitThetaRate2Ratio = 1.3f;
    float purpleOrbitEccentricity2Ratio = 0.8f;
    float purpleOrbitMix1 = 0.6f;
    float purpleOrbitStereoThetaOffset = 0.25f;
    float purpleOrbitDelaySmoothingMs = 20.0f;

    float blackNqDepthBase = 0.6f;
    float blackNqDepthScale = 1.0f;
    float blackNqDelayGlideMs = 2.5f;

    float blackHqTap2MixBase = 0.18f;
    float blackHqTap2MixScale = 0.32f;
    float blackHqSecondTapDepthBase = 0.55f;
    float blackHqSecondTapDepthScale = 0.7f;
    float blackHqSecondTapDelayOffsetBase = 0.2f;
    float blackHqSecondTapDelayOffsetScale = 2.0f;

    float bbdDelaySmoothingMs = 20.0f;
    float bbdDelayMinMs = 8.0f;
    float bbdDelayMaxMs = 100.0f;
    float bbdCentreBaseMs = 16.0f;
    float bbdCentreScale = 2.0f;
    float bbdDepthMs = 12.0f;
    float bbdClockSmoothingMs = 20.0f;
    float bbdFilterSmoothingMs = 10.0f;
    float bbdFilterCutoffMinHz = 3000.0f;
    float bbdFilterCutoffMaxHz = 9000.0f;
    float bbdFilterCutoffScale = 0.45f;
    float bbdClockMinHz = 6000.0f;
    float bbdClockMaxRatio = 0.9f;
    float bbdStages = 1024.0f;
    float bbdFilterMaxRatio = 0.22f;

    float tapeDelaySmoothingMs = 90.0f;
    float tapeCentreBaseMs = 16.0f;
    float tapeCentreScale = 2.0f;
    float tapeToneMaxHz = 16000.0f;
    float tapeToneMinHz = 12000.0f;
    float tapeToneSmoothingCoeff = 0.25f;  // τ in ms (was raw α = 0.08)
    float tapeDriveScale = 0.35f;
    float tapeLfoRatioScale = 0.05f;
    float tapeLfoModSmoothingCoeff = 0.008f;
    float tapeRatioSmoothingCoeff = 0.004f;
    float tapePhaseDampingPerSec = 0.6188f;  // per-second retention (was per-sample 0.99999)
    float tapeWowFreqBase = 0.33f;
    float tapeWowFreqSpread = 0.03f;
    float tapeFlutterFreqBase = 5.8f;
    float tapeFlutterFreqSpread = 0.2f;
    float tapeWowDepthBase = 0.0022f;
    float tapeWowDepthSpread = 0.0002f;
    float tapeFlutterDepthBase = 0.0011f;
    float tapeFlutterDepthSpread = 0.0001f;
    float tapeRatioMin = 0.96f;
    float tapeRatioMax = 1.04f;
    float tapeWetGain = 1.05f;
    float tapeHermiteTension = 0.75f;

    // Precomputed filter coefficients (raw arrays, not shared_ptr).
    // JUCE 2nd-order IIR uses 6 normalized coefficients:
    // [b0, b1, b2, a0, a1, a2]
    float hpfCoeffs[6] = { 1.0f, -2.0f, 1.0f, 1.0f, -2.0f, 1.0f };
    float lpfCoeffs[6] = { 1.0f, 2.0f, 1.0f, 1.0f, -2.0f, 1.0f };
    float preEmphasisCoeffs[6] = { 1.2f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

    // Flags: did this coefficient set change from last snapshot?
    bool hpfChanged = false;
    bool lpfChanged = false;
    bool preEmphasisChanged = false;
};

//==============================================================================
/**
    Lock-free double-buffer config manager for tuning parameter updates.

    Strategy: Fixed double-buffer with generation counter (like DspConfigManager).

    Usage:
    - Message/timer thread calls precomputeTuning() on message thread (allocation ok),
      then publishTuning() to atomically swap buffers.
    - Audio thread calls consumeIfChanged() at top of processBlock()
      to check if generation changed and retrieve the latest tuning snapshot.

    Guarantees:
    - No heap allocation on audio thread (pre-allocated buffers).
    - No lock acquisition on audio thread (only atomic operations).
    - All TuningSnapshot fields update atomically (one buffer swap).
    - No unbounded latency (O(1) per publish, O(1) per consume).
*/
class TuningConfigManager
{
public:
    TuningConfigManager();
    ~TuningConfigManager() = default;

    //==============================================================================
    /** Publish a new tuning snapshot (message/timer thread).
        Acquires no locks. Copies snapshot to inactive buffer, swaps, increments generation.
        Safe to call from any thread that is NOT the audio thread.
    */
    void publishTuning(const TuningSnapshot& snapshot);

    //==============================================================================
    /** Check for new tuning and retrieve if changed (audio thread, called from processBlock).
        Non-blocking O(1) check. If generation changed, returns pointer to latest snapshot.
        Returns nullptr if no new tuning available, or pointer to active snapshot if changed.
    */
    const TuningSnapshot* consumeIfChanged();

private:
    // Fixed double-buffer: no allocation
    std::array<TuningSnapshot, 2> buffers;

    // Which buffer is active (for reads)
    std::atomic<int> activeIndex { 0 };

    // Generation counter: incremented on every publish
    std::atomic<uint64_t> generationCounter { 0 };

    // Last generation seen by audio thread (for audio thread only)
    uint64_t lastAppliedGeneration = 0;
};
