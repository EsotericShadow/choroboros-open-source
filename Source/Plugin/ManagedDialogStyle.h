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

namespace choroboros::ui
{
inline juce::Colour managedDialogBackgroundColour() { return juce::Colour(0xff111111); }
inline juce::Colour managedDialogElevatedColour() { return juce::Colour(0xff191919); }
inline juce::Colour managedDialogBorderColour() { return juce::Colour(0xff2d4437); }
inline juce::Colour managedDialogInfoAccentColour() { return juce::Colour(0xff9ad879); }
inline juce::Colour managedDialogWarningAccentColour() { return juce::Colour(0xffd4a043); }
inline juce::Colour managedDialogBodyTextColour() { return juce::Colours::white.withAlpha(0.90f); }
inline juce::Colour managedDialogMutedTextColour() { return juce::Colours::white.withAlpha(0.72f); }

inline juce::Colour managedDialogAccent(bool warningTone)
{
    return warningTone ? managedDialogWarningAccentColour() : managedDialogInfoAccentColour();
}

inline juce::Font managedDialogTitleFont()
{
    return juce::Font(juce::FontOptions(22.0f).withStyle("Bold"));
}

inline juce::Font managedDialogBodyFont(float height = 14.0f)
{
    return juce::Font(juce::FontOptions(height));
}

inline void styleManagedDialogButton(juce::TextButton& button, bool primary, juce::Colour accent)
{
    button.setColour(juce::TextButton::buttonColourId,
                     primary ? accent.withAlpha(0.16f) : managedDialogElevatedColour());
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.24f));
    button.setColour(juce::TextButton::textColourOffId,
                     primary ? accent : managedDialogBodyTextColour());
    button.setColour(juce::TextButton::textColourOnId, accent);
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.24f));
}

inline void styleManagedDialogEditor(juce::TextEditor& editor, juce::Colour accent)
{
    editor.setColour(juce::TextEditor::backgroundColourId, managedDialogElevatedColour());
    editor.setColour(juce::TextEditor::outlineColourId, managedDialogBorderColour());
    editor.setColour(juce::TextEditor::focusedOutlineColourId, accent.withAlpha(0.8f));
    editor.setColour(juce::TextEditor::textColourId, managedDialogBodyTextColour());
    editor.setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.20f));
    editor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    editor.setFont(managedDialogBodyFont());
    editor.setBorder(juce::BorderSize<int>(12));
}

inline void paintManagedDialogBackground(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
{
    g.fillAll(managedDialogBackgroundColour());
    juce::ColourGradient bg(managedDialogElevatedColour(), 0.0f, 0.0f,
                            managedDialogBackgroundColour(), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(6.0f), 8.0f);

    g.setColour(managedDialogBorderColour().withAlpha(0.7f));
    g.drawRoundedRectangle(bounds.reduced(6.5f), 8.0f, 1.0f);

    g.setColour(accent.withAlpha(0.06f));
    g.drawRoundedRectangle(bounds.reduced(10.0f), 6.0f, 0.8f);
}
} // namespace choroboros::ui
