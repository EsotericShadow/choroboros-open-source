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

#include "SmoothedSlider.h"
#include <cmath>

SmoothedSlider::SmoothedSlider(float, bool) {}

void SmoothedSlider::mouseUp(const juce::MouseEvent& e)
{
    juce::Slider::mouseUp(e);
    if (onMouseUpCallback)
        onMouseUpCallback(e);
}

void SmoothedSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (shouldIgnoreWheelEvent(e, wheel))
        return;

    const float wheelAmount = getWheelAmount(wheel);
    if (std::abs(wheelAmount) < 1.0e-6f)
        return;

    const auto now = juce::Time::currentTimeMillis();
    prepareWheelAnchor(wheelAmount, now);

    const double delta = computeDirectWheelDelta(e, wheel);
    wheelAnchorProportion = clampWheelAnchorProportion(wheelAnchorProportion + delta);
    setValue(proportionOfLengthToValue(wheelAnchorProportion), juce::sendNotificationSync);

    lastWheelDirection = wheelAmount;
    lastScrollTimeMs = now;
}

void SmoothedSlider::setDragSensitivity(float sensitivityScale)
{
    // JUCE's setMouseDragSensitivity: higher = more pixels to traverse full range = heavier.
    // sensitivityScale ~1.0 = default, <1.0 = heavier, >1.0 = lighter.
    const int baseSensitivity = 250;
    const int scaled = juce::jmax(50, static_cast<int>(static_cast<float>(baseSensitivity) / juce::jmax(0.01f, sensitivityScale)));
    setMouseDragSensitivity(scaled);
}

void SmoothedSlider::setScrollWheelSensitivity(float sensitivityScale)
{
    scrollWheelScale = juce::jmax(0.01f, sensitivityScale);
}

// ---------------------------------------------------------------------------
// Scroll wheel helpers
// ---------------------------------------------------------------------------

double SmoothedSlider::clampWheelAnchorProportion(double proportion) const
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag && ! getRotaryParameters().stopAtEnd)
        return proportion - std::floor(proportion);

    return juce::jlimit(0.0, 1.0, proportion);
}

float SmoothedSlider::getWheelAmount(const juce::MouseWheelDetails& wheel) const
{
    const float dominant = std::abs(wheel.deltaX) > std::abs(wheel.deltaY)
                               ? -wheel.deltaX
                               : wheel.deltaY;
    return dominant * (wheel.isReversed ? -1.0f : 1.0f);
}

bool SmoothedSlider::isFineWheelAdjustActive(const juce::MouseEvent& e) const
{
    const auto live = juce::ModifierKeys::getCurrentModifiersRealtime();
    return e.mods.isCommandDown() || live.isCommandDown()
        || e.mods.isCtrlDown()   || live.isCtrlDown();
}

bool SmoothedSlider::shouldIgnoreWheelEvent(const juce::MouseEvent& e,
                                            const juce::MouseWheelDetails& wheel) const
{
    return ! isScrollWheelEnabled() || e.mods.isAnyMouseButtonDown() || wheel.isInertial;
}

double SmoothedSlider::computeDirectWheelDelta(const juce::MouseEvent& e,
                                               const juce::MouseWheelDetails& wheel) const
{
    static constexpr double kBaseScale     = 1.60;
    static constexpr double kNormalMaxStep = 0.048;
    static constexpr double kFineMaxStep   = 0.012;

    const bool isFine = isFineWheelAdjustActive(e);
    const double maxStep = isFine ? kFineMaxStep : kNormalMaxStep;
    const double scaled  = static_cast<double>(getWheelAmount(wheel)) * kBaseScale;
    const double capped  = std::copysign(std::min(std::abs(scaled), maxStep), scaled);
    return capped * static_cast<double>(scrollWheelScale);
}

void SmoothedSlider::prepareWheelAnchor(float wheelAmount, int64_t now)
{
    const bool directionChanged = (lastWheelDirection > 0.0f && wheelAmount < 0.0f)
                               || (lastWheelDirection < 0.0f && wheelAmount > 0.0f);

    if (! hasWheelAnchor || directionChanged || (now - lastScrollTimeMs) > 150)
    {
        wheelAnchorProportion = valueToProportionOfLength(getValue());
        hasWheelAnchor = true;
    }
}

void SmoothedSlider::resetWheelAnchor()
{
    hasWheelAnchor = false;
    lastWheelDirection = 0.0f;
}
