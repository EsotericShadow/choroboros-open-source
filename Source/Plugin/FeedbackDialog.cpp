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
    
    titleLabel.setText("Choroboros Beta - Feedback", juce::dontSendNotification);
    titleLabel.setFont(makeRetroFont(20.0f, true));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    infoLabel.setText("Help us improve! Share your thoughts, bug reports, or feature requests.\n"
                      "Usage statistics will be included automatically.\n\n"
                      "Not yet a beta tester? Sign up:",
                      juce::dontSendNotification);
    infoLabel.setFont(makeRetroFont(14.0f, false));
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(infoLabel);
    
    feedbackText.setMultiLine(true, true);
    feedbackText.setReturnKeyStartsNewLine(true);
    feedbackText.setFont(makeRetroFont(14.0f, false));
    // Note: setPlaceholderText may not be available in all JUCE versions
    // feedbackText.setPlaceholderText("Enter your feedback here...");
    addAndMakeVisible(feedbackText);
    
    saveButton.setButtonText("Save to File");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    emailButton.setButtonText("Open Email");
    emailButton.addListener(this);
    addAndMakeVisible(emailButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);

    betaSignUpLink.setButtonText("Choroboros v2.01 Beta Test Sign-up");
    betaSignUpLink.setURL(juce::URL("https://docs.google.com/forms/d/e/1FAIpQLSekP3STx1XL63JRam5p9bdKY7h667w_V4ZSVr0U_x4OAjZW9g/viewform"));
    betaSignUpLink.setFont(makeRetroFont(12.0f, false), false);
    addAndMakeVisible(betaSignUpLink);
}

void FeedbackDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
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
    saveButton.setBounds(buttonArea.removeFromLeft(120));
    buttonArea.removeFromLeft(10);
    emailButton.setBounds(buttonArea.removeFromLeft(120));
    buttonArea.removeFromLeft(10);
    cancelButton.setBounds(buttonArea.removeFromLeft(120));
}

void FeedbackDialog::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        saveFeedback();
    }
    else if (button == &emailButton)
    {
        openEmail();
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

void FeedbackDialog::openEmail()
{
    juce::String emailBody;
    emailBody << "Choroboros Beta Feedback\n\n";
    emailBody << feedbackCollector.getUsageSummary() << "\n";
    emailBody << "User Feedback:\n";
    emailBody << feedbackText.getText() << "\n";
    
    juce::String emailLink = "mailto:info@kaizenstrategic.ai?subject=Choroboros%20Beta%20Feedback&body=";
    emailLink += juce::URL::addEscapeChars(emailBody, false);
    
    juce::URL(emailLink).launchInDefaultBrowser();
    
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
