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

#include "DspConfigManager.h"
#include "../DSP/ChorusDSP.h"

//==============================================================================
// DspConfig implementation

bool DspConfig::isValid() const
{
    // Check ranges
    if (rate < 0.01f || rate > 10.0f)
        return false;
    if (depth < 0.0f || depth > 1.0f)
        return false;
    if (offset < 0.0f || offset > 180.0f)
        return false;
    if (width < 0.0f || width > 2.0f)
        return false;
    if (color < 0.0f || color > 1.0f)
        return false;
    if (mix < 0.0f || mix > 1.0f)
        return false;
    if (engineColorIndex < 0 || engineColorIndex > 4)
        return false;

    return true;
}

bool DspConfig::operator==(const DspConfig& other) const
{
    return rate == other.rate
        && depth == other.depth
        && offset == other.offset
        && width == other.width
        && color == other.color
        && mix == other.mix
        && hqEnabled == other.hqEnabled
        && engineColorIndex == other.engineColorIndex
        && modularCoresEnabled == other.modularCoresEnabled
        && coreAssignments == other.coreAssignments;
}

//==============================================================================
// DspConfigManager implementation

DspConfigManager::DspConfigManager()
{
    // Pre-allocate both buffers; no heap allocation during publish/consume
    buffers[0] = DspConfig{};
    buffers[1] = DspConfig{};
    activeIndex.store(0);
    generationCounter.store(0);
    lastAppliedGeneration = 0;
}

void DspConfigManager::publishConfig(const DspConfig& newConfig)
{
    // Get inactive buffer index
    const int currentActive = activeIndex.load(std::memory_order_acquire);
    const int inactive = 1 - currentActive;

    // Copy to inactive buffer (no allocation, no lock)
    buffers[inactive] = newConfig;

    // Atomically swap active index
    activeIndex.store(inactive, std::memory_order_release);

    // Increment generation counter
    generationCounter.fetch_add(1, std::memory_order_release);
}

bool DspConfigManager::consumeAndApplyIfChanged(ChorusDSP& dsp)
{
    // Check generation counter (audio thread only)
    const uint64_t currentGen = generationCounter.load(std::memory_order_acquire);

    if (currentGen == lastAppliedGeneration)
    {
        // No new config published since last apply
        return false;
    }

    // Load active index and get config
    const int active = activeIndex.load(std::memory_order_acquire);
    const DspConfig config = buffers[active];

    // Apply to DSP
    applyConfigToDsp(config, dsp);

    // Update generation
    lastAppliedGeneration = currentGen;

    return true;
}

void DspConfigManager::applyConfigToDsp(const DspConfig& config, ChorusDSP& dsp)
{
    // Apply all live DSP state in one coherent place
    // This is the ONLY place where ChorusDSP setters are called from the audio thread

    // Basic parameters (always apply in order to maintain consistency)
    dsp.setRate(config.rate);
    dsp.setDepth(config.depth);
    dsp.setOffset(config.offset);
    dsp.setWidth(config.width);
    dsp.setColor(config.color);
    dsp.setMix(config.mix);
    dsp.setOutputTrim(config.outputTrim);

    // Quality and engine selection
    dsp.setQualityEnabled(config.hqEnabled);
    dsp.setEngineColor(config.engineColorIndex);

    // Modular core routing
    dsp.setModularCoreModeEnabled(config.modularCoresEnabled);
    dsp.setCoreAssignments(config.coreAssignments);
}
