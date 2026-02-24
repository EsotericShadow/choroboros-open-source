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

class ChoroborosPluginEditor;
class ChoroborosAudioProcessor;

class DevPanel : public juce::Component,
                 private juce::Timer
{
public:
    DevPanel(ChoroborosPluginEditor& editorRef, ChoroborosAudioProcessor& processorRef);
    void resized() override;

private:
    ChoroborosPluginEditor& editor;
    ChoroborosAudioProcessor& processor;

    juce::Viewport viewport;
    juce::Component content;
    juce::Label mappingTitle;
    juce::Label mappingDescription;
    juce::Label uiTitle;
    juce::Label uiDescription;
    juce::Label internalsTitle;
    juce::Label internalsDescription;
    juce::Label bbdTitle;
    juce::Label bbdDescription;
    juce::Label tapeTitle;
    juce::Label tapeDescription;
    juce::Label layoutTitle;
    juce::Label layoutDescription;
    juce::Label activeProfileLabel;
    juce::PropertyPanel mappingPanel;
    juce::PropertyPanel uiPanel;
    juce::PropertyPanel internalsPanel;
    juce::PropertyPanel bbdPanel;
    juce::PropertyPanel tapePanel;
    juce::PropertyPanel layoutPanel;
    juce::Array<juce::PropertyComponent*> lockableProperties;
    juce::TextButton copyJsonButton;
    juce::TextButton saveDefaultsButton;
    juce::TextButton lockToggleButton;
    juce::TextButton fxPresetOffButton;
    juce::TextButton fxPresetSubtleButton;
    juce::TextButton fxPresetMediumButton;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool editingLocked = true;
    int saveButtonResetCountdownTicks = 0;

    juce::String buildJson() const;
    void setEditingLocked(bool shouldLock);
    void saveCurrentAsDefaults();
    void applyValueFxPreset(int presetId);
    void updateActiveProfileLabel();
    void triggerSaveButtonReset();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DevPanel)
};
