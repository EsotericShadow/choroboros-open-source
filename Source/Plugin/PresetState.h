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

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/CoreAssignments.h"
#include <optional>
#include <string>

//==============================================================================
/**
    Canonical preset state container.

    This is the single source of truth for user-facing sound state that should
    round-trip through presets, host state, and save files.

    Includes:
    - Core APVTS parameters (rate, depth, offset, width, color, mix)
    - Quality toggle (hqEnabled)
    - Modular core routing (modularCoresEnabled, coreAssignments)
    - Format version for migration

    Does NOT include:
    - Engine parameter profiles (UI convenience, not audio state)
    - Global tuning state (internal calibration)
    - Analyzer state or snapshots
    - Session/UI state
    - Consent or preferences

    PresetState is serializable to/from JSON and can migrate from legacy
    raw-APVTS XML formats.
*/
struct PresetState
{
    // Format version for migration
    int version = 1;

    // Core sound parameters (all required for reproducibility)
    float rate = 1.0f;              // 0.01–10.0 Hz
    float depth = 0.5f;             // 0.0–1.0
    float offset = 90.0f;           // 0.0–180.0°
    float width = 1.0f;             // 0.0–2.0
    float color = 0.5f;             // 0.0–1.0, tone/saturation
    float mix = 0.5f;               // 0.0–1.0, dry/wet blend

    // Quality
    bool hqEnabled = false;

    // Engine selection (0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black)
    int engineColorIndex = 0;

    // Custom engine identity (empty = factory engine, non-empty = custom engine UUID).
    // When non-empty, takes precedence over engineColorIndex for engine identity.
    // See KZN_SHARED_CONTRACT.md section 12.
    std::string customEngineId;

    // Modular core routing (affects audio output)
    bool modularCoresEnabled = false;
    choroboros::CoreAssignmentTable coreAssignments;

    //==============================================================================
    // Validation

    /** Check if preset state contains valid values. */
    bool isValid() const;

    //==============================================================================
    // Serialization to in-memory formats

    /** Serialize to JSON string. Returns empty string on error. */
    std::string serializeToJson() const;

    /** Deserialize from JSON string. Returns std::nullopt on parse error. */
    static std::optional<PresetState> deserializeFromJson(const std::string& json);

    /** Serialize to binary blob (for host state). */
    juce::MemoryBlock serializeToBinary() const;

    /** Deserialize from binary blob, with legacy APVTS XML migration support. */
    static std::optional<PresetState> deserializeFromBinary(const void* data, int sizeInBytes);

    //==============================================================================
    // File I/O

    /** Save to file (user presets). Returns true on success. */
    bool saveToFile(const juce::File& file) const;

    /** Load from file (user presets), with legacy XML support. */
    static std::optional<PresetState> loadFromFile(const juce::File& file);

    //==============================================================================
    // Factory presets

    /** Create a factory preset by index (0-6). */
    static std::optional<PresetState> makeFactoryPreset(int index);
};
