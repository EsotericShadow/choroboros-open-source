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
#include "../Plugin/PresetManager.h"

//==============================================================================
/**
    Branded header bar sitting at the very top of the plugin window.

    Left:   Kaizen DSP logo mark (floating, no box)
    Centre: Preset browser — chevron nav + combo + save/delete
    Right:  (empty — drawer sits in the main editor area below)

    Design: slim dark bar with subtle engine-colour tint, smooth
    gradient transition into the main content below.
*/
class TopHeaderBar : public juce::Component,
                     public PresetManager::Listener
{
public:
    TopHeaderBar (PresetManager& presetManager, float uiScale);
    ~TopHeaderBar() override;

    /** Design height before UI scaling. */
    static constexpr int kDesignHeight = 36;

    /** Scaled height for this instance. */
    int getBarHeight() const { return barHeight_; }
    void setUiScale (float newUiScale);
    void setAccentColour (juce::Colour newAccent);

    /** Adopt an externally-owned engine-selector ComboBox so the header
        can lay it out.  Ownership stays with the caller. */
    void setEngineSelector (juce::ComboBox* selector);

private:
    void paint (juce::Graphics& g) override;
    void resized() override;

    // PresetManager::Listener
    void presetChanged (const juce::String& name) override;
    void presetListChanged() override;

    void refreshPresetMenu();
    void rebuildLogoImage();
    PresetManager& presetManager_;
    float uiScale_;
    int barHeight_;
    bool updatingPresetMenu_ = false;
    juce::Colour accentColour_;

    juce::Image logoImage_;
    juce::ComboBox* engineSelector_ = nullptr;   // non-owning, lives in editor
    juce::ComboBox presetMenu_;
    juce::TextButton prevButton_    { juce::String::charToString (0x2039) }; // ‹
    juce::TextButton nextButton_    { juce::String::charToString (0x203a) }; // ›
    juce::TextButton saveButton_    { "+" };
    juce::TextButton deleteButton_  { juce::String::charToString (0x2212) }; // −

    void showSaveDialog();
    void showDeleteDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopHeaderBar)
};
