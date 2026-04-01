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

#include "DspConfig.h"
#include <array>
#include <atomic>
#include <cstdint>

class ChorusDSP;

//==============================================================================
/**
    Lock-free double-buffer config manager for audio-thread ownership.

    Strategy: Fixed double-buffer with generation counter (DECISION_REALTIME_MUTATION_STRATEGY).

    Usage:
    - UI threads call publishConfig() to atomically publish a new DspConfig.
      No locks, no allocation. Just copies to inactive buffer and swaps index.
    - Audio thread calls consumeAndApplyIfChanged() at top of processBlock()
      to check if generation changed and apply new config.

    Guarantees:
    - No heap allocation on either thread (pre-allocated buffers).
    - No lock acquisition on audio thread (only atomic operations).
    - All DspConfig fields update atomically (one buffer swap).
    - No unbounded latency (O(1) per publish, O(1) per consume).
*/
class DspConfigManager
{
public:
    DspConfigManager();
    ~DspConfigManager() = default;

    //==============================================================================
    /** Publish a new config (UI/message thread).
        Acquires no locks. Copies config to inactive buffer, swaps, increments generation.
        Safe to call from any thread that is NOT the audio thread.
    */
    void publishConfig(const DspConfig& newConfig);

    //==============================================================================
    /** Check for new config and apply to DSP (audio thread, called from processBlock).
        Non-blocking O(1) check. If generation changed, applies config to DSP.
        Returns true if a new config was applied.
    */
    bool consumeAndApplyIfChanged(ChorusDSP& dsp);

    //==============================================================================
    /** For testing: get the last applied config. */
    DspConfig getLastAppliedConfig() const { return buffers[activeIndex.load()]; }

private:
    /** Apply config to DSP. Called only from audio thread. */
    void applyConfigToDsp(const DspConfig& config, ChorusDSP& dsp);

    // Fixed double-buffer: no allocation
    std::array<DspConfig, 2> buffers;

    // Which buffer is active (for reads)
    std::atomic<int> activeIndex { 0 };

    // Generation counter: incremented on every publish
    std::atomic<uint64_t> generationCounter { 0 };

    // Last generation seen by audio thread (for audio thread only)
    uint64_t lastAppliedGeneration = 0;
};
