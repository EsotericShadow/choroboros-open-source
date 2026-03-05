/*
 * Choroboros - Defaults persistence service (SRP: config file I/O only)
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#pragma once

#include <juce_core/juce_core.h>

/** Handles read/write of defaults.json and recovery files. No UI, no DSP. */
struct DefaultsPersistence
{
    static juce::File getUserDefaultsFile();
    static juce::File getFactoryDefaultsFile();

    /** Legacy compatibility alias (returns user defaults path). */
    static juce::File getDefaultsFile();
    static juce::File getRecoveryFile(bool timestamped);
    static juce::File getFailureLogFile();

    /** Write JSON to user defaults. */
    static bool saveUser(const juce::String& json, juce::String* outError = nullptr);
    /** Read user defaults. Migrates legacy defaults.json if needed. */
    static juce::String loadUser(juce::String* outError = nullptr);
    /** Write JSON to factory defaults sheet. */
    static bool saveFactory(const juce::String& json, juce::String* outError = nullptr);
    /** Read factory defaults sheet. */
    static juce::String loadFactory(juce::String* outError = nullptr);
    /** Copy factory defaults sheet over user defaults sheet. */
    static bool resetUserToFactory(juce::String* outError = nullptr);

    /** Write JSON to defaults file. On failure, writes recovery file and logs. */
    static bool save(const juce::String& json, juce::String* outError = nullptr);

    /** Read defaults file. Returns empty string on failure. */
    static juce::String load(juce::String* outError = nullptr);

    /** Append a failure line to the log (for diagnostics). */
    static void logFailure(const juce::String& reason);
};
