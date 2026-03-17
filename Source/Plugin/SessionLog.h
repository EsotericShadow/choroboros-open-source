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
#include <juce_events/juce_events.h>
#include <array>
#include <mutex>

/**
 * Lightweight session event log for diagnostics and crash reporting.
 *
 * Maintains a fixed-size ring buffer of recent plugin events (engine switches,
 * HQ toggles, preset loads, DSP anomalies, errors). Periodically flushed to
 * disk so the log survives crashes.
 *
 * Multi-instance safe: each SessionLog keys its live log and clean-shutdown
 * marker by the host process PID, so multiple plugin instances in the same
 * DAW share one log file (correct — they share one process) and instances in
 * different DAWs don't collide. On startup the *first* instance to construct
 * scans for orphaned logs from dead PIDs and promotes them to crash reports.
 *
 * Thread-safe: the ring buffer is guarded by a mutex. Audio-thread callers
 * use tryLog() which does a try_lock and silently drops the event on
 * contention rather than blocking the realtime thread.
 */
class SessionLog : private juce::Timer
{
public:
    //--------------------------------------------------------------------------
    enum class EventType
    {
        SessionStart,
        SessionEnd,
        EngineSwitch,
        HqToggle,
        PresetLoad,
        CoreSwitch,
        ParamChange,       // only logged for significant jumps
        DspAnomaly,        // NaN, Inf, sustained clipping
        Error,             // file I/O failure, state error, etc.
        HostInfo           // DAW name, sample rate, buffer size
    };

    struct Event
    {
        juce::int64  timestampMs = 0;   // millis since epoch
        EventType    type = EventType::SessionStart;
        juce::String detail;            // short description, ≤120 chars
    };

    //--------------------------------------------------------------------------
    SessionLog();
    ~SessionLog() override;

    /** Log an event from the message thread (guaranteed write). */
    void log (EventType type, const juce::String& detail = {});

    /** Log an event from the audio thread (non-blocking, may drop). */
    void tryLog (EventType type, const juce::String& detail = {});

    /** Call before destruction to perform final flush and marker write while
        the message thread and file system are still fully available.
        Prevents file I/O from running inside the destructor (which may
        execute during DLL_PROCESS_DETACH on Windows, where I/O can hang). */
    void prepareForShutdown();

    /** Flush the ring buffer to the "live" log file on disk. */
    void flushToDisk();

    /** Write a clean-shutdown marker so next launch knows we exited OK. */
    void markCleanShutdown();

    /** Returns true if there is at least one pending crash report. */
    static bool hasPendingCrashReport();

    /** Read the pending crash log and return it as a human-readable string.
        Does NOT delete the file — call clearPendingCrashReport() after
        the user has successfully sent or dismissed it. */
    static juce::String readPendingCrashReport();

    /** Delete the pending crash report file (call after user action). */
    static void clearPendingCrashReport();

    /** Get a concise summary of the current session log (for feedback email body). */
    juce::String getSessionSummary() const;

    //--------------------------------------------------------------------------
    // Paths (PID-keyed for multi-instance safety)
    static juce::File getDataDir();
    static juce::File getLiveLogFile();
    static juce::File getCleanShutdownMarker();
    static juce::File getPendingCrashReportFile();

private:
    //--------------------------------------------------------------------------
    static constexpr int kRingSize = 64;

    std::array<Event, kRingSize> ring;
    int writeIndex = 0;
    int eventCount = 0;          // total events written (may exceed kRingSize)
    mutable std::mutex mtx;

    /** Instance ref-count: first ctor in this process scans for orphaned logs;
        last dtor writes the clean-shutdown marker. */
    static std::atomic<int> s_instanceCount;

    void pushEvent (EventType type, const juce::String& detail);
    juce::String formatLog() const;   // caller must hold mtx

    /** Scan for session_log_*.json files with no matching .clean_shutdown_*
        and promote them to crash reports. Only runs once per process. */
    static void promoteOrphanedLogs();

    // Timer callback — periodic flush
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionLog)
};
