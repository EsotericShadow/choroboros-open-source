#include "PluginEditorSetup.h"
#include "../Plugin/PluginEditor.h"

void PluginEditorSetup::setupSliders(ChoroborosPluginEditor& editor)
{
    // Set exact bounds - knobs 10% smaller (135x135 instead of 150x150)
    editor.rateSlider.setBounds(45 + 7, 120 + 7, 135, 135);
    editor.depthSlider.setBounds(255 + 7, 120 + 7, 135, 135);
    editor.offsetSlider.setBounds(45 + 7, 310 + 7 + 16, 135, 135);
    editor.widthSlider.setBounds(255 + 7, 310 + 7 + 16, 135, 135);
    editor.colorSlider.setBounds(75, 520, 300, 70); // Moved up 24px (was 544)
    // Mix knob in bottom right corner - 20% smaller than actual image size (50 * 0.8 = 40px)
    const int mixKnobSize = 40; // 20% smaller than 50px
    const int mixKnobX = 450 - mixKnobSize - 20 - 32 + 8 + 3; // Move 32px left, then 8px right, then 3px right
    const int mixKnobY = 700 - mixKnobSize - 20 - 24 - 24; // Move up 24px more (was -24, now -48 total)
    editor.mixSlider.setBounds(mixKnobX, mixKnobY, mixKnobSize, mixKnobSize);
    
    // Configure slider styles
    editor.rateSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.depthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.offsetSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.widthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    editor.colorSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    editor.mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    
    editor.rateSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.depthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.offsetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.widthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.colorSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    editor.mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    // Set mix slider name so CustomLookAndFeel can identify it
    editor.mixSlider.setName("Mix");
    
    // Set visual smoothing times
    editor.rateSlider.setUseExponential(true);
    editor.rateSlider.setSmoothingTime(100.0f);
    editor.depthSlider.setUseExponential(true);
    editor.depthSlider.setSmoothingTime(150.0f);
    editor.offsetSlider.setUseExponential(true);
    editor.offsetSlider.setSmoothingTime(100.0f);
    editor.widthSlider.setUseExponential(true);
    editor.widthSlider.setSmoothingTime(100.0f);
    editor.colorSlider.setSmoothingTime(90.0f);
    editor.mixSlider.setSmoothingTime(100.0f);
}

void PluginEditorSetup::setupValueLabels(ChoroborosPluginEditor& editor)
{
    editor.rateValueLabel.setJustificationType(juce::Justification::centred);
    editor.depthValueLabel.setJustificationType(juce::Justification::centred);
    editor.offsetValueLabel.setJustificationType(juce::Justification::centred);
    editor.widthValueLabel.setJustificationType(juce::Justification::centred);
    editor.colorValueLabel.setJustificationType(juce::Justification::centred);
    editor.mixValueLabel.setJustificationType(juce::Justification::centred);
    
    editor.rateValueLabel.setValueLabelStyle(true);
    editor.depthValueLabel.setValueLabelStyle(true);
    editor.offsetValueLabel.setValueLabelStyle(true);
    editor.widthValueLabel.setValueLabelStyle(true);
    editor.colorValueLabel.setValueLabelStyle(true);
    editor.mixValueLabel.setValueLabelStyle(true);
    
    juce::Colour valueTextColor(0xff9dbd78);
    editor.rateValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.depthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.offsetValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.widthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.colorValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    editor.mixValueLabel.setColour(juce::Label::textColourId, valueTextColor);
}

void PluginEditorSetup::setupLabels(ChoroborosPluginEditor& editor)
{
    juce::Font labelFont;
    labelFont.setHeight(14.0f);
    labelFont.setBold(true);
    
    juce::Font valueFont;
    valueFont.setHeight(14.0f);
    valueFont.setBold(true);
    
    int rateLabelWidth = editor.calculateLabelWidth("RATE", labelFont);
    int depthLabelWidth = editor.calculateLabelWidth("DEPTH", labelFont);
    int offsetLabelWidth = editor.calculateLabelWidth("OFFSET", labelFont);
    int widthLabelWidth = editor.calculateLabelWidth("WIDTH", labelFont);
    int colorLabelWidth = editor.calculateLabelWidth("COLOR", labelFont);
    int mixLabelWidth = editor.calculateLabelWidth("MIX", labelFont);
    
    int rateLabelX = (45 + 7 + 67) - (rateLabelWidth / 2);
    int depthLabelX = (255 + 7 + 67) - (depthLabelWidth / 2);
    int offsetLabelX = (45 + 7 + 67) - (offsetLabelWidth / 2);
    int widthLabelX = (255 + 7 + 67) - (widthLabelWidth / 2);
    int colorLabelX = (75 + 150) - (colorLabelWidth / 2);
    
    editor.rateLabel.setBounds(rateLabelX, 98, rateLabelWidth, 20);
    editor.depthLabel.setBounds(depthLabelX, 98, depthLabelWidth, 20);
    editor.offsetLabel.setBounds(offsetLabelX, 293 + 16, offsetLabelWidth, 20);
    editor.widthLabel.setBounds(widthLabelX, 293 + 16, widthLabelWidth, 20);
    editor.colorLabel.setBounds(colorLabelX, 490, colorLabelWidth, 20); // Moved up 24px (was 514)
    
    // Mix label positioned above mix knob
    // Use same variables defined in setupSliders
    const int mixKnobSizeForLabel = 40; // 20% smaller than 50px
    const int mixKnobXForLabel = 450 - mixKnobSizeForLabel - 20 - 32 + 8 + 3; // Move 32px left, then 8px right, then 3px right
    const int mixKnobYForLabel = 700 - mixKnobSizeForLabel - 20 - 24 - 24; // Move up 24px more (was -24, now -48 total)
    const int mixKnobCenterXForLabel = mixKnobXForLabel + (mixKnobSizeForLabel / 2);
    const int mixLabelX = mixKnobCenterXForLabel - (mixLabelWidth / 2) + 2; // Move right 2px
    editor.mixLabel.setBounds(mixLabelX, mixKnobYForLabel - 22 + 40 - 24 - 16 - 6 - 24, mixLabelWidth, 20); // Move up 24px more
    
    int rateValueWidth = editor.calculateLabelWidth("10.0 Hz", valueFont);
    int depthValueWidth = editor.calculateLabelWidth("100%", valueFont);
    int offsetValueWidth = editor.calculateLabelWidth("180°", valueFont);
    int widthValueWidth = editor.calculateLabelWidth("200%", valueFont);
    int colorValueWidth = editor.calculateLabelWidth("100%", valueFont);
    // Mix value label uses 25% smaller font (14.0f * 0.75 = 10.5f)
    juce::Font mixValueFont;
    mixValueFont.setHeight(10.5f);
    mixValueFont.setBold(true);
    int mixValueWidth = editor.calculateLabelWidth("100%", mixValueFont);
    
    int rateValueX = (45 + 7 + 67) - (rateValueWidth / 2);
    int depthValueX = (255 + 7 + 67) - (depthValueWidth / 2);
    int offsetValueX = (45 + 7 + 67) - (offsetValueWidth / 2);
    int widthValueX = (255 + 7 + 67) - (widthValueWidth / 2);
    int colorValueX = (75 + 150) - (colorValueWidth / 2);
    
    editor.rateValueLabel.setBounds(rateValueX, 120 + 7 + 135 + 5, rateValueWidth, 20);
    editor.depthValueLabel.setBounds(depthValueX, 120 + 7 + 135 + 5, depthValueWidth, 20);
    editor.offsetValueLabel.setBounds(offsetValueX, 310 + 7 + 135 + 5 + 16, offsetValueWidth, 20);
    editor.widthValueLabel.setBounds(widthValueX, 310 + 7 + 135 + 5 + 16, widthValueWidth, 20);
    editor.colorValueLabel.setBounds(colorValueX, 520 + 70 + 5, colorValueWidth, 20); // Moved up 24px (was 544 + 70 + 5)
    
    // Mix knob label and value in bottom right
    // Use same variables defined in setupSliders
    const int mixKnobSizeForValue = 40; // 20% smaller than 50px
    const int mixKnobXForValue = 450 - mixKnobSizeForValue - 20 - 32 + 8 + 3; // Move 32px left, then 8px right, then 3px right
    const int mixKnobYForValue = 700 - mixKnobSizeForValue - 20 - 24 - 24; // Move up 24px more (was -24, now -48 total)
    const int mixKnobCenterXForValue = mixKnobXForValue + (mixKnobSizeForValue / 2);
    const int mixValueX = mixKnobCenterXForValue - (mixValueWidth / 2);
    // Position value label below knob, but ensure it's within window bounds (700px height)
    // mixKnobYForValue + mixKnobSizeForValue + 5 = bottom of knob + spacing
    // Clamp to ensure it fits: max Y position should be 700 - label height (20) = 680
    const int mixValueY = juce::jmin(mixKnobYForValue + mixKnobSizeForValue + 5, 680);
    editor.mixValueLabel.setBounds(mixValueX, mixValueY, mixValueWidth, 20);
    
    // Apply the smaller font to mix value label
    editor.mixValueLabel.setFont(mixValueFont);
}

void PluginEditorSetup::setupHQButton(ChoroborosPluginEditor& editor)
{
    editor.addAndMakeVisible(editor.hqButton);
    editor.addAndMakeVisible(editor.hqLabel);
    
    editor.hqLabel.setText("HQ", juce::dontSendNotification);
    editor.hqLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    editor.hqLabel.setJustificationType(juce::Justification::centred);
    juce::Font hqFont;
    hqFont.setHeight(14.0f);
    hqFont.setBold(true);
    editor.hqLabel.setFont(hqFont);
    
    const int buttonWidth = 60;
    const int buttonHeight = 24;
    const int centerX = 225;
    const int centerY = 306;
    editor.hqButton.setBounds(centerX - buttonWidth / 2, centerY - buttonHeight / 2, buttonWidth, buttonHeight);
    
    // Set HQ button tooltip
    editor.hqButton.setTooltip("High Quality Mode: Enables higher-quality algorithm variant for the selected engine. Increases CPU usage but improves audio fidelity.");
    
    int hqLabelWidth = editor.calculateLabelWidth("HQ", hqFont);
    editor.hqLabel.setBounds(centerX - hqLabelWidth / 2, centerY - buttonHeight / 2 - 22, hqLabelWidth, 20);
}
