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
