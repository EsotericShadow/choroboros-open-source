/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "KnobPhysicsLabSlider.h"
#include <cmath>

namespace
{
constexpr double kFineDragMultiplier = 4.0;

double smoothstep01 (double t)
{
    t = juce::jlimit (0.0, 1.0, t);
    return t * t * (3.0 - 2.0 * t);
}

double velvetCurve (double peakAbsDy, double span, double pStart, double pEnd)
{
    if (peakAbsDy >= span)
        return pEnd;
    const double u = smoothstep01 (peakAbsDy / span);
    return pStart + (pEnd - pStart) * u;
}

bool isSpeedSenseMode (KnobPhysicsLabSlider::PhysicsMode m)
{
    using PM = KnobPhysicsLabSlider::PhysicsMode;
    return m == PM::speedSensitiveA || m == PM::speedSensitiveB
        || m == PM::speedSensitiveC || m == PM::speedSensitiveD;
}

struct SpeedSenseProfile
{
    double blendLo;
    double blendHi;
    double mSlow;
    double mFast;
};

SpeedSenseProfile speedSenseProfileFor (KnobPhysicsLabSlider::PhysicsMode m)
{
    using PM = KnobPhysicsLabSlider::PhysicsMode;
    switch (m)
    {
        case PM::speedSensitiveA: return { 0.45, 7.0, 1.42, 0.82 };
        case PM::speedSensitiveB: return { 0.85, 12.5, 1.26, 0.90 };
        case PM::speedSensitiveC: return { 0.22, 4.8, 1.58, 0.74 };
        case PM::speedSensitiveD: return { 0.38, 17.0, 1.36, 0.84 };
        default: return { 0.5, 8.0, 1.35, 0.85 };
    }
}

double speedSenseMultiplier (KnobPhysicsLabSlider::PhysicsMode m, double smoothedAbsDdy)
{
    const SpeedSenseProfile p = speedSenseProfileFor (m);
    const double ad = juce::jmax (0.0, smoothedAbsDdy);
    if (ad <= p.blendLo)
        return p.mSlow;
    if (ad >= p.blendHi)
        return p.mFast;
    const double u = smoothstep01 ((ad - p.blendLo) / (p.blendHi - p.blendLo));
    return p.mSlow + (p.mFast - p.mSlow) * u;
}
}

KnobPhysicsLabSlider::KnobPhysicsLabSlider (PhysicsMode mode)
    : SmoothedSlider (60.0f, false), physicsMode (mode)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setVelocityBasedMode (false);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
}

bool KnobPhysicsLabSlider::isFinePointerDrag (const juce::MouseEvent& e)
{
    const auto live = juce::ModifierKeys::getCurrentModifiersRealtime();
    return e.mods.isCommandDown() || live.isCommandDown()
        || e.mods.isCtrlDown()   || live.isCtrlDown();
}

double KnobPhysicsLabSlider::basePixelsForFullDrag (bool fine) const
{
    const double p = static_cast<double> (juce::jmax (50, getMouseDragSensitivity()));
    return fine ? (p * kFineDragMultiplier) : p;
}

double KnobPhysicsLabSlider::pixelMultiplierAt (double peakAbsDy, double currentProp) const
{
    const double P0 = 1.0;
    const double Pheavy = 2.35;

    switch (physicsMode)
    {
        case PhysicsMode::linearReference:
        case PhysicsMode::linearDuplicate:
            return P0;

        case PhysicsMode::stiffThenLinear:
            return peakAbsDy < 10.0 ? Pheavy : P0;

        case PhysicsMode::stiffSnapTight:
            return peakAbsDy < 6.0 ? Pheavy : P0;

        case PhysicsMode::stiffEaseLinear:
        {
            constexpr double T = 8.0;
            constexpr double W = 22.0;
            if (peakAbsDy <= T)
                return Pheavy;
            if (peakAbsDy >= T + W)
                return P0;
            const double u = smoothstep01 ((peakAbsDy - T) / W);
            return Pheavy + (P0 - Pheavy) * u;
        }

        case PhysicsMode::stiffEaseFast:
        {
            constexpr double T = 6.0;
            constexpr double W = 12.0;
            if (peakAbsDy <= T)
                return Pheavy;
            if (peakAbsDy >= T + W)
                return P0;
            const double u = smoothstep01 ((peakAbsDy - T) / W);
            return Pheavy + (P0 - Pheavy) * u;
        }

        case PhysicsMode::alwaysHeavy:
            return Pheavy;

        case PhysicsMode::quickStart:
            return peakAbsDy < 8.0 ? 0.55 : P0;

        case PhysicsMode::edgeHeavy:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            const double nearLow = juce::jmax (0.0, (0.10 - p) / 0.10);
            const double nearHigh = juce::jmax (0.0, (p - 0.90) / 0.10);
            const double edge = juce::jlimit (0.0, 1.0, juce::jmax (nearLow, nearHigh));
            return P0 + (Pheavy - P0) * edge;
        }

        case PhysicsMode::velvetClutch:
        case PhysicsMode::velvetRearmOnReverse:
            return velvetCurve (peakAbsDy, 56.0, 3.65, 0.88);

        case PhysicsMode::velvetClutchDramatic:
            return velvetCurve (peakAbsDy, 78.0, 5.45, 0.52);

        case PhysicsMode::directionBandHeavy:
            return peakAbsDy < 15.0 ? 2.95 : P0;

        case PhysicsMode::tripleZonePeak:
            if (peakAbsDy < 18.0)
                return 3.85;
            if (peakAbsDy < 48.0)
                return 0.52;
            if (peakAbsDy < 76.0)
                return 3.05;
            return P0;

        case PhysicsMode::centerVault:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            const double sigma = 0.095;
            const double d = p - 0.5;
            const double g = std::exp (-(d * d) / (2.0 * sigma * sigma));
            return P0 + 2.75 * g;
        }

        case PhysicsMode::endMagnets:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            const double nearLow = juce::jmax (0.0, (0.25 - p) / 0.25);
            const double nearHigh = juce::jmax (0.0, (p - 0.75) / 0.25);
            const double edge = juce::jlimit (0.0, 1.0, juce::jmax (nearLow, nearHigh));
            return P0 + 3.35 * edge;
        }

        case PhysicsMode::barrierExtremeEnds:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            if (p < 0.08 || p > 0.92)
                return P0 + 4.35;
            return P0;
        }

        case PhysicsMode::stickyMidBand:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            if (p >= 0.38 && p <= 0.62)
                return 2.72;
            return P0;
        }

        case PhysicsMode::valueDetents:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            double s = 0.0;
            for (int k = 1; k < 10; ++k)
            {
                const double t = p * 10.0 - static_cast<double> (k);
                s += std::exp (-(t * t) / (2.0 * 0.021 * 0.021));
            }
            return P0 + 1.22 * s;
        }

        case PhysicsMode::depthQuadWell:
        {
            const double p = juce::jlimit (0.0, 1.0, currentProp);
            const double d = p - 0.5;
            return P0 + 2.88 * d * d;
        }

        case PhysicsMode::sprintBoost:
            return P0;

        case PhysicsMode::iceThenFire:
        {
            constexpr double iceEnd = 8.0;
            constexpr double fireEnd = 38.0;
            constexpr double Pice = 0.34;
            constexpr double Pfire = 3.45;
            if (peakAbsDy < iceEnd)
                return Pice;
            if (peakAbsDy < fireEnd)
            {
                const double u = smoothstep01 ((peakAbsDy - iceEnd) / (fireEnd - iceEnd));
                return Pice + (Pfire - Pice) * u;
            }
            return P0;
        }

        case PhysicsMode::heavyVise:
            return 3.92;

        case PhysicsMode::featherTouch:
            return 0.50;

        case PhysicsMode::asymmetricUp:
        case PhysicsMode::speedSensitiveA:
        case PhysicsMode::speedSensitiveB:
        case PhysicsMode::speedSensitiveC:
        case PhysicsMode::speedSensitiveD:
            return P0;

        default:
            return P0;
    }
}

void KnobPhysicsLabSlider::mouseDown (const juce::MouseEvent& e)
{
    if (physicsMode == PhysicsMode::linearReference
        || physicsMode == PhysicsMode::linearDuplicate
        || physicsMode == PhysicsMode::mixShowcase)
    {
        usingCustomDrag = false;
        SmoothedSlider::mouseDown (e);
        return;
    }

    if (! isEnabled() || e.mods.isPopupMenu())
    {
        SmoothedSlider::mouseDown (e);
        return;
    }

    usingCustomDrag = true;
    dragAccumProp = valueToProportionOfLength (getValue());
    customDragAnchor = e.position;
    lastAccumDy = 0.0;
    dragPeakAbsDy = 0.0;
    pathSinceDirectionChange = 0.0;
    lastIncrementalSign = 0;
    if (isSpeedSenseMode (physicsMode))
        speedSenseEma = -1.0;
}

void KnobPhysicsLabSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (! usingCustomDrag
        || physicsMode == PhysicsMode::linearReference
        || physicsMode == PhysicsMode::linearDuplicate
        || physicsMode == PhysicsMode::mixShowcase)
    {
        SmoothedSlider::mouseDrag (e);
        return;
    }

    const bool fine = isFinePointerDrag (e);
    const double Pbase = basePixelsForFullDrag (fine);

    const double newDy = static_cast<double> (customDragAnchor.y - e.position.y);
    const double ddy = newDy - lastAccumDy;
    lastAccumDy = newDy;

    if (std::abs (ddy) < 1.0e-12)
        return;

    const int sgn = ddy > 0 ? 1 : (ddy < 0 ? -1 : 0);
    const bool directionFlipped = (sgn != 0 && lastIncrementalSign != 0 && sgn != lastIncrementalSign);

    if (directionFlipped)
    {
        if (physicsMode == PhysicsMode::velvetRearmOnReverse)
            dragPeakAbsDy = 0.0;
        if (physicsMode == PhysicsMode::directionBandHeavy)
            pathSinceDirectionChange = 0.0;
    }

    if (sgn != 0)
        lastIncrementalSign = sgn;

    dragPeakAbsDy = juce::jmax (dragPeakAbsDy, std::abs (newDy));
    pathSinceDirectionChange += std::abs (ddy);

    double distanceKey = dragPeakAbsDy;
    if (physicsMode == PhysicsMode::directionBandHeavy)
        distanceKey = pathSinceDirectionChange;

    double m = pixelMultiplierAt (distanceKey, dragAccumProp);

    if (physicsMode == PhysicsMode::asymmetricUp)
    {
        if (ddy > 0.0)
            m *= 1.64;
        else
            m *= 0.76;
    }

    if (isSpeedSenseMode (physicsMode))
    {
        const double ad = std::abs (ddy);
        constexpr double alpha = 0.40;
        if (speedSenseEma < 0.0)
            speedSenseEma = ad;
        else
            speedSenseEma = alpha * ad + (1.0 - alpha) * speedSenseEma;
        m *= speedSenseMultiplier (physicsMode, speedSenseEma);
    }

    double delta = ddy / (Pbase * juce::jmax (1.0e-6, m));

    if (physicsMode == PhysicsMode::sprintBoost)
    {
        const double ad = std::abs (ddy);
        if (ad > 2.75)
            delta *= juce::jmin (1.42, 1.0 + (ad - 2.75) * 0.038);
    }

    dragAccumProp += delta;
    dragAccumProp = juce::jlimit (0.0, 1.0, dragAccumProp);
    setValue (proportionOfLengthToValue (dragAccumProp), juce::sendNotificationSync);
}

void KnobPhysicsLabSlider::mouseUp (const juce::MouseEvent& e)
{
    if (usingCustomDrag)
        usingCustomDrag = false;

    SmoothedSlider::mouseUp (e);
}
