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

#include <juce_gui_basics/juce_gui_basics.h>
#include "FeedbackCollector.h"

class FeedbackDialog : public juce::Component,
                       public juce::Button::Listener
{
public:
    /** Normal feedback mode. */
    FeedbackDialog (FeedbackCollector& collector);

    /** Crash report mode — pre-fills with crash context. */
    FeedbackDialog (FeedbackCollector& collector, const juce::String& crashReport);

    ~FeedbackDialog() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

    /** Launch feedback dialog (normal mode). */
    static void show (FeedbackCollector& collector);

    /** Launch crash report dialog (pre-filled with crash log). */
    static void showCrashReport (FeedbackCollector& collector,
                                 const juce::String& crashReport);

private:
    FeedbackCollector& feedbackCollector;
    bool crashReportMode = false;
    juce::String crashReportText;

    juce::TextEditor feedbackText;
    juce::TextButton sendButton;       // "Send to Developer" — mailto:
    juce::TextButton saveButton;       // "Save to File"
    juce::TextButton formButton;       // "Open Feedback Form"
    juce::TextButton cancelButton;
    juce::Label titleLabel;
    juce::Label infoLabel;
    juce::HyperlinkButton betaSignUpLink;

    void initCommon();
    void sendToDeveloper();
    void saveFeedback();
    void openFeedbackForm();
    void closeDialog();

    /** Build the mailto: body from user text + session/crash data. */
    juce::String buildEmailBody() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackDialog)
};
