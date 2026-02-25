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
#include <functional>

class AnimatedToggleButton : public juce::Slider, public juce::Timer
{
public:
    AnimatedToggleButton();
    ~AnimatedToggleButton() override = default;

    /** Returns 0..1 for animation progress toward ON state (1 = fully lit). Use to sync overlays. */
    float getAnimationProgress() const;

    /** Optional: called each animation tick so parent can repaint (e.g. for synced backpanel overlay). */
    std::function<void()> onAnimationTick;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void timerCallback() override;
    
private:
    void startAnimationToState(bool on);
    void commitToggleState(bool newState, juce::NotificationType notificationType = juce::sendNotificationSync);
    static constexpr int kNumFrames = 18;
    static constexpr int kCols = 5;
    static constexpr int kFramePx = 512;
    float animatedFrame = 0.0f;
    float animationStartFrame = 0.0f;
    float animationEndFrame = 0.0f;
    double animationStartMs = 0.0;
    double animationDurationMs = 185.0;
    bool animationRunning = false;
    int dragStartScreenY = 0;
    bool dragTriggered = false;
    bool pointerIsDown = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedToggleButton)
};
