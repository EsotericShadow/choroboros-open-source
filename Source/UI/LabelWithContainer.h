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

class LabelWithContainer : public juce::Label, private juce::Timer
{
public:
    LabelWithContainer() = default;
    ~LabelWithContainer() override = default;
    
    void paint(juce::Graphics& g) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    // Set whether this is a value label (light grey background) or name label (dark background)
    void setValueLabelStyle(bool isValueLabel);
    bool isValueLabelStyleEnabled() const { return isValueLabelStyle; }
    
    // Set callback for when value is edited (called when text editor loses focus or Enter is pressed)
    // Returns true if value was applied successfully, false if invalid
    std::function<bool(const juce::String&)> onValueEdited;
    void setAnimatedValueText(const juce::String& text);
    void setFlipAnimationParams(bool enabled, int durationMs, float travelUpPx, float travelDownPx,
                                float travelOutScale, float travelInScale, float shearAmount, float minScale);
    void setValueFxParams(bool enabled,
                          float glowAlpha,
                          float glowSpreadPx,
                          float perCharOffsetX,
                          float perCharOffsetY,
                          float topAlpha,
                          float topOffsetX,
                          float topOffsetY,
                          float topShear,
                          float topRotateDeg,
                          float bottomAlpha,
                          float bottomOffsetX,
                          float bottomOffsetY,
                          float bottomShear,
                          float bottomRotateDeg,
                          float reflectBlurPx,
                          float reflectSquash,
                          float reflectMotionAmount);
    
    // Set the text editor color (for when editing value labels)
    void setEditorTextColor(juce::Colour color);
    
private:
    bool isValueLabelStyle = false;
    bool isEditing = false;
    juce::String pendingFormattedText;  // Store formatted text to apply after editor hides
    juce::Colour editorTextColor = juce::Colour(0xff9dbd78);  // Default green, will be updated
    bool isAnimatingFlip = false;
    float flipProgress = 1.0f;
    double flipStartTimeMs = 0.0;
    int flipDurationMs = 140;
    float flipTravelUpPx = 3.0f;
    float flipTravelDownPx = 3.0f;
    float flipTravelOutScale = 1.0f;
    float flipTravelInScale = 1.0f;
    float flipShearAmount = 0.32f;
    float flipMinScale = 0.35f;
    bool flipAnimationEnabled = true;
    bool flipHasVisualEffect = true;
    bool valueFxEnabled = true;
    float valueGlowAlpha = 0.035f;
    float valueGlowSpreadPx = 0.65f;
    float valueFxPerCharOffsetX = 0.0f;
    float valueFxPerCharOffsetY = 0.0f;
    float valueTopReflectAlpha = 0.018f;
    float valueTopReflectOffsetX = 0.0f;
    float valueTopReflectOffsetY = -0.8f;
    float valueTopReflectShear = 0.06f;
    float valueTopReflectRotateDeg = 0.0f;
    float valueBottomReflectAlpha = 0.026f;
    float valueBottomReflectOffsetX = 0.0f;
    float valueBottomReflectOffsetY = 1.1f;
    float valueBottomReflectShear = -0.08f;
    float valueBottomReflectRotateDeg = 0.0f;
    float valueReflectBlurPx = 0.7f;
    float valueReflectSquash = 0.35f;
    float valueReflectMotionAmount = 0.20f;
    int flipDirection = 0; // +1 value up, -1 value down
    juce::String flipFromText;
    juce::String flipToText;
    juce::String flipFromMappedText;
    juce::Array<int> flippingCharIndices;
    void editorShown(juce::TextEditor* editor) override;
    void editorAboutToBeHidden(juce::TextEditor* editor) override;
    juce::TextEditor* createEditorComponent() override;
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelWithContainer)
};
