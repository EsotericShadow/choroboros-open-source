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
 * A juce::Slider with custom scroll-wheel handling and configurable drag weight.
 *
 * The "heavy knob" feel comes from JUCE's native setMouseDragSensitivity() -
 * higher values = more mouse travel to turn the knob = deliberate, weighted feel.
 * NO visual smoothing, NO ghost/body desync. The knob, text, parameter, and
 * automation are always perfectly in sync.
 *
 * Custom behaviour:
 *   - Scroll wheel: inertial events ignored, per-event saturation cap,
 *     configurable sensitivity, Cmd/Ctrl fine-adjust mode.
 *   - onMouseUpCallback: used for the Rate knob's right-click sync menu.
 *   - setDragSensitivity: maps to JUCE's setMouseDragSensitivity.
 *
 * Compatibility note:
 *   - The open-source UI still calls setSmoothingTime() / setUseExponential()
 *     and reads getVisualValue(). These are kept as no-ops / passthrough so
 *     the beta build uses the same knob physics as commercial.
 */
class SmoothedSlider : public juce::Slider
{
public:
    SmoothedSlider(float smoothingTimeMs = 60.0f, bool useExponential = false);
    ~SmoothedSlider() override = default;

    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void setSmoothingTime(float) {}
    void setUseExponential(bool) {}
    void setDragSensitivity(float sensitivityScale);
    void setScrollWheelSensitivity(float sensitivityScale);
    float getVisualValue() const { return static_cast<float>(getValue()); }
    std::function<void(const juce::MouseEvent&)> onMouseUpCallback;

private:
    double wheelAnchorProportion = 0.0;
    float lastWheelDirection = 0.0f;
    bool hasWheelAnchor = false;
    float scrollWheelScale = 0.25f;
    int64_t lastScrollTimeMs = 0;

    double clampWheelAnchorProportion(double proportion) const;
    float getWheelAmount(const juce::MouseWheelDetails& wheel) const;
    bool isFineWheelAdjustActive(const juce::MouseEvent& e) const;
    bool shouldIgnoreWheelEvent(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) const;
    double computeDirectWheelDelta(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) const;
    void prepareWheelAnchor(float wheelAmount, int64_t now);
    void resetWheelAnchor();
};
