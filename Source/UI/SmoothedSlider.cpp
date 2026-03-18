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


SmoothedSlider::SmoothedSlider(float smoothingTimeMs_, bool useExponential_)
    : smoothingTimeMs(smoothingTimeMs_), useExponential(useExponential_)
{
    if (useExponential)
    {
        updateSmoothingCoeff();
        visualValueExp = getValue();
    }
    else
    {
        visualValueLinear.reset(getSampleRate(), smoothingTimeMs * 0.001);
        visualValueLinear.setCurrentAndTargetValue(getValue());
    }
    startTimerHz(120);
}

SmoothedSlider::~SmoothedSlider()
{
    stopTimer();
}

void SmoothedSlider::mouseDown(const juce::MouseEvent& e)
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag)
        resetDragAnchor(e);

    resetWheelAnchor();
    juce::Slider::mouseDown(e);
}

void SmoothedSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag)
    {
        const bool fineAdjustActive = isFineWheelAdjustActive(e);
        if (fineAdjustActive != dragFineAdjustActive)
            resetDragAnchor(e);

        const double sensitivity = getDragSensitivity(e);
        const double pixelDelta = verticalDragStartY - e.position.y;

        if (std::abs(pixelDelta) < 0.5)
            return;

        const double proportionDelta = pixelDelta / sensitivity;
        double newProportion = valueToProportionOfLength(valueAtVerticalDragStart) + proportionDelta;
        newProportion = juce::jlimit(0.0, 1.0, newProportion);
        const double newValue = proportionOfLengthToValue(newProportion);

        const double oldValue = getValue();
        setValue(newValue, juce::sendNotificationSync);
        if (getValue() != oldValue)
        {
            valueAtVerticalDragStart = getValue();
            verticalDragStartY = e.position.y;
        }
        return;
    }
    juce::Slider::mouseDrag(e);
}

void SmoothedSlider::mouseUp(const juce::MouseEvent& e)
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag)
        setValue(getVisualValue(), juce::sendNotificationSync);

    snapVisualToValue();
    dragFineAdjustActive = false;
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

    // Apply delta directly — no accumulation queue.
    // Snap the visual immediately too: scroll wheel events arrive at OS cadence
    // which already creates natural motion. Adding SmoothedValue lag on top
    // causes the visual to lunge to the final position when the gesture ends.
    const double delta = computeDirectWheelDelta(e, wheel);
    wheelAnchorProportion = clampWheelAnchorProportion(wheelAnchorProportion + delta);
    setValue(proportionOfLengthToValue(wheelAnchorProportion), juce::sendNotificationSync);
    snapVisualToValue();

    wheelFineAdjustActive = isFineWheelAdjustActive(e);
    lastWheelDirection = wheelAmount;
    lastScrollTimeMs = now;
    needsRepaint = true;
}

void SmoothedSlider::valueChanged()
{
    if (useExponential)
        needsRepaint = true;
    else
    {
        visualValueLinear.setTargetValue(getValue());
        needsRepaint = true;
    }
}

void SmoothedSlider::timerCallback()
{
    bool isSmoothing = false;

    if (useExponential)
    {
        const float target = getValue();
        visualValueExp = visualValueExp * smoothingCoeff + target * (1.0f - smoothingCoeff);
        isSmoothing = std::abs(visualValueExp - target) > 0.001f;
    }
    else
    {
        visualValueLinear.skip(1);
        isSmoothing = visualValueLinear.isSmoothing();

        if (! isSmoothing)
        {
            const float target = static_cast<float>(getValue());
            if (std::abs(visualValueLinear.getCurrentValue() - target) > 1.0e-6f)
            {
                visualValueLinear.setCurrentAndTargetValue(target);
                needsRepaint = true;
            }
        }
    }

    if (lastScrollTimeMs > 0 && (juce::Time::currentTimeMillis() - lastScrollTimeMs) > 150)
    {
        resetWheelAnchor();
        lastScrollTimeMs = 0;
    }

    if (needsRepaint || isSmoothing)
    {
        repaint();
        needsRepaint = false;
    }
}

void SmoothedSlider::setSmoothingTime(float timeMs)
{
    smoothingTimeMs = timeMs;

    if (useExponential)
        updateSmoothingCoeff();
    else
    {
        visualValueLinear.reset(getSampleRate(), smoothingTimeMs * 0.001);
        visualValueLinear.setCurrentAndTargetValue(getValue());
    }
}

void SmoothedSlider::setUseExponential(bool useExp)
{
    if (useExponential == useExp)
        return;

    useExponential = useExp;

    if (useExponential)
    {
        updateSmoothingCoeff();
        visualValueExp = getValue();
    }
    else
    {
        visualValueLinear.reset(getSampleRate(), smoothingTimeMs * 0.001);
        visualValueLinear.setCurrentAndTargetValue(getValue());
    }
}

void SmoothedSlider::setDragSensitivity(float sensitivityScale)
{
    dragSensitivityScale = juce::jmax(0.01f, sensitivityScale);
}

void SmoothedSlider::setScrollWheelSensitivity(float sensitivityScale)
{
    scrollWheelScale = juce::jmax(0.01f, sensitivityScale);
}

float SmoothedSlider::getVisualValue() const
{
    if (useExponential)
        return visualValueExp;
    return visualValueLinear.getCurrentValue();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

double SmoothedSlider::clampWheelAnchorProportion(double proportion) const
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag && ! getRotaryParameters().stopAtEnd)
        return proportion - std::floor(proportion);

    return juce::jlimit(0.0, 1.0, proportion);
}

double SmoothedSlider::getDragSensitivity(const juce::MouseEvent& e) const
{
    const double base = juce::jmax(1.0, static_cast<double>(getHeight())
                                            / static_cast<double>(dragSensitivityScale));
    return isFineWheelAdjustActive(e) ? base * 3.0 : base;
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
    // Wheel sensitivity design (at factory default scrollWheelSensitivityPct = 25,
    // i.e. scrollWheelScale = 0.25):
    //
    //   kBaseScale     — linear multiplier applied before the cap.
    //   kNormalMaxStep — per-event cap for normal mode  → effective ~0.012/event
    //   kFineMaxStep   — per-event cap for Cmd+scroll  → effective ~0.003/event
    //
    // Fast flicks: many events arrive rapidly, each capped, so total movement is
    // proportional to gesture length — no runaway accumulation.
    // Inertial (momentum) events are already rejected in shouldIgnoreWheelEvent.
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

void SmoothedSlider::resetDragAnchor(const juce::MouseEvent& e)
{
    verticalDragStartY = e.position.y;
    valueAtVerticalDragStart = getValue();
    dragFineAdjustActive = isFineWheelAdjustActive(e);
}

void SmoothedSlider::resetWheelAnchor()
{
    hasWheelAnchor = false;
    lastWheelDirection = 0.0f;
    wheelFineAdjustActive = false;
}

void SmoothedSlider::updateSmoothingCoeff()
{
    smoothingCoeff = std::exp(-1.0f / (smoothingTimeMs * 0.001f * getSampleRate()));
}

void SmoothedSlider::snapVisualToValue()
{
    if (useExponential)
        visualValueExp = static_cast<float>(getValue());
    else
        visualValueLinear.setCurrentAndTargetValue(static_cast<float>(getValue()));

    needsRepaint = true;
}
