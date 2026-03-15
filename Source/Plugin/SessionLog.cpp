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

#if JUCE_MAC || JUCE_LINUX
  #include <unistd.h>   // getpid()
  #include <signal.h>   // kill() for liveness check
#elif JUCE_WINDOWS
  #include <windows.h>  // GetCurrentProcessId()
#endif

//==============================================================================
// Static members
//==============================================================================

std::atomic<int> SessionLog::s_instanceCount { 0 };

//==============================================================================
// PID helper
//==============================================================================

static juce::String getProcessPidString()
{
#if JUCE_MAC || JUCE_LINUX
    return juce::String (static_cast<int> (getpid()));
#elif JUCE_WINDOWS
    return juce::String (static_cast<int> (GetCurrentProcessId()));
#else
    return "0";
#endif
}

//==============================================================================
// File paths — all under ~/Library/Application Support/Choroboros/ (macOS)
// or %APPDATA%/Choroboros/ (Windows).
// Live log and clean-shutdown marker are keyed by PID so multiple DAW
// processes each get their own file, while multiple instances *inside*
// the same DAW correctly share one.
//==============================================================================

juce::File SessionLog::getDataDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Choroboros");
}

juce::File SessionLog::getLiveLogFile()
{
    return getDataDir().getChildFile ("session_log_" + getProcessPidString() + ".json");
}

juce::File SessionLog::getCleanShutdownMarker()
{
    return getDataDir().getChildFile (".clean_shutdown_" + getProcessPidString());
}

juce::File SessionLog::getPendingCrashReportFile()
{
    return getDataDir().getChildFile ("crash_report_pending.txt");
}

//==============================================================================
// Lifecycle
//==============================================================================

SessionLog::SessionLog()
{
    const int prevCount = s_instanceCount.fetch_add (1, std::memory_order_acq_rel);

    if (prevCount == 0)
    {
        // First instance in this process — scan for orphaned logs from
        // previous crashed sessions (any PID) and promote to crash report.
        promoteOrphanedLogs();

        // Delete any stale clean-shutdown marker for our PID (shouldn't
        // exist if we just launched, but handle PID reuse).
        getCleanShutdownMarker().deleteFile();
    }

    // Log session start
    log (EventType::SessionStart, "Session started (instance " + juce::String (prevCount + 1) + ")");

    // Start periodic flush timer (every 30 seconds)
    startTimer (30000);
}

SessionLog::~SessionLog()
{
    stopTimer();
    log (EventType::SessionEnd, "Session ended");

    const int remaining = s_instanceCount.fetch_sub (1, std::memory_order_acq_rel) - 1;

    if (remaining <= 0)
    {
        // Last instance in this process — final flush and clean marker.
        flushToDisk();
        markCleanShutdown();
    }
    else
    {
        // Other instances still alive — just flush, don't mark clean.
        flushToDisk();
    }
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
    const int start = (eventCount >= kRingSize) ? writeIndex : 0;

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
// Process liveness check — used by orphan scan to avoid stealing another
// running DAW's live session log.
//==============================================================================

static bool isProcessAlive (int pid)
{
    if (pid <= 0) return false;

#if JUCE_MAC || JUCE_LINUX
    // kill(pid, 0) checks existence without sending a signal.
    // Returns 0 if process exists, -1/ESRCH if it doesn't.
    return kill (static_cast<pid_t> (pid), 0) == 0;
#elif JUCE_WINDOWS
    HANDLE h = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                            static_cast<DWORD> (pid));
    if (h == nullptr)
        return false;
    DWORD exitCode = 0;
    bool alive = GetExitCodeProcess (h, &exitCode) && exitCode == STILL_ACTIVE;
    CloseHandle (h);
    return alive;
#else
    return false;  // assume dead — safe side
#endif
}

//==============================================================================
// Orphaned log scanning — multi-instance / multi-process safe
//==============================================================================

void SessionLog::promoteOrphanedLogs()
{
    auto dataDir = getDataDir();
    if (! dataDir.isDirectory())
        return;

    auto logFiles = dataDir.findChildFiles (
        juce::File::findFiles, false, "session_log_*.json");

    const auto myPid = getProcessPidString();

    for (const auto& logFile : logFiles)
    {
        // Extract PID from filename: session_log_<pid>.json
        auto name   = logFile.getFileNameWithoutExtension();
        auto pidStr = name.fromLastOccurrenceOf ("_", false, false);

        // Check if a matching clean-shutdown marker exists
        auto markerFile = dataDir.getChildFile (".clean_shutdown_" + pidStr);
        if (markerFile.existsAsFile())
        {
            // Clean shutdown — delete both and move on
            logFile.deleteFile();
            markerFile.deleteFile();
            continue;
        }

        // No clean marker — but this process might still be running.
        // Skip our own PID (we're obviously alive).
        if (pidStr == myPid)
            continue;

        // Check whether that PID is still a live process.  If so, it's
        // another DAW instance still running — leave its log alone.
        const int pid = pidStr.getIntValue();
        if (isProcessAlive (pid))
            continue;

        // PID is dead with no clean marker → genuine crash.
        // Promote to pending crash report (append — there may be
        // multiple crashed PIDs).
        auto pendingFile = getPendingCrashReportFile();
        auto oldLog = logFile.loadFileAsString();
        if (oldLog.isNotEmpty())
        {
            pendingFile.getParentDirectory().createDirectory();

            juce::String report;
            report << "=== Choroboros Crash Report (PID " << pidStr << ") ===\n";
            report << "Previous session did not shut down cleanly.\n";
            report << "Recovered session log follows:\n\n";
            report << oldLog << "\n";

            if (pendingFile.existsAsFile())
                pendingFile.appendText (report);
            else
                pendingFile.replaceWithText (report);
        }

        logFile.deleteFile();
    }
}

//==============================================================================
// Crash report access — read without deleting, separate clear step
//==============================================================================

bool SessionLog::hasPendingCrashReport()
{
    return getPendingCrashReportFile().existsAsFile();
}

juce::String SessionLog::readPendingCrashReport()
{
    auto file = getPendingCrashReportFile();
    if (! file.existsAsFile())
        return {};
    return file.loadFileAsString();
}

void SessionLog::clearPendingCrashReport()
{
    getPendingCrashReportFile().deleteFile();
}

//==============================================================================
// Timer
//==============================================================================

void SessionLog::timerCallback()
{
    flushToDisk();
}
