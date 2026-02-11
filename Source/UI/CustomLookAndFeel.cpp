#include "CustomLookAndFeel.h"
#include "SmoothedSlider.h"
#include "BinaryData.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    loadImages(0); // Default to Green
}

void CustomLookAndFeel::setColorTheme(int colorIndex)
{
    colorIndex = juce::jlimit(0, 3, colorIndex);
    if (currentColorIndex != colorIndex)
    {
        currentColorIndex = colorIndex;
        loadImages(colorIndex);
    }
}

void CustomLookAndFeel::loadImages(int colorIndex)
{
    const char* knobName = nullptr;
    int knobSize = 0;
    const char* trackName = nullptr;
    int trackSize = 0;
    const char* thumbName = nullptr;
    int thumbSize = 0;
    const char* mixKnobName = nullptr;
    int mixKnobSize = 0;
    
    getImageDataForColor(colorIndex, knobName, knobSize, trackName, trackSize, thumbName, thumbSize, mixKnobName, mixKnobSize);
    
    if (knobName && knobSize > 0)
        knobBaseImage = juce::ImageCache::getFromMemory(knobName, knobSize);
    
    if (trackName && trackSize > 0)
        sliderTrackImage = juce::ImageCache::getFromMemory(trackName, trackSize);
    
    if (thumbName && thumbSize > 0)
        sliderThumbImage = juce::ImageCache::getFromMemory(thumbName, thumbSize);
    
    if (mixKnobName && mixKnobSize > 0)
        mixKnobImage = juce::ImageCache::getFromMemory(mixKnobName, mixKnobSize);
}

void CustomLookAndFeel::getImageDataForColor(int colorIndex, const char*& knobName, int& knobSize,
                                             const char*& trackName, int& trackSize,
                                             const char*& thumbName, int& thumbSize,
                                             const char*& mixKnobName, int& mixKnobSize)
{
    if (colorIndex == 0) // Green
    {
        knobName = BinaryData::green_knob_base_png;
        knobSize = BinaryData::green_knob_base_pngSize;
        trackName = BinaryData::green_slider_track_png;
        trackSize = BinaryData::green_slider_track_pngSize;
        thumbName = BinaryData::green_slider_thumb_png;
        thumbSize = BinaryData::green_slider_thumb_pngSize;
        mixKnobName = BinaryData::green_mix_knob_png;
        mixKnobSize = BinaryData::green_mix_knob_pngSize;
    }
    else if (colorIndex == 1) // Blue
    {
        knobName = BinaryData::blue_knob_base_png;
        knobSize = BinaryData::blue_knob_base_pngSize;
        trackName = BinaryData::blue_slider_track_png;
        trackSize = BinaryData::blue_slider_track_pngSize;
        thumbName = BinaryData::blue_slider_thumb_png;
        thumbSize = BinaryData::blue_slider_thumb_pngSize;
        mixKnobName = BinaryData::blue_mix_knob_png;
        mixKnobSize = BinaryData::blue_mix_knob_pngSize;
    }
    else if (colorIndex == 2) // Red
    {
        knobName = BinaryData::red_knob_base_png;
        knobSize = BinaryData::red_knob_base_pngSize;
        trackName = BinaryData::red_slider_track_png;
        trackSize = BinaryData::red_slider_track_pngSize;
        thumbName = BinaryData::red_slider_thumb_png;
        thumbSize = BinaryData::red_slider_thumb_pngSize;
        mixKnobName = BinaryData::red_mix_knob_png;
        mixKnobSize = BinaryData::red_mix_knob_pngSize;
    }
    else // Purple (colorIndex == 3)
    {
        knobName = BinaryData::purple_knob_base_png;
        knobSize = BinaryData::purple_knob_base_pngSize;
        trackName = BinaryData::purple_slider_track_png;
        trackSize = BinaryData::purple_slider_track_pngSize;
        thumbName = BinaryData::purple_slider_thumb_png;
        thumbSize = BinaryData::purple_slider_thumb_pngSize;
        mixKnobName = BinaryData::purple_mix_knob_png;
        mixKnobSize = BinaryData::purple_mix_knob_pngSize;
    }
}

// Codacy: Parameter count warning unavoidable - this is a JUCE override method
// that must match the base class signature (LookAndFeel_V4::drawRotarySlider)
void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    // Check if this is the mix knob (by component name)
    const bool isMixKnob = slider.getName() == "Mix" || slider.getComponentID() == "Mix";
    juce::Image& knobImage = (isMixKnob && mixKnobImage.isValid()) ? mixKnobImage : knobBaseImage;
    
    if (knobImage.isValid())
    {
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        
        // Use smoothed visual value if this is a SmoothedSlider
        float visualSliderPos = sliderPos;
        if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
        {
            // Recalculate sliderPos based on smoothed visual value
            const double minValue = slider.getMinimum();
            const double maxValue = slider.getMaximum();
            const double visualValue = smoothedSlider->getVisualValue();
            visualSliderPos = static_cast<float>((visualValue - minValue) / (maxValue - minValue));
            visualSliderPos = juce::jlimit(0.0f, 1.0f, visualSliderPos);
        }
        
        // Calculate rotation angle: knob points north (0°) as origin
        // visualSliderPos is 0.0 to 1.0, map to rotaryStartAngle to rotaryEndAngle
        const float angle = rotaryStartAngle + visualSliderPos * (rotaryEndAngle - rotaryStartAngle);
        
        // Rotate the knob image around its centre
        // The knob image itself points north, so we rotate it by the calculated angle
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
        g.drawImage(knobImage, x, y, width, height, 0, 0, 
                   knobImage.getWidth(), knobImage.getHeight());
        g.restoreState();
    }
    else
    {
        // Fallback to default
        LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPos, 
                                        rotaryStartAngle, rotaryEndAngle, slider);
    }
}

// Codacy: Parameter count warning unavoidable - this is a JUCE override method
// that must match the base class signature (LookAndFeel_V4::drawLinearSlider)
void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        drawSliderTrack(g, x, y, width, height);
        
        // Calculate visual slider position (with smoothing if applicable)
        float visualSliderPos = sliderPos;
        if (auto* smoothedSlider = dynamic_cast<SmoothedSlider*>(&slider))
        {
            const double minValue = slider.getMinimum();
            const double maxValue = slider.getMaximum();
            const double visualValue = smoothedSlider->getVisualValue();
            const float normalized = static_cast<float>((visualValue - minValue) / (maxValue - minValue));
            const float clampedNormalized = juce::jlimit(0.0f, 1.0f, normalized);
            visualSliderPos = x + clampedNormalized * width;
        }
        
        drawSliderThumb(g, x, y, width, height, visualSliderPos);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                        minSliderPos, maxSliderPos, style, slider);
    }
}

void CustomLookAndFeel::drawSliderTrack(juce::Graphics& g, int x, int y, int width, int height)
{
    if (sliderTrackImage.isValid())
    {
        g.drawImage(sliderTrackImage, x, y, width, height, 0, 0,
                   sliderTrackImage.getWidth(), sliderTrackImage.getHeight());
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.fillRoundedRectangle(x, y + height * 0.4f, width, height * 0.2f, 2.0f);
    }
}

void CustomLookAndFeel::drawSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                                       float visualSliderPos)
{
    const float constraintOffset = width * 0.115f;
    
    const float juceMinPos = x;
    const float juceMaxPos = x + width;
    const float actualMinPos = x + constraintOffset;
    const float actualMaxPos = x + width - constraintOffset;
    
    float constrainedSliderPos;
    if (juceMaxPos > juceMinPos)
    {
        const float normalized = (visualSliderPos - juceMinPos) / (juceMaxPos - juceMinPos);
        constrainedSliderPos = actualMinPos + normalized * (actualMaxPos - actualMinPos);
    }
    else
    {
        constrainedSliderPos = actualMinPos;
    }
    
    // Calculate thumb size maintaining original aspect ratio (40x80 = 1:2)
    float thumbWidth, thumbHeight;
    if (sliderThumbImage.isValid())
    {
        const float imageAspectRatio = static_cast<float>(sliderThumbImage.getWidth()) / static_cast<float>(sliderThumbImage.getHeight());
        const float maxThumbHeight = height * 0.8f;
        const float maxThumbWidth = juce::jmin(width * 0.1f, 30.0f);
        
        // Calculate dimensions maintaining aspect ratio
        thumbHeight = maxThumbHeight;
        thumbWidth = thumbHeight * imageAspectRatio;
        
        // If width exceeds max, scale down
        if (thumbWidth > maxThumbWidth)
        {
            thumbWidth = maxThumbWidth;
            thumbHeight = thumbWidth / imageAspectRatio;
        }
    }
    else
    {
        // Fallback if no image
        const float baseThumbWidth = juce::jmin(width * 0.1f, 30.0f);
        thumbWidth = baseThumbWidth * 0.8f;
        thumbHeight = height * 0.8f;
    }
    
    const float thumbX = constrainedSliderPos - thumbWidth * 0.5f;
    const float thumbY = y + (height - thumbHeight) * 0.5f;
    
    if (sliderThumbImage.isValid())
    {
        g.drawImage(sliderThumbImage, thumbX, thumbY, thumbWidth, thumbHeight,
                   0, 0, sliderThumbImage.getWidth(), sliderThumbImage.getHeight());
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);
    }
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const float borderWidth = 1.0f;
    const bool isOn = button.getToggleState();
    
    drawToggleBorder(g, bounds, isOn);
    
    const auto innerBounds = bounds.reduced(borderWidth);
    drawToggleBackground(g, innerBounds, isOn);
    drawToggleIndicator(g, innerBounds, isOn);
}

void CustomLookAndFeel::drawToggleBorder(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isOn)
{
    const float cornerRadius = 6.0f;
    juce::ColourGradient gradient;
    
    if (isOn)
    {
        gradient = juce::ColourGradient(
            juce::Colour(0xff4a6b5a), bounds.getX(), bounds.getY(),
            juce::Colour(0xff2a3b32), bounds.getX(), bounds.getBottom(), false);
    }
    else
    {
        gradient = juce::ColourGradient(
            juce::Colour(0xff606060), bounds.getX(), bounds.getY(),
            juce::Colour(0xff404040), bounds.getX(), bounds.getBottom(), false);
    }
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

void CustomLookAndFeel::drawToggleBackground(juce::Graphics& g, const juce::Rectangle<float>& innerBounds, bool isOn)
{
    const float cornerRadius = 5.0f; // 6.0f - 1.0f border
    
    if (isOn)
        g.setColour(juce::Colour(0xff2a4a3a).withAlpha(0.9f));
    else
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
    
    g.fillRoundedRectangle(innerBounds, cornerRadius);
}

void CustomLookAndFeel::drawToggleIndicator(juce::Graphics& g, const juce::Rectangle<float>& innerBounds, bool isOn)
{
    const float indicatorSize = innerBounds.getHeight() * 0.7f;
    const float indicatorY = innerBounds.getCentreY() - indicatorSize * 0.5f;
    const float indicatorX = isOn 
        ? (innerBounds.getRight() - indicatorSize - 4.0f)
        : (innerBounds.getX() + 4.0f);
    
    juce::ColourGradient indicatorGradient;
    if (isOn)
    {
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff9dbd78), indicatorX, indicatorY,
            juce::Colour(0xff6b8d5a), indicatorX, indicatorY + indicatorSize, false);
    }
    else
    {
        indicatorGradient = juce::ColourGradient(
            juce::Colour(0xff808080), indicatorX, indicatorY,
            juce::Colour(0xff505050), indicatorX, indicatorY + indicatorSize, false);
    }
    
    g.setGradientFill(indicatorGradient);
    g.fillEllipse(indicatorX, indicatorY, indicatorSize, indicatorSize);
    
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillEllipse(indicatorX + 1.0f, indicatorY + 1.0f, indicatorSize * 0.3f, indicatorSize * 0.3f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    // Don't draw background - let it be transparent
    // JUCE will draw the text automatically, we just need to draw the arrow
    // Draw the dropdown arrow (simple triangle)
    juce::Path arrow;
    const float arrowSize = 6.0f;
    const float arrowX = static_cast<float>(buttonX + buttonW / 2);
    const float arrowY = static_cast<float>(buttonY + buttonH / 2);
    
    g.setColour(juce::Colours::white);
    
    if (isButtonDown)
    {
        // Arrow pointing up when open
        arrow.addTriangle(arrowX, arrowY - arrowSize / 2,
                         arrowX - arrowSize / 2, arrowY + arrowSize / 2,
                         arrowX + arrowSize / 2, arrowY + arrowSize / 2);
    }
    else
    {
        // Arrow pointing down when closed
        arrow.addTriangle(arrowX, arrowY + arrowSize / 2,
                         arrowX - arrowSize / 2, arrowY - arrowSize / 2,
                         arrowX + arrowSize / 2, arrowY - arrowSize / 2);
    }
    
    g.fillPath(arrow);
}
