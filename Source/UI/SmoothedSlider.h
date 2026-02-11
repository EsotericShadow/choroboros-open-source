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
    
    void setSmoothingTime(float timeMs);
    void setUseExponential(bool useExp);
    float getVisualValue() const;
    
private:
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
