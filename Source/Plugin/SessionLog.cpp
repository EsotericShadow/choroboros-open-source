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

#include "SessionLog.h"

//==============================================================================
// File paths — all under ~/Library/Application Support/Choroboros/ (macOS)
// or %APPDATA%/Choroboros/ (Windows)
//==============================================================================

static juce::File getChoroborosDataDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Choroboros");
}

juce::File SessionLog::getLiveLogFile()
{
    return getChoroborosDataDir().getChildFile ("session_log.json");
}

juce::File SessionLog::getCleanShutdownMarker()
{
    return getChoroborosDataDir().getChildFile (".clean_shutdown");
}

juce::File SessionLog::getPendingCrashReportFile()
{
    return getChoroborosDataDir().getChildFile ("crash_report_pending.txt");
}

//==============================================================================
// Lifecycle
//==============================================================================

SessionLog::SessionLog()
{
    // If there is a live log but no clean-shutdown marker, the previous
    // session crashed. Promote the live log to a pending crash report
    // before we start overwriting it.
    auto liveLog = getLiveLogFile();
    auto marker  = getCleanShutdownMarker();

    if (liveLog.existsAsFile() && ! marker.existsAsFile())
    {
        auto pendingFile = getPendingCrashReportFile();
        if (! pendingFile.existsAsFile())
        {
            // Read the old live log and write it as the pending crash report
            auto oldLog = liveLog.loadFileAsString();
            if (oldLog.isNotEmpty())
            {
                pendingFile.getParentDirectory().createDirectory();
                juce::String report;
                report << "=== Choroboros Crash Report ===\n";
                report << "Previous session did not shut down cleanly.\n";
                report << "Recovered session log follows:\n\n";
                report << oldLog;
                pendingFile.replaceWithText (report);
            }
        }
    }

    // Delete the old clean-shutdown marker (we'll write a new one on exit)
    marker.deleteFile();

    // Log session start
    log (EventType::SessionStart, "Session started");

    // Start periodic flush timer (every 30 seconds)
    startTimer (30000);
}

SessionLog::~SessionLog()
{
    stopTimer();
    log (EventType::SessionEnd, "Session ended");
    flushToDisk();
    markCleanShutdown();
}

//==============================================================================
// Event logging
//==============================================================================

void SessionLog::log (EventType type, const juce::String& detail)
{
    std::lock_guard<std::mutex> lock (mtx);
    pushEvent (type, detail);
}

void SessionLog::tryLog (EventType type, const juce::String& detail)
{
    std::unique_lock<std::mutex> lock (mtx, std::try_to_lock);
    if (lock.owns_lock())
        pushEvent (type, detail);
    // else: silently drop — never block the audio thread
}

void SessionLog::pushEvent (EventType type, const juce::String& detail)
{
    auto& event      = ring[static_cast<size_t> (writeIndex)];
    event.timestampMs = juce::Time::currentTimeMillis();
    event.type        = type;
    event.detail      = detail.substring (0, 120);  // cap length

    writeIndex = (writeIndex + 1) % kRingSize;
    eventCount++;
}

//==============================================================================
// Formatting
//==============================================================================

static const char* eventTypeName (SessionLog::EventType t)
{
    switch (t)
    {
        case SessionLog::EventType::SessionStart:  return "SESSION_START";
        case SessionLog::EventType::SessionEnd:    return "SESSION_END";
        case SessionLog::EventType::EngineSwitch:  return "ENGINE";
        case SessionLog::EventType::HqToggle:      return "HQ_TOGGLE";
        case SessionLog::EventType::PresetLoad:    return "PRESET";
        case SessionLog::EventType::CoreSwitch:    return "CORE_SWITCH";
        case SessionLog::EventType::ParamChange:   return "PARAM";
        case SessionLog::EventType::DspAnomaly:    return "DSP_ANOMALY";
        case SessionLog::EventType::Error:         return "ERROR";
        case SessionLog::EventType::HostInfo:      return "HOST_INFO";
    }
    return "UNKNOWN";
}

juce::String SessionLog::formatLog() const
{
    // Caller must hold mtx
    juce::String out;

    const int total = juce::jmin (eventCount, kRingSize);
    // Start from the oldest event in the ring
    const int start = (eventCount >= kRingSize)
                          ? writeIndex          // ring has wrapped
                          : 0;

    for (int i = 0; i < total; ++i)
    {
        const auto& ev = ring[static_cast<size_t> ((start + i) % kRingSize)];
        auto time = juce::Time (ev.timestampMs);

        out << time.formatted ("%Y-%m-%d %H:%M:%S")
            << " [" << eventTypeName (ev.type) << "] "
            << ev.detail << "\n";
    }

    return out;
}

juce::String SessionLog::getSessionSummary() const
{
    std::lock_guard<std::mutex> lock (mtx);
    return formatLog();
}

//==============================================================================
// Disk I/O
//==============================================================================

void SessionLog::flushToDisk()
{
    juce::String logText;
    {
        std::lock_guard<std::mutex> lock (mtx);
        logText = formatLog();
    }

    auto file = getLiveLogFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (logText);
}

void SessionLog::markCleanShutdown()
{
    auto marker = getCleanShutdownMarker();
    marker.getParentDirectory().createDirectory();
    marker.replaceWithText ("ok");

    // Clean up the live log file on graceful exit
    getLiveLogFile().deleteFile();
}

//==============================================================================
// Crash report detection
//==============================================================================

bool SessionLog::hasPendingCrashReport()
{
    return getPendingCrashReportFile().existsAsFile();
}

juce::String SessionLog::consumePendingCrashReport()
{
    auto file = getPendingCrashReportFile();
    if (! file.existsAsFile())
        return {};

    auto content = file.loadFileAsString();
    file.deleteFile();
    return content;
}

//==============================================================================
// Timer
//==============================================================================

void SessionLog::timerCallback()
{
    flushToDisk();
}
