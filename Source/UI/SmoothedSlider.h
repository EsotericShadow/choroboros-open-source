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

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * A slider with visual smoothing for a more natural, weighted feel.
 * The actual parameter value updates immediately, but the visual position
 * follows smoothly with a slight drag/weight effect.
 * 
 * Supports both linear and exponential smoothing to match audio processing.
 */
class SmoothedSlider : public juce::Slider, public juce::Timer
{
public:
    SmoothedSlider(float smoothingTimeMs = 60.0f, bool useExponential = false);
    ~SmoothedSlider() override;
    
    void valueChanged() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSmoothingTime(float timeMs);
    void setUseExponential(bool useExp);
    float getVisualValue() const;
    std::function<void(const juce::MouseEvent&)> onMouseUpCallback;
    
private:
    float verticalDragStartY = 0.0f;
    double valueAtVerticalDragStart = 0.0;

    // Linear smoothing (default)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> visualValueLinear;
    
    // Exponential smoothing (one-pole, for depth knob to match audio)
    float visualValueExp = 0.5f;
    float smoothingCoeff = 0.0f;
    
    float smoothingTimeMs = 60.0f;
    bool useExponential = false;
    bool needsRepaint = false;
    
    float getSampleRate() const { return 120.0f; } // 120 FPS for visual updates (smoother animation)
    void updateSmoothingCoeff();
};
