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

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class SmoothedSlider; // Forward declaration

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;
    
    // Set color theme: 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    void setColorTheme(int colorIndex);
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;
    void setUiTextTypeface(juce::Typeface::Ptr typeface);
    void setPopupMenuFontHeight(float height) { popupMenuFontHeight = height; }
    
private:
    juce::Image knobBaseImage;
    juce::Image knobIndicatorImage;
    juce::Image knobShadowOverlayImage;
    juce::Image sliderTrackImage;
    juce::Image sliderThumbImage;
    juce::Image mixKnobImage;
    juce::Image knobSpriteSheetRateImage;
    juce::Image knobSpriteSheetDepthImage;
    juce::Image knobSpriteSheetOffsetImage;
    juce::Image knobSpriteSheetWidthImage;
    juce::Image mixKnobSpriteSheetImage;
    juce::Typeface::Ptr uiTextTypeface;
    float popupMenuFontHeight = 0.0f;
    
    int currentColorIndex = 0; // 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    
    void loadImages(int colorIndex);
    
    // Helper methods for loadImages
    void getImageDataForColor(int colorIndex, const char*& knobBaseName, int& knobBaseSize,
                              const char*& indicatorName, int& indicatorSize,
                              const char*& shadowName, int& shadowSize,
                              const char*& trackName, int& trackSize,
                              const char*& thumbName, int& thumbSize,
                              const char*& mixKnobName, int& mixKnobSize,
                              const char*& knobSheetRateName, int& knobSheetRateSize,
                              const char*& knobSheetDepthName, int& knobSheetDepthSize,
                              const char*& knobSheetOffsetName, int& knobSheetOffsetSize,
                              const char*& knobSheetWidthName, int& knobSheetWidthSize,
                              const char*& mixKnobSpriteSheetName, int& mixKnobSpriteSheetSize);
    
    // Helper methods for drawLinearSlider
    void drawSliderTrack(juce::Graphics& g, int x, int y, int width, int height);
    void drawSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                        float visualSliderPos);
    
    // Helper methods for drawToggleButton
    void drawToggleBorder(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isOn);
    void drawToggleBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isOn);
    void drawToggleIndicator(juce::Graphics& g, const juce::Rectangle<float>& innerBounds, bool isOn);
};
