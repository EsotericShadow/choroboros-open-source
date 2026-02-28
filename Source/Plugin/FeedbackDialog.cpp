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

#include "FeedbackDialog.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace
{
juce::Font makeRetroFont(float height, bool bold)
{
    juce::Font font { juce::FontOptions { height, bold ? juce::Font::bold : juce::Font::plain } };
    if (BinaryData::Retroica_ttfSize > 0)
    {
        static juce::Typeface::Ptr retroTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Retroica_ttf,
            static_cast<size_t>(BinaryData::Retroica_ttfSize));
        if (retroTypeface != nullptr)
            font = juce::Font { juce::FontOptions { retroTypeface }.withHeight(height) };
    }
    if (bold)
        font.setBold(true);
    return font;
}
}

FeedbackDialog::FeedbackDialog(FeedbackCollector& collector)
    : feedbackCollector(collector)
{
    setSize(500, 440);

    const auto accent = juce::Colour(0xff9dbd78);
    const auto bodyText = juce::Colour(0xffe8ecf1);
    const auto mutedText = juce::Colour(0xffa0acba);
    
    titleLabel.setText("Choroboros Beta - Feedback", juce::dontSendNotification);
    titleLabel.setFont(makeRetroFont(20.0f, true));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);
    
    infoLabel.setText("Help us improve! Use the feedback form to share your thoughts, bug reports, or feature requests.\n"
                      "You can save usage statistics to a file and paste them into the form.\n\n"
                      "Not yet a beta tester? Sign up:",
                      juce::dontSendNotification);
    infoLabel.setFont(makeRetroFont(14.0f, false));
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, mutedText);
    addAndMakeVisible(infoLabel);
    
    feedbackText.setMultiLine(true, true);
    feedbackText.setReturnKeyStartsNewLine(true);
    feedbackText.setFont(makeRetroFont(14.0f, false));
    feedbackText.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff11161b));
    feedbackText.setColour(juce::TextEditor::textColourId, bodyText);
    feedbackText.setColour(juce::TextEditor::outlineColourId, accent.withAlpha(0.65f));
    feedbackText.setColour(juce::TextEditor::focusedOutlineColourId, accent);
    feedbackText.setColour(juce::CaretComponent::caretColourId, accent.brighter(0.25f));
    // Note: setPlaceholderText may not be available in all JUCE versions
    // feedbackText.setPlaceholderText("Enter your feedback here...");
    addAndMakeVisible(feedbackText);
    
    saveButton.setButtonText("Save to File");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f2f23));
    saveButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2a4130));
    saveButton.setColour(juce::TextButton::textColourOffId, accent.brighter(0.18f));
    saveButton.setColour(juce::TextButton::textColourOnId, accent.brighter(0.18f));
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    formButton.setButtonText("Open Feedback Form");
    formButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff243138));
    formButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff31424c));
    formButton.setColour(juce::TextButton::textColourOffId, bodyText);
    formButton.setColour(juce::TextButton::textColourOnId, bodyText);
    formButton.addListener(this);
    addAndMakeVisible(formButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a3138));
    cancelButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a444d));
    cancelButton.setColour(juce::TextButton::textColourOffId, bodyText);
    cancelButton.setColour(juce::TextButton::textColourOnId, bodyText);
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);

    betaSignUpLink.setButtonText("Choroboros v2.02 Beta Test Sign-up");
    betaSignUpLink.setURL(juce::URL("https://docs.google.com/forms/d/e/1FAIpQLSc5OQpZlMpVSOfcRr6k2nqo5D25M_COfb0qyhCxdj2WmxpGpw/viewform"));
    betaSignUpLink.setFont(makeRetroFont(12.0f, false), false);
    betaSignUpLink.setColour(juce::HyperlinkButton::textColourId, accent.brighter(0.18f));
    addAndMakeVisible(betaSignUpLink);
}

void FeedbackDialog::paint(juce::Graphics& g)
{
    const auto accent = juce::Colour(0xff9dbd78);
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour(0xff090b0d));
    juce::ColourGradient bg(juce::Colour(0xff151a1f), 0.0f, 0.0f,
                            juce::Colour(0xff11161b), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(8.0f), 10.0f);

    g.setColour(accent.withAlpha(0.78f));
    g.drawRoundedRectangle(bounds.reduced(8.5f), 10.0f, 1.2f);
    g.setColour(accent.withAlpha(0.18f));
    g.drawRoundedRectangle(bounds.reduced(12.0f), 8.0f, 1.0f);
}

void FeedbackDialog::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    
    infoLabel.setBounds(area.removeFromTop(70));
    area.removeFromTop(5);
    betaSignUpLink.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);

    feedbackText.setBounds(area.removeFromTop(200));
    area.removeFromTop(20);
    
    auto buttonArea = area.removeFromTop(30);
    saveButton.setBounds(buttonArea.removeFromLeft(140));
    buttonArea.removeFromLeft(10);
    formButton.setBounds(buttonArea.removeFromLeft(160));
    buttonArea.removeFromLeft(10);
    cancelButton.setBounds(buttonArea.removeFromLeft(120));
}

void FeedbackDialog::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        saveFeedback();
    }
    else if (button == &formButton)
    {
        openFeedbackForm();
    }
    else if (button == &cancelButton)
    {
        // Close dialog
        if (auto* parent = getParentComponent())
        {
            parent->removeChildComponent(this);
        }
        delete this;
    }
}

void FeedbackDialog::saveFeedback()
{
    if (feedbackCollector.saveFeedbackToFile(feedbackText.getText()))
    {
        // Close dialog - feedback is saved
        if (auto* parent = getParentComponent())
        {
            parent->removeChildComponent(this);
        }
        delete this;
    }
    // If save failed, dialog stays open so user can try again
}

void FeedbackDialog::openFeedbackForm()
{
    juce::URL("https://docs.google.com/forms/d/e/1FAIpQLSc5OQpZlMpVSOfcRr6k2nqo5D25M_COfb0qyhCxdj2WmxpGpw/viewform").launchInDefaultBrowser();
    
    if (auto* parent = getParentComponent())
    {
        parent->removeChildComponent(this);
    }
    delete this;
}

void FeedbackDialog::show(FeedbackCollector& collector)
{
    auto* dialog = new FeedbackDialog(collector);
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(dialog);
    options.content->setSize(500, 440);
    options.dialogTitle = "Feedback";
    options.dialogBackgroundColour = juce::Colour(0xff2a2a2a);
    options.resizable = false;
    options.useNativeTitleBar = true;
    
    auto* window = options.launchAsync();
    (void)window; // Suppress unused variable warning
}
