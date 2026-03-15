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
 * disk so the log survives crashes. On clean shutdown a "clean" marker is
 * written; if absent on next launch the previous session is treated as a crash.
 *
 * Thread-safe: the ring buffer is guarded by a mutex. Audio-thread callers
 * use tryLog() which does a try_lock and silently drops the event on contention
 * rather than blocking the realtime thread.
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

    /** Flush the ring buffer to the "live" log file on disk. */
    void flushToDisk();

    /** Write a clean-shutdown marker so next launch knows we exited OK. */
    void markCleanShutdown();

    /** Returns true if the previous session did NOT shut down cleanly. */
    static bool hasPendingCrashReport();

    /** Read the pending crash log and return it as a human-readable string.
        Deletes the pending file after reading. */
    static juce::String consumePendingCrashReport();

    /** Get a concise summary of the current session log (for feedback email body). */
    juce::String getSessionSummary() const;

    //--------------------------------------------------------------------------
    // Paths
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

    void pushEvent (EventType type, const juce::String& detail);
    juce::String formatLog() const;   // caller must hold mtx

    // Timer callback — periodic flush
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionLog)
};
