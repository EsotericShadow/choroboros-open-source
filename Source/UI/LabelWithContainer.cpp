#include "LabelWithContainer.h"

void LabelWithContainer::setValueLabelStyle(bool isValueLabel)
{
    isValueLabelStyle = isValueLabel;
    if (isValueLabel)
    {
        setEditable(true, false, false);  // Editable on double-click, not single-click
    }
}

void LabelWithContainer::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (isValueLabelStyle && onValueEdited)
    {
        showEditor();
    }
    else
    {
        juce::Label::mouseDoubleClick(e);
    }
}

void LabelWithContainer::setEditorTextColor(juce::Colour color)
{
    editorTextColor = color;
}

juce::TextEditor* LabelWithContainer::createEditorComponent()
{
    if (!isValueLabelStyle)
        return juce::Label::createEditorComponent();
    
    auto* editor = new juce::TextEditor();
    editor->setJustification(juce::Justification::centred);
    editor->setColour(juce::TextEditor::textColourId, editorTextColor);
    editor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff000000));
    editor->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff404040));
    editor->setColour(juce::TextEditor::focusedOutlineColourId, editorTextColor);
    editor->setBorder(juce::BorderSize<int>(0));
    editor->setFont(getFont());
    editor->setText(getText(), false);
    editor->setSelectAllWhenFocused(true);
    editor->setReturnKeyStartsNewLine(false);
    
    return editor;
}

void LabelWithContainer::editorShown(juce::TextEditor* editor)
{
    if (editor && isValueLabelStyle)
    {
        isEditing = true;
        repaint();  // Repaint to hide underlying text
        
        // Select all text for easy replacement
        editor->selectAll();
        
        // Store original text in case we need to restore it
        juce::String originalText = getText();
        
        // Set up callback for when editing finishes
        editor->onReturnKey = [this, editor, originalText]()
        {
            bool valueApplied = false;
            pendingFormattedText = originalText;  // Default to original
            
            if (onValueEdited)
            {
                valueApplied = onValueEdited(editor->getText());
                // If value was applied, get the formatted text that was set
                if (valueApplied)
                {
                    pendingFormattedText = getText();  // Get the formatted text that was set
                }
            }
            
            isEditing = false;
            hideEditor(true);  // Discard editor contents - we'll set text in editorAboutToBeHidden
        };
        
        editor->onEscapeKey = [this, originalText]()
        {
            pendingFormattedText = originalText;  // Restore original
            isEditing = false;
            hideEditor(true);  // Discard editor contents
        };
        
        editor->onFocusLost = [this, editor, originalText]()
        {
            bool valueApplied = false;
            pendingFormattedText = originalText;  // Default to original
            
            if (onValueEdited)
            {
                valueApplied = onValueEdited(editor->getText());
                // If value was applied, get the formatted text that was set
                if (valueApplied)
                {
                    pendingFormattedText = getText();  // Get the formatted text that was set
                }
            }
            
            isEditing = false;
            hideEditor(true);  // Discard editor contents - we'll set text in editorAboutToBeHidden
        };
    }
    else
    {
        juce::Label::editorShown(editor);
    }
}

void LabelWithContainer::editorAboutToBeHidden(juce::TextEditor* editor)
{
    if (editor && isValueLabelStyle)
    {
        // Set the formatted text AFTER editor is about to be hidden but BEFORE it's actually hidden
        // This ensures the text is set correctly and won't be overwritten
        if (!pendingFormattedText.isEmpty())
        {
            setText(pendingFormattedText, juce::dontSendNotification);
            pendingFormattedText.clear();
        }
    }
    
    // Call base class to maintain JUCE's normal behavior
    juce::Label::editorAboutToBeHidden(editor);
}

void LabelWithContainer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float cornerRadius = 4.0f;
    const float borderWidth = 1.0f;
    
    if (isValueLabelStyle)
    {
        // Value label style: black background with subtle border
        // Create gradient for border (subtle dark gradient)
        juce::ColourGradient gradient(
            juce::Colour(0xff404040), bounds.getX(), bounds.getY(),  // Top-left: lighter dark grey
            juce::Colour(0xff202020), bounds.getX(), bounds.getBottom(), // Bottom: darker grey
            false
        );
        
        // Draw border with gradient
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // Draw inner background (black)
        const auto innerBounds = bounds.reduced(borderWidth);
        g.setColour(juce::Colour(0xff000000)); // Black background
        g.fillRoundedRectangle(innerBounds, cornerRadius - borderWidth);
        
        // Only draw text if not editing (editor will show its own text)
        if (!isEditing)
        {
            g.setColour(findColour(juce::Label::textColourId));
            g.setFont(getFont());
            g.drawText(getText(), innerBounds, getJustificationType(), false);
        }
    }
    else
    {
        // Name label style: dark background with green gradient border (original style)
        // Create gradient for border (subtle gradient from lighter to darker green)
        juce::ColourGradient gradient(
            juce::Colour(0xff4a6b5a), bounds.getX(), bounds.getY(),  // Top-left: lighter green
            juce::Colour(0xff2a3b32), bounds.getX(), bounds.getBottom(), // Bottom: darker green
            false
        );
        
        // Draw border with gradient
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // Draw inner background (slightly inset for border effect)
        const auto innerBounds = bounds.reduced(borderWidth);
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f)); // Dark semi-transparent background
        g.fillRoundedRectangle(innerBounds, cornerRadius - borderWidth);
        
        // Draw text
        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(getFont());
        g.drawText(getText(), innerBounds, getJustificationType(), false);
    }
}
