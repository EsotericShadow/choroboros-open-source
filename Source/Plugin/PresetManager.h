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

#include <juce_audio_processors/juce_audio_processors.h>

class ChoroborosAudioProcessor;

//==============================================================================
/**
    Manages factory presets and user-saved presets for Choroboros.

    Factory presets are the 7 built-in programs from PluginProcessor.
    User presets are stored as XML files in a dedicated directory.

    currentIndex_ == -1 means "no preset active" — the user has tweaked
    something (like the engine colour) that takes the state away from any
    stored preset.
*/
class PresetManager
{
public:
    explicit PresetManager (ChoroborosAudioProcessor& processor);

    //==========================================================================
    // Preset browsing

    /** Total number of presets (factory + user). */
    int getNumPresets() const;

    /** Name of the current preset, or empty string if none active. */
    juce::String getCurrentPresetName() const;

    /** All preset names in display order (factory first, then user presets). */
    juce::StringArray getPresetNames() const;

    /** Index of the current preset. -1 means no preset is active. */
    int getCurrentIndex() const { return currentIndex_; }

    /** True when a valid preset is selected. */
    bool hasActivePreset() const { return currentIndex_ >= 0; }

    /** Load the next preset (wraps around). */
    void nextPreset();

    /** Load the previous preset (wraps around). */
    void previousPreset();

    /** Load a specific preset by index. */
    void loadPreset (int index);

    /** Mark the current preset as invalid (user tweaked state away from it).
        Notifies listeners so the UI can show "no preset" placeholder. */
    void invalidatePreset();

    /** True while a preset is being loaded — use this to avoid re-entrant
        invalidation when parameters change as a side-effect of the load. */
    bool isLoadInProgress() const { return loadInProgress_; }

    //==========================================================================
    // User preset management

    /** Save the current processor state as a user preset with the given name. */
    void saveUserPreset (const juce::String& name);

    /** Delete a user preset by index. Only works for user presets (not factory). */
    bool deleteUserPreset (int index);

    /** Returns true if the given index is a user preset (not factory). */
    bool isUserPreset (int index) const;

    /** Number of factory presets. */
    int getNumFactoryPresets() const;

    /** Refresh the list of user presets from disk. */
    void scanUserPresets();

    //==========================================================================
    // Listeners

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void presetChanged (const juce::String& name) = 0;
        virtual void presetListChanged() = 0;
    };

    void addListener (Listener* l)    { listeners_.add (l); }
    void removeListener (Listener* l) { listeners_.remove (l); }

private:
    ChoroborosAudioProcessor& processor_;

    int currentIndex_ = -1;           // -1 = no preset active
    bool loadInProgress_ = false;     // guard against re-entrant invalidation
    juce::StringArray userPresetNames_;
    juce::Array<juce::File> userPresetFiles_;

    juce::File getUserPresetsDirectory() const;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
