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

class AnimatedToggleButton : public juce::ToggleButton, public juce::Timer
{
public:
    AnimatedToggleButton() = default;
    ~AnimatedToggleButton() override = default;
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void timerCallback() override
    {
        // Update animation and repaint
        repaint();
    }
    
    float getAnimatedPosition() const
    {
        return animatedPosition;
    }
    
private:
    float animatedPosition = 0.0f;  // 0.0 = off, 1.0 = on
    static constexpr float animationSpeed = 0.15f;  // Higher = faster animation
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedToggleButton)
};
