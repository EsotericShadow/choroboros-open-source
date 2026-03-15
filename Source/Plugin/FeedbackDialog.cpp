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
juce::Font makeRetroFont (float height, bool bold)
{
    juce::Font font { juce::FontOptions { height, bold ? juce::Font::bold : juce::Font::plain } };
    if (BinaryData::Retroica_ttfSize > 0)
    {
        static juce::Typeface::Ptr retroTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::Retroica_ttf,
            static_cast<size_t> (BinaryData::Retroica_ttfSize));
        if (retroTypeface != nullptr)
            font = juce::Font { juce::FontOptions { retroTypeface }.withHeight (height) };
    }
    if (bold)
        font.setBold (true);
    return font;
}
} // namespace

//==============================================================================
// Constructors
//==============================================================================

FeedbackDialog::FeedbackDialog (FeedbackCollector& collector)
    : feedbackCollector (collector)
{
    initCommon();
}

FeedbackDialog::FeedbackDialog (FeedbackCollector& collector,
                                const juce::String& crashReport)
    : feedbackCollector (collector),
      crashReportMode (true),
      crashReportText (crashReport)
{
    initCommon();

    // Pre-fill with crash context
    feedbackText.setText ("The plugin did not shut down cleanly during my last session.\n"
                          "Here is what I was doing:\n\n");
}

void FeedbackDialog::initCommon()
{
    setSize (520, 570);

    const auto accent    = juce::Colour (0xff9dbd78);
    const auto bodyText  = juce::Colour (0xffe8ecf1);
    const auto mutedText = juce::Colour (0xffa0acba);
    const auto warnText  = juce::Colour (0xffd4a043);

    // Title
    if (crashReportMode)
        titleLabel.setText ("Choroboros - Crash Report", juce::dontSendNotification);
    else
        titleLabel.setText ("Choroboros Beta - Feedback", juce::dontSendNotification);

    titleLabel.setFont (makeRetroFont (20.0f, true));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, crashReportMode ? warnText : accent);
    addAndMakeVisible (titleLabel);

    // Info label
    if (crashReportMode)
    {
        infoLabel.setText ("It looks like Choroboros didn't close properly last time.\n"
                           "Help us fix this — send the crash log to the developer.\n"
                           "Add any details about what you were doing when it happened.",
                           juce::dontSendNotification);
    }
    else
    {
        infoLabel.setText ("Help us improve! Share bug reports, feature requests, or feedback.\n"
                           "Click \"Send to Developer\" to email directly from the plugin.\n\n"
                           "Not yet a beta tester? Sign up:",
                           juce::dontSendNotification);
    }
    infoLabel.setFont (makeRetroFont (14.0f, false));
    infoLabel.setJustificationType (juce::Justification::centred);
    infoLabel.setColour (juce::Label::textColourId, mutedText);
    addAndMakeVisible (infoLabel);

    // Feedback text editor
    feedbackText.setMultiLine (true, true);
    feedbackText.setReturnKeyStartsNewLine (true);
    feedbackText.setFont (makeRetroFont (14.0f, false));
    feedbackText.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff11161b));
    feedbackText.setColour (juce::TextEditor::textColourId, bodyText);
    feedbackText.setColour (juce::TextEditor::outlineColourId, accent.withAlpha (0.65f));
    feedbackText.setColour (juce::TextEditor::focusedOutlineColourId, accent);
    feedbackText.setColour (juce::CaretComponent::caretColourId, accent.brighter (0.25f));
    addAndMakeVisible (feedbackText);

    // Send to Developer button (primary action)
    sendButton.setButtonText ("Send to Developer");
    sendButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1f3f23));
    sendButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2a5130));
    sendButton.setColour (juce::TextButton::textColourOffId, accent.brighter (0.25f));
    sendButton.setColour (juce::TextButton::textColourOnId, accent.brighter (0.25f));
    sendButton.addListener (this);
    addAndMakeVisible (sendButton);

    // Save to File button
    saveButton.setButtonText ("Save to File");
    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1f2f23));
    saveButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2a4130));
    saveButton.setColour (juce::TextButton::textColourOffId, accent.brighter (0.18f));
    saveButton.setColour (juce::TextButton::textColourOnId, accent.brighter (0.18f));
    saveButton.addListener (this);
    addAndMakeVisible (saveButton);

    // Open Form button (hidden in crash mode — not useful there)
    if (! crashReportMode)
    {
        formButton.setButtonText ("Open Form");
        formButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff243138));
        formButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff31424c));
        formButton.setColour (juce::TextButton::textColourOffId, bodyText);
        formButton.setColour (juce::TextButton::textColourOnId, bodyText);
        formButton.addListener (this);
        addAndMakeVisible (formButton);
    }

    // Cancel / Dismiss
    cancelButton.setButtonText (crashReportMode ? "Dismiss" : "Cancel");
    cancelButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a3138));
    cancelButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3a444d));
    cancelButton.setColour (juce::TextButton::textColourOffId, bodyText);
    cancelButton.setColour (juce::TextButton::textColourOnId, bodyText);
    cancelButton.addListener (this);
    addAndMakeVisible (cancelButton);

    // Beta sign-up link (only in normal mode)
    if (! crashReportMode)
    {
        betaSignUpLink.setButtonText ("Choroboros v2.04 Sign-up");
        betaSignUpLink.setURL (juce::URL ("https://docs.google.com/forms/d/e/"
                                           "1FAIpQLSc5OQpZlMpVSOfcRr6k2nqo5D25M_COfb0qyhCxdj2WmxpGpw/viewform"));
        betaSignUpLink.setFont (makeRetroFont (12.0f, false), false);
        betaSignUpLink.setColour (juce::HyperlinkButton::textColourId, accent.brighter (0.18f));
        addAndMakeVisible (betaSignUpLink);
    }
}

//==============================================================================
// Paint / Layout
//==============================================================================

void FeedbackDialog::paint (juce::Graphics& g)
{
    const auto accent = crashReportMode ? juce::Colour (0xffd4a043)
                                        : juce::Colour (0xff9dbd78);
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour (0xff090b0d));
    juce::ColourGradient bg (juce::Colour (0xff151a1f), 0.0f, 0.0f,
                             juce::Colour (0xff11161b), 0.0f, bounds.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds.reduced (8.0f), 10.0f);

    g.setColour (accent.withAlpha (0.78f));
    g.drawRoundedRectangle (bounds.reduced (8.5f), 10.0f, 1.2f);
    g.setColour (accent.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds.reduced (12.0f), 8.0f, 1.0f);
}

void FeedbackDialog::resized()
{
    auto area = getLocalBounds().reduced (20);

    titleLabel.setBounds (area.removeFromTop (30));
    area.removeFromTop (10);

    infoLabel.setBounds (area.removeFromTop (70));
    area.removeFromTop (5);

    if (! crashReportMode)
    {
        betaSignUpLink.setBounds (area.removeFromTop (22));
        area.removeFromTop (10);
    }
    else
    {
        area.removeFromTop (10);
    }

    auto buttonArea = area.removeFromBottom (30);
    area.removeFromBottom (10);
    feedbackText.setBounds (area);

    // Button layout: [Send to Developer] [Save to File] [Open Form?] [Cancel/Dismiss]
    sendButton.setBounds (buttonArea.removeFromLeft (145));
    buttonArea.removeFromLeft (8);
    saveButton.setBounds (buttonArea.removeFromLeft (100));
    buttonArea.removeFromLeft (8);

    if (! crashReportMode)
    {
        formButton.setBounds (buttonArea.removeFromLeft (90));
        buttonArea.removeFromLeft (8);
    }

    cancelButton.setBounds (buttonArea.removeFromLeft (90));
}

//==============================================================================
// Button handling
//==============================================================================

void FeedbackDialog::buttonClicked (juce::Button* button)
{
    if (button == &sendButton)
        sendToDeveloper();
    else if (button == &saveButton)
        saveFeedback();
    else if (button == &formButton)
        openFeedbackForm();
    else if (button == &cancelButton)
        closeDialog();
}

//==============================================================================
// Actions
//==============================================================================

void FeedbackDialog::sendToDeveloper()
{
    auto body    = buildEmailBody();
    auto subject = crashReportMode
                       ? juce::String ("Choroboros Crash Report")
                       : juce::String ("Choroboros Feedback");

    // Add version to subject
#ifdef CHOROBOROS_VERSION_STRING
    subject << " - v" << juce::String (CHOROBOROS_VERSION_STRING);
#else
    subject << " - v2.04-dev";
#endif

    // Build mailto: URL.  Body is URL-encoded by juce::URL.
    juce::String mailto = "mailto:info@kaizenstrategic.ai"
                          "?subject=" + juce::URL::addEscapeChars (subject, true)
                        + "&body="    + juce::URL::addEscapeChars (body, true);

    juce::URL (mailto).launchInDefaultBrowser();

    // Also save a local copy
    feedbackCollector.saveFeedbackToFile (body);

    closeDialog();
}

juce::String FeedbackDialog::buildEmailBody() const
{
    juce::String body;

    // User's typed message
    auto userText = feedbackText.getText().trim();
    if (userText.isNotEmpty())
    {
        body << userText << "\n\n";
    }

    body << "---\n";

    // Crash report data (if applicable)
    if (crashReportMode && crashReportText.isNotEmpty())
    {
        // Truncate crash report to keep mailto: URL reasonable
        auto truncated = crashReportText.substring (0, 1200);
        body << "CRASH LOG:\n" << truncated << "\n\n";
    }

    // Usage summary (concise)
    body << feedbackCollector.getUsageSummary();

    // Session log summary (if available via collector)
    auto sessionSummary = feedbackCollector.getSessionLogSummary();
    if (sessionSummary.isNotEmpty())
    {
        body << "\nSESSION LOG (last events):\n" << sessionSummary << "\n";
    }

    return body;
}

void FeedbackDialog::saveFeedback()
{
    auto body = buildEmailBody();
    if (feedbackCollector.saveFeedbackToFile (body))
        closeDialog();
    // If save failed, dialog stays open
}

void FeedbackDialog::openFeedbackForm()
{
    juce::URL ("https://docs.google.com/forms/d/e/"
               "1FAIpQLSc5OQpZlMpVSOfcRr6k2nqo5D25M_COfb0qyhCxdj2WmxpGpw/viewform")
        .launchInDefaultBrowser();
    closeDialog();
}

void FeedbackDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState (0);
        dw->setVisible (false);
        // DialogWindow cleans up the content component
        return;
    }

    // Fallback for non-DialogWindow parents
    if (auto* parent = getParentComponent())
        parent->removeChildComponent (this);
    delete this;
}

//==============================================================================
// Static launchers
//==============================================================================

void FeedbackDialog::show (FeedbackCollector& collector)
{
    auto* dialog = new FeedbackDialog (collector);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (dialog);
    options.content->setSize (520, 570);
    options.dialogTitle          = "Feedback";
    options.dialogBackgroundColour = juce::Colour (0xff2a2a2a);
    options.resizable            = true;
    options.useNativeTitleBar    = true;

    auto* window = options.launchAsync();
    if (window != nullptr)
        window->setResizeLimits (500, 440, 800, 800);
}

void FeedbackDialog::showCrashReport (FeedbackCollector& collector,
                                       const juce::String& crashReport)
{
    auto* dialog = new FeedbackDialog (collector, crashReport);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (dialog);
    options.content->setSize (520, 570);
    options.dialogTitle          = "Crash Report";
    options.dialogBackgroundColour = juce::Colour (0xff2a2a2a);
    options.resizable            = true;
    options.useNativeTitleBar    = true;

    auto* window = options.launchAsync();
    if (window != nullptr)
        window->setResizeLimits (500, 440, 800, 800);
}
