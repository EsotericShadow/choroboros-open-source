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
 * Proprietary 5th-order Butterworth lowpass design.
 * References: JOS/CCRMA Butterworth poles, bilinear transform with c = cot(pi*fc/fs).
 */

#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <array>

namespace choroboros
{

/** Coefficients for one first-order section: y = b0*x + b1*x1 - a1*y1 */
struct BBDFirstOrderCoeffs
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
};

/** Coefficients for one biquad: y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2 */
struct BBDBiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
};

/** 5th-order Butterworth = 1 first-order + 2 biquads */
struct BBD5thOrderButterworthCoeffs
{
    BBDFirstOrderCoeffs first;
    BBDBiquadCoeffs biquad1;
    BBDBiquadCoeffs biquad2;
};

/**
 * Design 5th-order Butterworth lowpass at cutoff fc Hz, sample rate fs.
 * Uses c = cot(pi*fc/fs) for cutoff prewarp (JOS Digitizing Analog Filters).
 */
inline BBD5thOrderButterworthCoeffs designBBD5thOrderButterworth(float fcHz, float fsHz)
{
    BBD5thOrderButterworthCoeffs coeffs;

    const float fc = juce::jlimit(20.0f, fsHz * 0.49f, fcHz);
    const float c = 1.0f / std::tan(static_cast<float>(juce::MathConstants<double>::pi) * fc / fsHz);

    // 5th-order Butterworth poles (normalized, left-half plane):
    // k=0: theta=pi/10,  pole -sin(pi/10) + j*cos(pi/10)
    // k=1: theta=3pi/10, pole -sin(3pi/10) + j*cos(3pi/10)
    // k=2: theta=5pi/10, real pole at -1
    constexpr double pi = juce::MathConstants<double>::pi;
    const double s1 = std::sin(pi / 10.0);
    const double c1 = std::cos(pi / 10.0);
    const double s2 = std::sin(3.0 * pi / 10.0);
    const double c2 = std::cos(3.0 * pi / 10.0);

    // Real pole at -1: a = 1
    const float a = 1.0f;
    const float denom1 = c + a;
    coeffs.first.b0 = 1.0f / denom1;
    coeffs.first.b1 = 1.0f / denom1;
    coeffs.first.a1 = (a - c) / denom1;

    // Complex pair 1: sigma = s1, omega = c1
    {
        const float sigma = static_cast<float>(s1);
        const float omega = static_cast<float>(c1);
        const float sigmaSq = sigma * sigma;
        const float omegaSq = omega * omega;
        const float cSq = c * c;
        const float K = cSq + 2.0f * sigma * c + (sigmaSq + omegaSq);
        coeffs.biquad1.b0 = 1.0f / K;
        coeffs.biquad1.b1 = 2.0f / K;
        coeffs.biquad1.b2 = 1.0f / K;
        coeffs.biquad1.a1 = 2.0f * (cSq - (sigmaSq + omegaSq)) / K;
        coeffs.biquad1.a2 = (cSq - 2.0f * sigma * c + (sigmaSq + omegaSq)) / K;
    }

    // Complex pair 2: sigma = s2, omega = c2
    {
        const float sigma = static_cast<float>(s2);
        const float omega = static_cast<float>(c2);
        const float sigmaSq = sigma * sigma;
        const float omegaSq = omega * omega;
        const float cSq = c * c;
        const float K = cSq + 2.0f * sigma * c + (sigmaSq + omegaSq);
        coeffs.biquad2.b0 = 1.0f / K;
        coeffs.biquad2.b1 = 2.0f / K;
        coeffs.biquad2.b2 = 1.0f / K;
        coeffs.biquad2.a1 = 2.0f * (cSq - (sigmaSq + omegaSq)) / K;
        coeffs.biquad2.a2 = (cSq - 2.0f * sigma * c + (sigmaSq + omegaSq)) / K;
    }

    return coeffs;
}

} // namespace choroboros
