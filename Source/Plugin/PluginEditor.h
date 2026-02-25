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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "../UI/CustomLookAndFeel.h"
#include "../UI/LabelWithContainer.h"
#include "../UI/SmoothedSlider.h"
#include "../UI/AnimatedToggleButton.h"
#include "../UI/PluginEditorSetup.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
*/
class ChoroborosPluginEditor  : public juce::AudioProcessorEditor,
                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChoroborosPluginEditor (ChoroborosAudioProcessor&);
    ~ChoroborosPluginEditor() override;
    static constexpr float kUiScale = 1.16875f;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void applyLayout();
    void applyTuningToUI();
    void refreshValueLabels();
    float getUiScale() const { return kUiScale; }
    const LayoutTuning& getLayoutTuning() const { return layoutTuning; }
    LayoutTuning& getLayoutTuning() { return layoutTuning; }
    juce::Font makeValueLabelFont(float heightPx, bool bold = true) const;
    juce::Font makeUiTextFont(float heightPx, bool bold = true) const;

private:
    ChoroborosAudioProcessor& audioProcessor;
    LayoutTuning layoutTuning;
    
    CustomLookAndFeel customLookAndFeel;
    juce::Image backgroundImage;
    juce::Image backgroundImageLit;  // Light-on overlay (opacity synced to HQ switch) for all themes
    
    // Sliders (with visual smoothing for natural feel)
    SmoothedSlider rateSlider;
    SmoothedSlider depthSlider;  // Will use 50ms to match audio smoothing
    SmoothedSlider offsetSlider;
    SmoothedSlider widthSlider;
    SmoothedSlider colorSlider;
    SmoothedSlider mixSlider;
    
    // Labels (with containers)
    LabelWithContainer rateLabel;
    LabelWithContainer depthLabel;
    LabelWithContainer offsetLabel;
    LabelWithContainer widthLabel;
    LabelWithContainer colorLabel;
    LabelWithContainer mixLabel;
    
    // Value labels (with containers)
    LabelWithContainer rateValueLabel;
    LabelWithContainer depthValueLabel;
    LabelWithContainer offsetValueLabel;
    LabelWithContainer widthValueLabel;
    LabelWithContainer colorValueLabel;
    LabelWithContainer mixValueLabel;
    
    // Engine Color selector
    juce::ComboBox engineColorBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineColorAttachment;
    
    // HQ toggle button (with animation)
    AnimatedToggleButton hqButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hqAttachment;
    LabelWithContainer hqLabel;
    
    // Feedback button (alpha version)
    juce::TextButton feedbackButton;
    
    // Help and About buttons
    juce::TextButton helpButton;
    juce::TextButton aboutButton;
    juce::TextButton devButton;
    std::unique_ptr<juce::DocumentWindow> devWindow;
    
    // Tooltip window
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    juce::Typeface::Ptr valueLabelTypeface;
    juce::Typeface::Ptr uiTextTypeface;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> offsetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    void loadBackgroundImage(int colorIndex = 0);
    void loadValueLabelTypeface();
    void loadUiTextTypeface();
    void updateValueLabelColors(int colorIndex);
    void setupSlider(juce::Slider& slider, LabelWithContainer& label, LabelWithContainer& valueLabel,
                     const juce::String& name, const juce::String& paramId);
    void updateValueLabel(LabelWithContainer& label, float value, const juce::String& paramId);
    void setupValueLabelEditing(LabelWithContainer& label, juce::Slider& slider, const juce::String& paramId);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    float parseValueFromText(const juce::String& text, const juce::String& paramId);
    
    // Helper to calculate label width with 8px padding on each side
    int calculateLabelWidth(const juce::String& text, const juce::Font& font) const;
    
    // Constructor helper methods
    void setupEngineColorSelector();
    void setupSliderAttachments();
    void setupSliderValueChangeListeners();
    double getHostBpm() const;
    void showRateSyncMenu(juce::Slider& rateControl);
    
    // parseValueFromText helper methods
    float parseRateValue(const juce::String& trimmed);
    float parseDepthValue(const juce::String& trimmed);
    float parseOffsetValue(const juce::String& trimmed);
    float parseWidthValue(const juce::String& trimmed);
    float parseColorValue(const juce::String& trimmed);
    float parseMixValue(const juce::String& trimmed);
    
    // Make members accessible to setup helper
    friend class PluginEditorSetup;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoroborosPluginEditor)
};
