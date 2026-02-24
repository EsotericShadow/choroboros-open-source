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
    FeedbackDialog(FeedbackCollector& collector);
    ~FeedbackDialog() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    
    static void show(FeedbackCollector& collector);
    
private:
    FeedbackCollector& feedbackCollector;
    juce::TextEditor feedbackText;
    juce::TextButton saveButton;
    juce::TextButton emailButton;
    juce::TextButton cancelButton;
    juce::Label titleLabel;
    juce::Label infoLabel;
    juce::HyperlinkButton betaSignUpLink;
    
    void saveFeedback();
    void openEmail();
};
