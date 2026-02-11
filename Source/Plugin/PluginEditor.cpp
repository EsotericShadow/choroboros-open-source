#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../UI/LabelWithContainer.h"
#include "BinaryData.h"
#include "../UI/PluginEditorSetup.h"

//==============================================================================
ChoroborosPluginEditor::ChoroborosPluginEditor (ChoroborosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    setupEngineColorSelector();
    // Note: setupEngineColorSelector now reads the saved parameter value and updates UI
    
    // Setup sliders with exact bounds
    setupSlider(rateSlider, rateLabel, rateValueLabel, "RATE", ChoroborosAudioProcessor::RATE_ID);
    setupSlider(depthSlider, depthLabel, depthValueLabel, "DEPTH", ChoroborosAudioProcessor::DEPTH_ID);
    setupSlider(offsetSlider, offsetLabel, offsetValueLabel, "OFFSET", ChoroborosAudioProcessor::OFFSET_ID);
    setupSlider(widthSlider, widthLabel, widthValueLabel, "WIDTH", ChoroborosAudioProcessor::WIDTH_ID);
    setupSlider(colorSlider, colorLabel, colorValueLabel, "COLOR", ChoroborosAudioProcessor::COLOR_ID);
    setupSlider(mixSlider, mixLabel, mixValueLabel, "MIX", ChoroborosAudioProcessor::MIX_ID);
    
    PluginEditorSetup::setupSliders(*this);
    setupSliderAttachments();
    
    PluginEditorSetup::setupHQButton(*this);
    hqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::HQ_ID, hqButton);
    
    PluginEditorSetup::setupValueLabels(*this);
    PluginEditorSetup::setupLabels(*this);
    setupSliderValueChangeListeners();
    
    // Update value label colors based on saved engine color (after all labels are set up)
    auto engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
    if (engineColorParam)
    {
        const int savedColorIndex = static_cast<int>(engineColorParam->load());
        updateValueLabelColors(savedColorIndex);
    }
    else
    {
        updateValueLabelColors(0);  // Default to Green if no parameter
    }
    
    // Set up double-click editing for value labels
    setupValueLabelEditing(rateValueLabel, rateSlider, ChoroborosAudioProcessor::RATE_ID);
    setupValueLabelEditing(depthValueLabel, depthSlider, ChoroborosAudioProcessor::DEPTH_ID);
    setupValueLabelEditing(offsetValueLabel, offsetSlider, ChoroborosAudioProcessor::OFFSET_ID);
    setupValueLabelEditing(widthValueLabel, widthSlider, ChoroborosAudioProcessor::WIDTH_ID);
    setupValueLabelEditing(colorValueLabel, colorSlider, ChoroborosAudioProcessor::COLOR_ID);
    setupValueLabelEditing(mixValueLabel, mixSlider, ChoroborosAudioProcessor::MIX_ID);
    
    // Initial value updates
    updateValueLabel(rateValueLabel, rateSlider.getValue(), ChoroborosAudioProcessor::RATE_ID);
    updateValueLabel(depthValueLabel, depthSlider.getValue(), ChoroborosAudioProcessor::DEPTH_ID);
    updateValueLabel(offsetValueLabel, offsetSlider.getValue(), ChoroborosAudioProcessor::OFFSET_ID);
    updateValueLabel(widthValueLabel, widthSlider.getValue(), ChoroborosAudioProcessor::WIDTH_ID);
    updateValueLabel(colorValueLabel, colorSlider.getValue(), ChoroborosAudioProcessor::COLOR_ID);
    updateValueLabel(mixValueLabel, mixSlider.getValue(), ChoroborosAudioProcessor::MIX_ID);
    
    // Setup tooltip window
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
    
    // Setup feedback button (alpha version)
    feedbackButton.setButtonText("Feedback");
    feedbackButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    feedbackButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    feedbackButton.onClick = [this] {
        if (audioProcessor.feedbackCollector)
        {
            FeedbackDialog::show(*audioProcessor.feedbackCollector);
        }
    };
    feedbackButton.setTooltip("Send Feedback: Share your thoughts, bug reports, or feature requests. Usage statistics included automatically.");
    addAndMakeVisible(feedbackButton);
    feedbackButton.setBounds(350, 5, 90, 20); // Top right, small button
    
    // Setup Help button
    helpButton.setButtonText("Help");
    helpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    helpButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    helpButton.onClick = [] {
        // Open help/documentation (for now, just open email - can be updated to PDF link later)
        juce::URL("mailto:info@kaizenstrategic.ai?subject=Choroboros%20Help").launchInDefaultBrowser();
    };
    helpButton.setTooltip("Help: Get documentation and information");
    addAndMakeVisible(helpButton);
    helpButton.setBounds(260, 5, 80, 20);
    
    // Setup About button
    aboutButton.setButtonText("About");
    aboutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    aboutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    aboutButton.onClick = [] {
        AboutDialog::show();
    };
    aboutButton.setTooltip("About: View version information and company details");
    addAndMakeVisible(aboutButton);
    aboutButton.setBounds(170, 5, 80, 20);
    
    // Set fixed size
    setSize(450, 700);
    setResizable(false, false);
}

ChoroborosPluginEditor::~ChoroborosPluginEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void ChoroborosPluginEditor::paint (juce::Graphics& g)
{
    // Draw background
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, 0, 0, 450, 700, 0, 0,
                   backgroundImage.getWidth(), backgroundImage.getHeight());
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
}

void ChoroborosPluginEditor::resized()
{
    // Fixed size, no resizing needed
}

void ChoroborosPluginEditor::setupEngineColorSelector()
{
    addAndMakeVisible(engineColorBox);
    
    engineColorBox.addItem("Green", 1);
    engineColorBox.addItem("Blue", 2);
    engineColorBox.addItem("Red", 3);
    engineColorBox.addItem("Purple", 4);
    engineColorBox.setSelectedId(1);
    
    const int engineColorWidth = 100;
    const int engineColorHeight = 24;
    // Position at top left corner
    engineColorBox.setBounds(0, 0, engineColorWidth, engineColorHeight);
    
    // Remove background color - make it transparent
    engineColorBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    engineColorBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    engineColorBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    
    engineColorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::ENGINE_COLOR_ID, engineColorBox);
    
    // Read the current parameter value and update UI to match (for persistence)
    // Do this AFTER attachment is created so ComboBox has the correct value
    auto engineColorParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
    if (engineColorParam)
    {
        const int savedColorIndex = static_cast<int>(engineColorParam->load());
        // Use the ComboBox's selected ID to ensure consistency
        const int actualColorIndex = engineColorBox.getSelectedId() - 1;
        customLookAndFeel.setColorTheme(actualColorIndex);
        loadBackgroundImage(actualColorIndex);
        // Value labels will be updated after they're set up in constructor
    }
    
    engineColorBox.setTooltip("Engine Selection: Choose between four distinct chorus algorithms. Green=Classic, Blue=Modern, Red=Vintage, Purple=Experimental.");
    
    engineColorBox.onChange = [this] {
        const int colorIndex = engineColorBox.getSelectedId() - 1;
        customLookAndFeel.setColorTheme(colorIndex);
        loadBackgroundImage(colorIndex);
        updateValueLabelColors(colorIndex);
        
        // Track engine switch for feedback
        if (audioProcessor.feedbackCollector)
        {
            auto hqParam = audioProcessor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::HQ_ID);
            bool hq = hqParam ? (hqParam->load() > 0.5f) : false;
            audioProcessor.feedbackCollector->trackEngineSwitch(colorIndex, hq);
        }
        
        repaint();
    };
}

void ChoroborosPluginEditor::setupSliderAttachments()
{
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::DEPTH_ID, depthSlider);
    offsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::OFFSET_ID, offsetSlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::WIDTH_ID, widthSlider);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::COLOR_ID, colorSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ChoroborosAudioProcessor::MIX_ID, mixSlider);
}

void ChoroborosPluginEditor::setupSliderValueChangeListeners()
{
    rateSlider.onValueChange = [this] { updateValueLabel(rateValueLabel, rateSlider.getValue(), ChoroborosAudioProcessor::RATE_ID); };
    depthSlider.onValueChange = [this] { updateValueLabel(depthValueLabel, depthSlider.getValue(), ChoroborosAudioProcessor::DEPTH_ID); };
    offsetSlider.onValueChange = [this] { updateValueLabel(offsetValueLabel, offsetSlider.getValue(), ChoroborosAudioProcessor::OFFSET_ID); };
    widthSlider.onValueChange = [this] { updateValueLabel(widthValueLabel, widthSlider.getValue(), ChoroborosAudioProcessor::WIDTH_ID); };
    colorSlider.onValueChange = [this] { updateValueLabel(colorValueLabel, colorSlider.getValue(), ChoroborosAudioProcessor::COLOR_ID); };
    mixSlider.onValueChange = [this] { updateValueLabel(mixValueLabel, mixSlider.getValue(), ChoroborosAudioProcessor::MIX_ID); };
}

void ChoroborosPluginEditor::updateValueLabelColors(int colorIndex)
{
    // Color values for each engine:
    // Green (0): #9dbd78
    // Blue (1): #ac91dd
    // Red (2): #ff8d8b
    // Purple (3): #b88dd8
    juce::Colour valueTextColor;
    if (colorIndex == 0) // Green
        valueTextColor = juce::Colour(0xff9dbd78);
    else if (colorIndex == 1) // Blue
        valueTextColor = juce::Colour(0xffac91dd);
    else if (colorIndex == 2) // Red
        valueTextColor = juce::Colour(0xffff8d8b);
    else // Purple (colorIndex == 3)
        valueTextColor = juce::Colour(0xffb88dd8);
    
    // Update all value label text colors
    rateValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    depthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    offsetValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    widthValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    colorValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    mixValueLabel.setColour(juce::Label::textColourId, valueTextColor);
    
    // Also update editor text colors (for when editing)
    rateValueLabel.setEditorTextColor(valueTextColor);
    depthValueLabel.setEditorTextColor(valueTextColor);
    offsetValueLabel.setEditorTextColor(valueTextColor);
    widthValueLabel.setEditorTextColor(valueTextColor);
    colorValueLabel.setEditorTextColor(valueTextColor);
    mixValueLabel.setEditorTextColor(valueTextColor);
    
    // Repaint all value labels to show new color
    rateValueLabel.repaint();
    depthValueLabel.repaint();
    offsetValueLabel.repaint();
    widthValueLabel.repaint();
    colorValueLabel.repaint();
    mixValueLabel.repaint();
}

void ChoroborosPluginEditor::loadBackgroundImage(int colorIndex)
{
    colorIndex = juce::jlimit(0, 3, colorIndex);
    
    const char* bgName = nullptr;
    int bgSize = 0;
    
    if (colorIndex == 0) // Green
    {
        bgName = BinaryData::green_panel_background_png;
        bgSize = BinaryData::green_panel_background_pngSize;
    }
    else if (colorIndex == 1) // Blue
    {
        bgName = BinaryData::blue_panel_background_png;
        bgSize = BinaryData::blue_panel_background_pngSize;
    }
    else if (colorIndex == 2) // Red
    {
        bgName = BinaryData::red_panel_background_png;
        bgSize = BinaryData::red_panel_background_pngSize;
    }
    else // Purple (colorIndex == 3)
    {
        bgName = BinaryData::purple_panel_background_png;
        bgSize = BinaryData::purple_panel_background_pngSize;
    }
    
    if (bgName && bgSize > 0)
        backgroundImage = juce::ImageCache::getFromMemory(bgName, bgSize);
}

int ChoroborosPluginEditor::calculateLabelWidth(const juce::String& text, const juce::Font& font) const
{
    // Calculate text width and add 16px total (8px padding on each side)
    // Use GlyphArrangement for accurate text width (recommended approach)
    float textWidth = juce::GlyphArrangement::getStringWidth(font, text);
    return static_cast<int>(std::ceil(textWidth)) + 16;
}

void ChoroborosPluginEditor::setupSlider(juce::Slider& slider, LabelWithContainer& label, LabelWithContainer& valueLabel,
                                         const juce::String& name, const juce::String& paramId)
{
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    addAndMakeVisible(valueLabel);
    
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    juce::Font font;
    font.setHeight(14.0f);
    font.setBold(true);
    label.setFont(font);
    
    // Set tooltips based on parameter
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
        slider.setTooltip("LFO Speed: Controls the modulation rate from 0.01 Hz (slow, lush) to 20 Hz (fast, vibrato). Lower values create classic chorus, higher values add movement.");
    else if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
        slider.setTooltip("Modulation Depth: Controls how much the delay time is modulated. 0% = no effect, 100% = maximum modulation. Engine-specific scaling applied.");
    else if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
        slider.setTooltip("LFO Phase Offset: Shifts the modulation phase from 0° to 180°. Useful for stereo width and avoiding phase cancellation.");
    else if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
        slider.setTooltip("Stereo Width: Controls the stereo spread from 0% (mono) to 200% (wide). Adjusts the phase relationship between left and right channels.");
    else if (paramId == ChoroborosAudioProcessor::COLOR_ID)
        slider.setTooltip("Tone/Character: Engine-specific parameter. Green=feedback, Blue=filter, Red=saturation, Purple=warp amount.");
    else if (paramId == ChoroborosAudioProcessor::MIX_ID)
        slider.setTooltip("Dry/Wet Mix: Blends the original signal (0%) with the processed signal (100%). 50% = equal blend.");
    
    // Position labels above knobs/sliders (will be set per control in constructor)
}

void ChoroborosPluginEditor::updateValueLabel(LabelWithContainer& label, float value, const juce::String& paramId)
{
    juce::String text;
    
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
    {
        // Rate: < 1.0 Hz: 2 decimals, >= 1.0 Hz: 1 decimal
        if (value < 1.0f)
            text = juce::String(value, 2) + " Hz";
        else
            text = juce::String(value, 1) + " Hz";
    }
    else if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
    {
        text = juce::String(static_cast<int>(value * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
    {
        text = juce::String(static_cast<int>(value)) + "°";
    }
    else if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
    {
        text = juce::String(static_cast<int>(value * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::COLOR_ID)
    {
        text = juce::String(static_cast<int>(value * 100.0f)) + "%";
    }
    else if (paramId == ChoroborosAudioProcessor::MIX_ID)
    {
        text = juce::String(static_cast<int>(value * 100.0f)) + "%";
    }
    
    label.setText(text, juce::dontSendNotification);
}

void ChoroborosPluginEditor::setupValueLabelEditing(LabelWithContainer& label, juce::Slider& slider, const juce::String& paramId)
{
    label.onValueEdited = [this, &slider, &label, paramId](const juce::String& newText) -> bool
    {
        float newValue = parseValueFromText(newText, paramId);
        if (newValue >= 0.0f)  // Valid value
        {
            // Clamp to slider range
            double minVal = slider.getMinimum();
            double maxVal = slider.getMaximum();
            float clampedValue = static_cast<float>(juce::jlimit(minVal, maxVal, static_cast<double>(newValue)));
            
            // Update the parameter directly via the value tree state to ensure it applies
            // This bypasses any potential attachment blocking
            auto* param = audioProcessor.getValueTreeState().getParameter(paramId);
            if (param != nullptr)
            {
                // Convert to normalized 0-1 range
                float normalizedValue = param->convertTo0to1(clampedValue);
                // Clamp normalized value to valid range
                normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
                param->setValueNotifyingHost(normalizedValue);
            }
            else
            {
                // Parameter not found - fallback to slider update
                slider.setValue(clampedValue, juce::sendNotificationSync);
            }
            
            // Also set slider value to update visual position
            slider.setValue(clampedValue, juce::dontSendNotification);
            
            // Format and set the label text - this will be picked up by editorAboutToBeHidden
            updateValueLabel(label, clampedValue, paramId);
            
            return true;  // Value was applied successfully
        }
        else
        {
            // Invalid value - restore previous value
            updateValueLabel(label, slider.getValue(), paramId);
            return false;  // Value was not applied
        }
    };
}

float ChoroborosPluginEditor::parseValueFromText(const juce::String& text, const juce::String& paramId)
{
    const juce::String trimmed = text.trim();
    
    if (paramId == ChoroborosAudioProcessor::RATE_ID)
        return parseRateValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::DEPTH_ID)
        return parseDepthValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::OFFSET_ID)
        return parseOffsetValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::WIDTH_ID)
        return parseWidthValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::COLOR_ID)
        return parseColorValue(trimmed);
    if (paramId == ChoroborosAudioProcessor::MIX_ID)
        return parseMixValue(trimmed);
    
    return -1.0f; // Invalid
}

float ChoroborosPluginEditor::parseRateValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("Hz").trim();
    const float value = clean.getFloatValue();
    if (value > 0.0f && value <= 10.0f)
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseDepthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseOffsetValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("°").trim();
    if (clean.endsWithIgnoreCase("deg"))
        clean = clean.substring(0, clean.length() - 3).trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 180.0f)
        return value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseWidthValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 200.0f)
        return (value > 2.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseColorValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}

float ChoroborosPluginEditor::parseMixValue(const juce::String& trimmed)
{
    juce::String clean = trimmed.removeCharacters("%").trim();
    const float value = clean.getFloatValue();
    if (value >= 0.0f && value <= 100.0f)
        return (value > 1.0f) ? (value / 100.0f) : value;
    return -1.0f;
}
