/*
 * Choroboros - A chorus that eats its own tail
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "SmoothedSlider.h"

/**
 * Isolated lab control: rotary drag with selectable “physics” curves.
 * No post-release easing — value tracks only while the mouse moves, and stops
 * immediately on mouse up. Cmd/Ctrl increases drag precision (fine mode).
 *
 * Mode 0 defers to JUCE / SmoothedSlider default (linear vs vertical travel).
 */
class KnobPhysicsLabSlider : public SmoothedSlider
{
public:
    enum class PhysicsMode : int
    {
        linearReference = 0,
        stiffThenLinear,
        stiffEaseLinear,
        alwaysHeavy,
        quickStart,
        edgeHeavy,
        stiffSnapTight,
        stiffEaseFast,
        linearDuplicate,
        velvetClutch,
        centerVault,
        endMagnets,
        sprintBoost,
        iceThenFire,
        mixShowcase,
        // Additional lab experiments
        velvetClutchDramatic,   // stronger velvet, peak-stable
        velvetRearmOnReverse,   // resets clutch peak each time drag direction flips
        directionBandHeavy,     // heavy for first px of path after each reversal
        tripleZonePeak,         // three peak-distance stiffness zones
        barrierExtremeEnds,     // very strong near 0% / 100%
        stickyMidBand,          // heavy through middle 24% of value range
        asymmetricUp,           // up-drag stiffer than down-drag
        speedSensitiveA,        // blue bank: smooth speed→weight + EMA (balanced)
        speedSensitiveB,        // gentler curve, longer blend
        speedSensitiveC,        // stronger slow/heavy vs fast/light
        speedSensitiveD,        // very long silky blend
        valueDetents,           // soft notches every 10%
        depthQuadWell,          // stiffer away from 50% (quadratic well)
        heavyVise,              // always very heavy
        featherTouch            // always very light
    };

    explicit KnobPhysicsLabSlider (PhysicsMode mode);

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    PhysicsMode physicsMode;

    bool usingCustomDrag = false;
    double dragAccumProp = 0.0;
    juce::Point<float> customDragAnchor {};
    double lastAccumDy = 0.0;
    /** Max |vertical delta| this gesture; peak-stable clutch curves. */
    double dragPeakAbsDy = 0.0;
    /** Path length (sum of |ddy|) since last direction flip — directionBandHeavy. */
    double pathSinceDirectionChange = 0.0;
    /** Sign of last non-zero incremental drag (-1, 0, +1). */
    int lastIncrementalSign = 0;
    /** Smoothed |Δy| per tick for speed-sense modes (<0 = not initialised). */
    double speedSenseEma = -1.0;

    static bool isFinePointerDrag (const juce::MouseEvent& e);
    double basePixelsForFullDrag (bool fine) const;
    double pixelMultiplierAt (double distanceKey, double currentProp) const;
};
