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

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class SessionLog;   // forward declare

/**
 * Lightweight feedback and usage tracking for beta version.
 * Privacy-conscious: no personal data, only usage patterns.
 *
 * Optionally holds a reference to a SessionLog for richer diagnostics
 * in feedback emails and crash reports.
 */
class FeedbackCollector
{
public:
    FeedbackCollector();
    ~FeedbackCollector();

    // ---- Event tracking ----------------------------------------------------
    void trackEngineSwitch (int engineIndex, bool hq);
    void trackPresetLoad (int presetIndex, const juce::String& presetName);
    void trackSessionStart();
    void trackSessionEnd();

    /** Flush stats to disk while the message thread is still alive.
        Called from PluginProcessor destructor before reset(). */
    void prepareForShutdown();

    // ---- Host / environment info (set once by PluginProcessor) -------------
    void setHostInfo (const juce::String& hostName,
                      const juce::String& wrapperType,
                      double sampleRate,
                      int blockSize);

    // ---- SessionLog link ---------------------------------------------------
    void setSessionLog (SessionLog* log);
    juce::String getSessionLogSummary() const;

    // ---- Output ------------------------------------------------------------
    juce::String getUsageSummary() const;
    bool saveFeedbackToFile (const juce::String& feedbackText) const;

    // ---- Paths / preferences -----------------------------------------------
    static juce::File getFeedbackDirectory();
    static void clearPersistedAnalyticsData();

private:
    struct UsageStats
    {
        int engineGreenCount  = 0;
        int engineBlueCount   = 0;
        int engineRedCount    = 0;
        int enginePurpleCount = 0;
        int engineBlackCount  = 0;
        int hqEnabledCount    = 0;
        int presetLoads[7]    = {};
        juce::int64 totalSessionTime = 0;
        int sessionCount = 0;
    };

    struct HostInfo
    {
        juce::String hostName;
        juce::String wrapperType;
        double sampleRate = 0.0;
        int blockSize     = 0;
    };

    UsageStats stats;
    HostInfo   hostInfo;
    juce::Time sessionStart;
    SessionLog* sessionLog = nullptr;   // non-owning

    void loadStats();
    void saveStats() const;
    juce::File getStatsFile() const;

    bool shutdownComplete = false;
};
