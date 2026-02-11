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
