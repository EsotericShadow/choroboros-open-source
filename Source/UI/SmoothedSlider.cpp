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
    startTimerHz(120); // 120 FPS for smoother animation (reduced jankiness)
}

SmoothedSlider::~SmoothedSlider()
{
    stopTimer();
}

void SmoothedSlider::mouseDown(const juce::MouseEvent& e)
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag)
    {
        verticalDragStartY = e.position.y;
        valueAtVerticalDragStart = getValue();
    }
    juce::Slider::mouseDown(e);
}

void SmoothedSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (getSliderStyle() == juce::Slider::RotaryVerticalDrag)
    {
        const double sensitivity = static_cast<double>(juce::jmax(1, getHeight()));
        // Upward drag should increase value, downward drag should decrease value.
        const double proportionDelta = (verticalDragStartY - e.position.y) / sensitivity;
        double newProportion = valueToProportionOfLength(valueAtVerticalDragStart) + proportionDelta;
        newProportion = juce::jlimit(0.0, 1.0, newProportion);
        setValue(proportionOfLengthToValue(newProportion), juce::sendNotificationSync);
        valueAtVerticalDragStart = getValue();
        verticalDragStartY = e.position.y;
        return;
    }
    juce::Slider::mouseDrag(e);
}

void SmoothedSlider::mouseUp(const juce::MouseEvent& e)
{
    juce::Slider::mouseUp(e);
    if (onMouseUpCallback)
        onMouseUpCallback(e);
}

void SmoothedSlider::valueChanged()
{
    // When the actual value changes, update the target for visual smoothing
    if (useExponential)
    {
        // Exponential smoothing target is set in timerCallback
        needsRepaint = true;
    }
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
        // Exponential smoothing (one-pole filter) - matches audio smoothing
        float target = getValue();
        visualValueExp = visualValueExp * smoothingCoeff + target * (1.0f - smoothingCoeff);
        // Check if still smoothing (within 0.1% of target)
        isSmoothing = std::abs(visualValueExp - target) > 0.001f;
    }
    else
    {
        // Linear smoothing - advance by actual time delta for smoother motion
        // At 120Hz, each frame is ~8.33ms, so we advance by 2 samples per frame
        // This ensures smooth interpolation without jankiness
        visualValueLinear.skip(2);
        isSmoothing = visualValueLinear.isSmoothing();
    }
    
    if (needsRepaint || isSmoothing)
    {
        // Force repaint to show smoothed visual position
        repaint();
        needsRepaint = false;
    }
}

void SmoothedSlider::setSmoothingTime(float timeMs)
{
    smoothingTimeMs = timeMs;
    
    if (useExponential)
    {
        updateSmoothingCoeff();
    }
    else
    {
        visualValueLinear.reset(getSampleRate(), smoothingTimeMs * 0.001);
        visualValueLinear.setCurrentAndTargetValue(getValue());
    }
}

void SmoothedSlider::setUseExponential(bool useExp)
{
    if (useExponential != useExp)
    {
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
}

float SmoothedSlider::getVisualValue() const
{
    if (useExponential)
        return visualValueExp;
    else
        return visualValueLinear.getCurrentValue();
}

void SmoothedSlider::updateSmoothingCoeff()
{
    // Calculate one-pole filter coefficient for exponential smoothing
    // Time constant = smoothingTimeMs
    smoothingCoeff = std::exp(-1.0f / (smoothingTimeMs * 0.001f * getSampleRate()));
}
