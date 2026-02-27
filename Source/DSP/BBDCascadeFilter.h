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
 * but WITHOUT ANY IMPLIED WARRANTY of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Cascaded 5th-order Butterworth filter. No heap allocation.
 */

#pragma once

#include "BBDFilterDesign.h"

namespace choroboros
{

/** Cascaded first-order + 2 biquads. processSample() for per-sample use. */
class BBDCascadeFilter
{
public:
    void setCoeffs(const BBD5thOrderButterworthCoeffs& coeffs)
    {
        fo = coeffs.first;
        bq1 = coeffs.biquad1;
        bq2 = coeffs.biquad2;
    }

    void reset()
    {
        x1_fo = 0.0f;
        y1_fo = 0.0f;
        x1_b1 = x2_b1 = 0.0f;
        y1_b1 = y2_b1 = 0.0f;
        x1_b2 = x2_b2 = 0.0f;
        y1_b2 = y2_b2 = 0.0f;
    }

    float processSample(float x)
    {
        // First-order: y = b0*x + b1*x1 - a1*y1
        float y = fo.b0 * x + fo.b1 * x1_fo - fo.a1 * y1_fo;
        x1_fo = x;
        y1_fo = y;

        // Biquad 1
        float out1 = bq1.b0 * y + bq1.b1 * x1_b1 + bq1.b2 * x2_b1 - bq1.a1 * y1_b1 - bq1.a2 * y2_b1;
        x2_b1 = x1_b1;
        x1_b1 = y;
        y2_b1 = y1_b1;
        y1_b1 = out1;

        // Biquad 2
        float out2 = bq2.b0 * out1 + bq2.b1 * x1_b2 + bq2.b2 * x2_b2 - bq2.a1 * y1_b2 - bq2.a2 * y2_b2;
        x2_b2 = x1_b2;
        x1_b2 = out1;
        y2_b2 = y1_b2;
        y1_b2 = out2;

        return out2;
    }

private:
    BBDFirstOrderCoeffs fo;
    BBDBiquadCoeffs bq1;
    BBDBiquadCoeffs bq2;

    float x1_fo = 0.0f, y1_fo = 0.0f;
    float x1_b1 = 0.0f, x2_b1 = 0.0f, y1_b1 = 0.0f, y2_b1 = 0.0f;
    float x1_b2 = 0.0f, x2_b2 = 0.0f, y1_b2 = 0.0f, y2_b2 = 0.0f;
};

} // namespace choroboros
