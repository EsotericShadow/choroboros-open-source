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

#include "TuningConfigManager.h"

//==============================================================================
// TuningConfigManager implementation

TuningConfigManager::TuningConfigManager()
{
    // Pre-allocate both buffers; no heap allocation during publish/consume
    buffers[0] = TuningSnapshot{};
    buffers[1] = TuningSnapshot{};
    activeIndex.store(0);
    generationCounter.store(0);
    lastAppliedGeneration = 0;
}

void TuningConfigManager::publishTuning(const TuningSnapshot& snapshot)
{
    // Get inactive buffer index
    const int currentActive = activeIndex.load(std::memory_order_acquire);
    const int inactive = 1 - currentActive;

    // Copy to inactive buffer (no allocation, no lock)
    buffers[inactive] = snapshot;

    // Atomically swap active index
    activeIndex.store(inactive, std::memory_order_release);

    // Increment generation counter
    generationCounter.fetch_add(1, std::memory_order_release);
}

const TuningSnapshot* TuningConfigManager::consumeIfChanged()
{
    // Check generation counter (audio thread only)
    const uint64_t currentGen = generationCounter.load(std::memory_order_acquire);

    if (currentGen == lastAppliedGeneration)
    {
        // No new tuning published since last consume
        return nullptr;
    }

    // Load active index and get snapshot
    const int active = activeIndex.load(std::memory_order_acquire);

    // Update generation
    lastAppliedGeneration = currentGen;

    return &buffers[active];
}
