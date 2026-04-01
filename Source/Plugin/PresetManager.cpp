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

#include "PresetManager.h"
#include "PluginProcessor.h"
#include "PresetState.h"
#include "ApplyContext.h"

//==============================================================================
PresetManager::PresetManager (ChoroborosAudioProcessor& processor)
    : processor_ (processor)
{
    scanUserPresets();
}

//==============================================================================
// Directory

juce::File PresetManager::getUserPresetsDirectory() const
{
    auto dir = juce::File::getSpecialLocation (
                   juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                   .getChildFile ("Application Support")
#endif
                   .getChildFile ("Kaizen DSP")
                   .getChildFile ("Choroboros")
                   .getChildFile ("Presets");

    dir.createDirectory();
    return dir;
}

//==============================================================================
// Scanning

void PresetManager::scanUserPresets()
{
    userPresetNames_.clear();
    userPresetFiles_.clear();

    auto dir = getUserPresetsDirectory();

    // Support both new .json format and legacy .xml format
    auto jsonFiles = dir.findChildFiles (juce::File::findFiles, false, "*.json");
    auto xmlFiles = dir.findChildFiles (juce::File::findFiles, false, "*.xml");

    juce::Array<juce::File> allFiles;
    allFiles.addArray(jsonFiles);
    allFiles.addArray(xmlFiles);
    allFiles.sort();

    for (auto& f : allFiles)
    {
        userPresetNames_.add (f.getFileNameWithoutExtension());
        userPresetFiles_.add (f);
    }

    listeners_.call (&Listener::presetListChanged);
}

//==============================================================================
// Counts

int PresetManager::getNumFactoryPresets() const
{
    return processor_.getNumPrograms();
}

int PresetManager::getNumPresets() const
{
    return getNumFactoryPresets() + userPresetNames_.size();
}

bool PresetManager::isUserPreset (int index) const
{
    return index >= getNumFactoryPresets();
}

//==============================================================================
// Browsing

juce::String PresetManager::getCurrentPresetName() const
{
    if (currentIndex_ < 0)
        return {};   // No preset active — caller should show placeholder.

    if (currentIndex_ < getNumFactoryPresets())
        return processor_.getProgramName (currentIndex_);

    const int userIdx = currentIndex_ - getNumFactoryPresets();
    if (userIdx >= 0 && userIdx < userPresetNames_.size())
        return userPresetNames_[userIdx];

    return {};
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    const int factoryCount = getNumFactoryPresets();
    names.ensureStorageAllocated (factoryCount + userPresetNames_.size());

    for (int i = 0; i < factoryCount; ++i)
        names.add (processor_.getProgramName (i));

    names.addArray (userPresetNames_);
    return names;
}

void PresetManager::loadPreset (int index)
{
    const int total = getNumPresets();
    if (total == 0)
        return;

    // Guard: prevent re-entrant invalidation while parameter values change
    // as a side-effect of the preset load.
    loadInProgress_ = true;

    currentIndex_ = juce::jlimit (0, total - 1, index);

    if (currentIndex_ < getNumFactoryPresets())
    {
        // Factory preset — load through canonical path
        if (const auto presetState = PresetState::makeFactoryPreset(currentIndex_))
        {
            processor_.applyPresetState(presetState.value(), ApplyContext::FactoryPresetLoad);
        }
    }
    else
    {
        // User preset — load through canonical path
        const int userIdx = currentIndex_ - getNumFactoryPresets();
        if (userIdx >= 0 && userIdx < userPresetFiles_.size())
        {
            auto file = userPresetFiles_[userIdx];
            if (const auto presetState = PresetState::loadFromFile(file))
            {
                processor_.applyPresetState(presetState.value(), ApplyContext::UserPresetLoad);
            }
        }
    }

    loadInProgress_ = false;
    listeners_.call (&Listener::presetChanged, getCurrentPresetName());
}

void PresetManager::nextPreset()
{
    const int total = getNumPresets();
    if (total == 0)
        return;

    // If no preset is active (index == -1), start from the first preset.
    if (currentIndex_ < 0)
        loadPreset (0);
    else
        loadPreset ((currentIndex_ + 1) % total);
}

void PresetManager::previousPreset()
{
    const int total = getNumPresets();
    if (total == 0)
        return;

    // If no preset is active (index == -1), start from the last preset.
    if (currentIndex_ < 0)
        loadPreset (total - 1);
    else
        loadPreset ((currentIndex_ - 1 + total) % total);
}

void PresetManager::invalidatePreset()
{
    if (currentIndex_ < 0)
        return;   // Already invalidated — nothing to do.

    currentIndex_ = -1;
    listeners_.call (&Listener::presetChanged, juce::String());
}

//==============================================================================
// User preset management

void PresetManager::saveUserPreset (const juce::String& name)
{
    if (name.isEmpty())
        return;

    // Capture current state through canonical path
    const auto presetState = processor_.capturePresetState();

    // Save to file
    auto dir = getUserPresetsDirectory();
    auto file = dir.getChildFile (name + ".json");

    if (!presetState.saveToFile(file))
        return;

    scanUserPresets();

    // Select the newly saved preset
    const int factoryCount = getNumFactoryPresets();
    for (int i = 0; i < userPresetNames_.size(); ++i)
    {
        if (userPresetNames_[i] == name)
        {
            currentIndex_ = factoryCount + i;
            break;
        }
    }

    listeners_.call (&Listener::presetChanged, getCurrentPresetName());
}

bool PresetManager::deleteUserPreset (int index)
{
    if (! isUserPreset (index))
        return false;

    const int userIdx = index - getNumFactoryPresets();
    if (userIdx < 0 || userIdx >= userPresetFiles_.size())
        return false;

    auto file = userPresetFiles_[userIdx];
    if (! file.deleteFile())
        return false;

    scanUserPresets();

    // After deletion, invalidate — no preset is active.
    currentIndex_ = -1;
    listeners_.call (&Listener::presetChanged, juce::String());
    return true;
}
