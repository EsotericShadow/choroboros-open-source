#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LabelWithContainer : public juce::Label
{
public:
    LabelWithContainer() = default;
    ~LabelWithContainer() override = default;
    
    void paint(juce::Graphics& g) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    // Set whether this is a value label (light grey background) or name label (dark background)
    void setValueLabelStyle(bool isValueLabel);
    
    // Set callback for when value is edited (called when text editor loses focus or Enter is pressed)
    // Returns true if value was applied successfully, false if invalid
    std::function<bool(const juce::String&)> onValueEdited;
    
    // Set the text editor color (for when editing value labels)
    void setEditorTextColor(juce::Colour color);
    
private:
    bool isValueLabelStyle = false;
    bool isEditing = false;
    juce::String pendingFormattedText;  // Store formatted text to apply after editor hides
    juce::Colour editorTextColor = juce::Colour(0xff9dbd78);  // Default green, will be updated
    void editorShown(juce::TextEditor* editor) override;
    void editorAboutToBeHidden(juce::TextEditor* editor) override;
    juce::TextEditor* createEditorComponent() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelWithContainer)
};
