#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "../UI/CustomLookAndFeel.h"
#include "../UI/LabelWithContainer.h"
#include "../UI/SmoothedSlider.h"
#include "../UI/AnimatedToggleButton.h"
#include "FeedbackDialog.h"
#include "AboutDialog.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
*/
class ChoroborosPluginEditor  : public juce::AudioProcessorEditor
{
public:
    ChoroborosPluginEditor (ChoroborosAudioProcessor&);
    ~ChoroborosPluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ChoroborosAudioProcessor& audioProcessor;
    
    CustomLookAndFeel customLookAndFeel;
    juce::Image backgroundImage;
    
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hqAttachment;
    LabelWithContainer hqLabel;
    
    // Feedback button (alpha version)
    juce::TextButton feedbackButton;
    
    // Help and About buttons
    juce::TextButton helpButton;
    juce::TextButton aboutButton;
    
    // Tooltip window
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> offsetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    void loadBackgroundImage(int colorIndex = 0);
    void updateValueLabelColors(int colorIndex);
    void setupSlider(juce::Slider& slider, LabelWithContainer& label, LabelWithContainer& valueLabel,
                     const juce::String& name, const juce::String& paramId);
    void updateValueLabel(LabelWithContainer& label, float value, const juce::String& paramId);
    void setupValueLabelEditing(LabelWithContainer& label, juce::Slider& slider, const juce::String& paramId);
    float parseValueFromText(const juce::String& text, const juce::String& paramId);
    
    // Helper to calculate label width with 8px padding on each side
    int calculateLabelWidth(const juce::String& text, const juce::Font& font) const;
    
    // Constructor helper methods
    void setupEngineColorSelector();
    void setupSliderAttachments();
    void setupSliderValueChangeListeners();
    
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
