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

#include "../DSP/CoreAssignments.h"

//==============================================================================
/**
    Immutable snapshot of live DSP configuration.

    This is a fixed-size value type that captures the runtime parameters
    needed by the audio thread to apply DSP state coherently. All fields
    change together (atomic double-buffer swap on UI thread, consumed on
    audio thread at block boundary).

    Does NOT include:
    - Engine parameter profiles (UI convenience state)
    - Global tuning state (internal calibration, not preset state)
    - Session/diagnostics/consent state
    - Runtime tuning values (applied separately via timer thread)

    This struct is cheap to copy and stack-friendly. It fits in a
    fixed double-buffer pair without heap allocation.
*/
struct DspConfig
{
    // Core sound parameters (mapped/display values)
    float rate = 1.0f;              // 0.01–10.0 Hz
    float depth = 0.5f;             // 0.0–1.0
    float offset = 90.0f;           // 0.0–180.0°
    float width = 1.0f;             // 0.0–2.0
    float color = 0.5f;             // 0.0–1.0, tone/saturation
    float mix = 0.5f;               // 0.0–1.0, dry/wet blend
    float outputTrim = 0.0f;        // -12.0–12.0 dB

    // Quality toggle
    bool hqEnabled = false;

    // Engine color index (0-4)
    int engineColorIndex = 0;

    // Modular core routing (affects which algorithm runs)
    bool modularCoresEnabled = false;
    choroboros::CoreAssignmentTable coreAssignments;

    //==============================================================================
    /** Check if config contains sensible values. */
    bool isValid() const;

    /** Compare configs for equality (used to skip redundant applies). */
    bool operator==(const DspConfig& other) const;
    bool operator!=(const DspConfig& other) const { return !(*this == other); }
};
