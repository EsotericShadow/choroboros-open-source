#include "AnimatedToggleButton.h"

void AnimatedToggleButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    // Animate towards target state
    const bool targetState = getToggleState();
    const float targetPosition = targetState ? 1.0f : 0.0f;
    
    // Smooth interpolation towards target
    const float diff = targetPosition - animatedPosition;
    animatedPosition += diff * animationSpeed;
    
    // Start/stop timer based on whether we're still animating
    if (std::abs(diff) > 0.001f)
    {
        if (!isTimerRunning())
            startTimerHz(60);  // 60 FPS for smooth animation
    }
    else
    {
        animatedPosition = targetPosition;  // Snap to final position
        if (isTimerRunning())
            stopTimer();
    }
    
    // Let the LookAndFeel draw the button, but we'll pass the animated position
    // For now, we'll use a custom drawing approach
    const auto bounds = getLocalBounds().toFloat();
    const float cornerRadius = 6.0f;
    const float borderWidth = 1.0f;
    const bool isOn = animatedPosition > 0.5f;  // Use animated position for visual state
    
    // Create gradient for border (green theme when on, grey when off)
    juce::ColourGradient gradient;
    if (isOn)
    {
        // Green gradient when on (matching label style)
        gradient = juce::ColourGradient(
            juce::Colour(0xff4a6b5a), bounds.getX(), bounds.getY(),  // Top-left: lighter green
            juce::Colour(0xff2a3b32), bounds.getX(), bounds.getBottom(), // Bottom: darker green
            false
        );
    }
    else
    {
        // Grey gradient when off
        gradient = juce::ColourGradient(
            juce::Colour(0xff606060), bounds.getX(), bounds.getY(),  // Top-left: lighter grey
            juce::Colour(0xff404040), bounds.getX(), bounds.getBottom(), // Bottom: darker grey
            false
        );
    }
    
    // Draw border with gradient
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Draw inner background
    const auto innerBounds = bounds.reduced(borderWidth);
    if (isOn)
    {
        // Green-tinted background when on
        g.setColour(juce::Colour(0xff2a4a3a).withAlpha(0.9f));
    }
    else
    {
        // Dark background when off
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
    }
    g.fillRoundedRectangle(innerBounds, cornerRadius - borderWidth);
    
    // Draw toggle indicator (circle that slides) - use animated position
    const float indicatorSize = innerBounds.getHeight() * 0.7f;
    const float indicatorY = innerBounds.getCentreY() - indicatorSize * 0.5f;
    
    // Interpolate position between left (off) and right (on)
    const float leftX = innerBounds.getX() + 4.0f;
    const float rightX = innerBounds.getRight() - indicatorSize - 4.0f;
    const float indicatorX = leftX + animatedPosition * (rightX - leftX);
    
    // Draw indicator with gradient (interpolate color based on position)
    juce::ColourGradient indicatorGradient;
    if (isOn)
    {
        // Bright green when on
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff9dbd78), indicatorX, indicatorY,  // Top: lighter green (#9dbd78)
            juce::Colour(0xff6b8d5a), indicatorX, indicatorY + indicatorSize, // Bottom: darker green
            false
        );
    }
    else
    {
        // Grey when off
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff808080), indicatorX, indicatorY,  // Top: lighter grey
            juce::Colour(0xff505050), indicatorX, indicatorY + indicatorSize, // Bottom: darker grey
            false
        );
    }
    
    g.setGradientFill(indicatorGradient);
    g.fillEllipse(indicatorX, indicatorY, indicatorSize, indicatorSize);
    
    // Add subtle highlight
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillEllipse(indicatorX + 1.0f, indicatorY + 1.0f, indicatorSize * 0.3f, indicatorSize * 0.3f);
}
