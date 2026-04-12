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

/**
 * Shared Catmull-Rom cubic interpolation for manual delay-line reads.
 *
 * Used by ChorusCoreCubic, ChorusCorePhaseWarped, and ChorusCoreOrbit.
 * Keeping one copy avoids drift between engines when fixes are applied
 * (e.g., the NaN guard added in v2.05 Phase 1).
 *
 * @param buf        Pointer to the circular delay buffer.
 * @param bufferMask Power-of-two mask for wrapping indices.
 * @param readPos    Fractional read position (must be >= 0, already wrapped).
 *                   A NaN guard is applied internally.
 * @return           Catmull-Rom interpolated sample.
 */
inline float readCubicInterp(const float* buf, int bufferMask, float readPos)
{
    // NaN guard: static_cast<int>(NaN) is undefined behaviour in C++.
    // The !(x >= 0) form catches NaN (unlike std::isnan which -ffast-math may remove).
    if (!(readPos >= 0.0f))
        readPos = 0.0f;

    // Get integer and fractional parts
    int i1 = static_cast<int>(readPos);
    float u = readPos - static_cast<float>(i1); // Fractional part in [0,1)

    // Get indices for 4-point cubic (p_{-1}, p_0, p_{+1}, p_{+2})
    int im1 = (i1 - 1) & bufferMask;
    int i0  = (i1 + 0) & bufferMask;
    int ip1 = (i1 + 1) & bufferMask;
    int ip2 = (i1 + 2) & bufferMask;

    // Get samples
    float pm1 = buf[im1];
    float p0  = buf[i0];
    float p1  = buf[ip1];
    float p2  = buf[ip2];

    // Catmull-Rom cubic weights
    float u2 = u * u;
    float u3 = u2 * u;

    float w_m1 = 0.5f * (-u3 + 2.0f * u2 - u);
    float w_0  = 0.5f * ( 3.0f * u3 - 5.0f * u2 + 2.0f);
    float w_1  = 0.5f * (-3.0f * u3 + 4.0f * u2 + u);
    float w_2  = 0.5f * ( u3 - u2);

    return w_m1 * pm1 + w_0 * p0 + w_1 * p1 + w_2 * p2;
}
