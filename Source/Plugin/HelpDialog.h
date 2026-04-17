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

class HelpDialog : public juce::Component
{
public:
    HelpDialog();
    ~HelpDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label titleLabel;
    juce::Label introLabel;
    juce::Label docsHeaderLabel;
    juce::Label docsBodyLabel;
    juce::Label supportHeaderLabel;
    juce::Label supportBodyLabel;
    juce::Label feedbackHintLabel;
    juce::TextButton docsButton;
    juce::TextButton supportButton;
    juce::TextButton closeButton;

    void openDocs();
    void emailSupport();
    void closeDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpDialog)
};
