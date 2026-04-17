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

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ConfirmDialog : public juce::Component
{
public:
    using DecisionCallback = std::function<void(bool)>;

    ConfirmDialog(juce::String title,
                  juce::String message,
                  juce::String confirmText,
                  juce::String cancelText,
                  bool warningTone,
                  DecisionCallback callback);
    ~ConfirmDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void finish(bool accepted);

    const bool warning;
    DecisionCallback decisionCallback;
    juce::Label titleLabel;
    juce::TextEditor messageEditor;
    juce::TextButton confirmButton;
    juce::TextButton cancelButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfirmDialog)
};
