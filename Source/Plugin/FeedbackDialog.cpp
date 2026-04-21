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
#include "PluginProcessor.h"
#include "SessionLog.h"
#include "../UI/DevPanelSupport.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace
{
constexpr auto kFeedbackFormUrl =
    "https://docs.google.com/forms/d/e/"
    "1FAIpQLSc5OQpZlMpVSOfcRr6k2nqo5D25M_COfb0qyhCxdj2WmxpGpw/viewform";

bool saveFeedbackBodyToFile(const juce::String& body)
{
    auto feedbackDir = FeedbackCollector::getFeedbackDirectory();
    if (! feedbackDir.createDirectory())
        return false;

    auto timestamp = juce::Time::getCurrentTime()
                         .toString(true, true)
                         .replaceCharacters(":", "-");
    auto feedbackFile = feedbackDir.getChildFile("feedback_" + timestamp + ".txt");
    return feedbackFile.replaceWithText(body);
}
}

//==============================================================================
// Constructors
//==============================================================================

FeedbackDialog::FeedbackDialog (FeedbackCollector* collector,
                                MessageCallback callback)
    : feedbackCollector (collector),
      showMessageCallback (std::move(callback))
{
    initCommon();
}

FeedbackDialog::FeedbackDialog (const juce::String& crashReport,
                                FeedbackCollector* collector,
                                MessageCallback callback)
    : feedbackCollector (collector),
      showMessageCallback (std::move(callback)),
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
    setSize (520, 560);

    const auto accent = devpanel::hackerText();        // bright green
    const auto body   = devpanel::hackerTextDim();     // softer green
    const auto muted  = devpanel::hackerTextMuted();   // dim green
    // Crash-mode uses amber from DevPanel's StagePalette-style warning
    const auto warn   = juce::Colour (0xffd4a043);

    // --- Title ---
    if (crashReportMode)
        titleLabel.setText ("Choroboros \u2014 Crash Report", juce::dontSendNotification);
    else
        titleLabel.setText ("Choroboros \u2014 Feedback", juce::dontSendNotification);

    titleLabel.setFont (devpanel::makeTitleFont (devpanel::Typography::title, true));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, crashReportMode ? warn : accent);
    addAndMakeVisible (titleLabel);

    // --- Info label ---
    if (crashReportMode)
    {
        infoLabel.setText ("It looks like Choroboros didn't close properly last time.\n"
                           "Help us fix this \u2014 send the crash log to the developer.\n"
                           "Add any details about what you were doing when it happened.",
                           juce::dontSendNotification);
    }
    else
    {
        infoLabel.setText ("Share bug reports, feature requests, or general feedback.\n"
                           "Click \"Send to Developer\" to open your default mail app with the report prefilled.",
                           juce::dontSendNotification);
    }
    infoLabel.setFont (devpanel::makeLabelFont (devpanel::Typography::description, false));
    infoLabel.setJustificationType (juce::Justification::centred);
    infoLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (infoLabel);

    // --- Feedback text editor (hacker-styled) ---
    feedbackText.setMultiLine (true, true);
    feedbackText.setReturnKeyStartsNewLine (true);
    feedbackText.setFont (devpanel::makeLabelFont (devpanel::Typography::description, false));
    devpanel::styleHackerEditor (feedbackText);
    feedbackText.setColour (juce::CaretComponent::caretColourId, accent);
    addAndMakeVisible (feedbackText);

    // --- Buttons (all hacker-styled) ---
    devpanel::styleHackerTextButton (sendButton, true);   // primary
    sendButton.setButtonText ("Send to Developer");
    sendButton.addListener (this);
    addAndMakeVisible (sendButton);

    devpanel::styleHackerTextButton (saveButton, false);
    saveButton.setButtonText ("Save to File");
    saveButton.addListener (this);
    addAndMakeVisible (saveButton);

    if (! crashReportMode)
    {
        devpanel::styleHackerTextButton (formButton, false);
        formButton.setButtonText ("Feedback Form");
        formButton.addListener (this);
        addAndMakeVisible (formButton);
    }

    devpanel::styleHackerTextButton (cancelButton, false);
    cancelButton.setButtonText (crashReportMode ? "Dismiss" : "Cancel");
    cancelButton.addListener (this);
    addAndMakeVisible (cancelButton);
}

//==============================================================================
// Paint / Layout
//==============================================================================

void FeedbackDialog::paint (juce::Graphics& g)
{
    const auto accent = crashReportMode ? juce::Colour (0xffd4a043)
                                        : devpanel::hackerText();
    const auto bounds = getLocalBounds().toFloat();

    // HackerTheme background
    g.fillAll (devpanel::hackerBg());
    juce::ColourGradient bg (devpanel::hackerBgElevated(), 0.0f, 0.0f,
                             devpanel::hackerBg(), 0.0f, bounds.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds.reduced (6.0f), 8.0f);

    // Border
    g.setColour (devpanel::hackerBorder().withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.reduced (6.5f), 8.0f, 1.0f);

    // Subtle inner glow
    g.setColour (accent.withAlpha (0.06f));
    g.drawRoundedRectangle (bounds.reduced (10.0f), 6.0f, 0.8f);
}

void FeedbackDialog::resized()
{
    auto area = getLocalBounds().reduced (20);

    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    infoLabel.setBounds (area.removeFromTop (50));
    area.removeFromTop (12);

    auto buttonArea = area.removeFromBottom (28);
    area.removeFromBottom (10);
    feedbackText.setBounds (area);

    // Button layout: [Send to Developer] [Save to File] [Feedback Form?] [Cancel/Dismiss]
    sendButton.setBounds (buttonArea.removeFromLeft (140));
    buttonArea.removeFromLeft (6);
    saveButton.setBounds (buttonArea.removeFromLeft (95));
    buttonArea.removeFromLeft (6);

    if (! crashReportMode)
    {
        formButton.setBounds (buttonArea.removeFromLeft (110));
        buttonArea.removeFromLeft (6);
    }

    cancelButton.setBounds (buttonArea.removeFromLeft (85));
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
    {
        if (crashReportMode)
            SessionLog::clearPendingCrashReport();
        closeDialog();
    }
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

#ifdef CHOROBOROS_VERSION_STRING
    subject << " - v" << juce::String (CHOROBOROS_VERSION_STRING);
#else
    subject << " - v2.05";
#endif

    juce::String mailto = "mailto:info@kaizenstrategic.ai"
                          "?subject=" + juce::URL::addEscapeChars (subject, true)
                        + "&body="    + juce::URL::addEscapeChars (body, true);

    const bool launched = juce::URL (mailto).launchInDefaultBrowser();
    const bool saved = feedbackCollector != nullptr
        ? feedbackCollector->saveFeedbackToFile (body)
        : saveFeedbackBodyToFile (body);

    if (launched)
    {
        if (crashReportMode)
            SessionLog::clearPendingCrashReport();

        closeDialog();
        return;
    }

    if (showMessageCallback)
    {
        juce::String message =
            "Choroboros couldn't open your default mail app.\n\n"
            "Send your report manually to:\n"
            "info@kaizenstrategic.ai\n\n";

        if (saved)
        {
            message << "A copy of this report was saved in:\n"
                    << FeedbackCollector::getFeedbackDirectory().getFullPathName()
                    << "\n\n";
        }
        else
        {
            message << "Choroboros also couldn't save a local copy automatically.\n\n";
        }

        message << "You can also use the Feedback Form button from this dialog.";

        showMessageCallback (juce::AlertWindow::WarningIcon,
                             "Couldn't Open Mail App",
                             message);
    }
}

juce::String FeedbackDialog::buildEmailBody() const
{
    juce::String body;

    auto userText = feedbackText.getText().trim();
    if (userText.isNotEmpty())
        body << userText << "\n\n";

    body << "---\n";

    if (crashReportMode && crashReportText.isNotEmpty())
    {
        auto truncated = crashReportText.substring (0, 1200);
        body << "CRASH LOG:\n" << truncated << "\n\n";
    }

    if (feedbackCollector != nullptr)
    {
        body << feedbackCollector->getUsageSummary();

        auto sessionSummary = feedbackCollector->getSessionLogSummary();
        if (sessionSummary.isNotEmpty())
            body << "\nSESSION LOG (last events):\n" << sessionSummary << "\n";
    }

    return body;
}

void FeedbackDialog::saveFeedback()
{
    auto body = buildEmailBody();
    const bool saved = feedbackCollector != nullptr
        ? feedbackCollector->saveFeedbackToFile(body)
        : saveFeedbackBodyToFile(body);

    if (saved)
    {
        if (crashReportMode)
            SessionLog::clearPendingCrashReport();
        closeDialog();
        return;
    }

    if (showMessageCallback)
    {
        showMessageCallback(juce::AlertWindow::WarningIcon,
                            "Couldn't Save Feedback",
                            "Choroboros couldn't save your report to disk.\n\n"
                            "Try Send to Developer instead, or copy your notes out before closing the window.");
    }
}

void FeedbackDialog::openFeedbackForm()
{
    if (! juce::URL (kFeedbackFormUrl).launchInDefaultBrowser())
    {
        if (showMessageCallback)
        {
            showMessageCallback(juce::AlertWindow::WarningIcon,
                                "Couldn't Open Feedback Form",
                                "Choroboros couldn't open your default browser.\n\n"
                                "Open this URL manually:\n" + juce::String(kFeedbackFormUrl));
        }

        return;
    }

    closeDialog();
}

void FeedbackDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState (0);
        dw->setVisible (false);
        return;
    }

    // Defer destruction so the call stack unwinds before `this` is deleted.
    // SafePointer guards against the component being destroyed by other means
    // before the async callback fires.
    juce::Component::SafePointer<FeedbackDialog> safeThis(this);
    if (auto* parent = getParentComponent())
        parent->removeChildComponent(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        delete safeThis.getComponent();
    });
}
