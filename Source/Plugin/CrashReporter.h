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

/**
 * Installs platform-specific crash signal handlers that delete the
 * clean-shutdown marker so the next launch detects the crash.
 *
 * Design rationale — async-signal-safety:
 *   The crash handler does NOT call flushToDisk(), take mutexes, or allocate.
 *   The session log is already periodically flushed to disk by a 30 s Timer,
 *   so the on-disk copy is always at most 30 s stale. The handler's only job
 *   is to remove the clean-shutdown marker file so that next launch can
 *   distinguish a crash from a clean exit.
 *
 *   On macOS/Linux: unlink() is async-signal-safe per POSIX.
 *   On Windows: DeleteFileA() is safe from an exception filter.
 *
 * Multi-instance safety:
 *   Uses a static refcount. install() only installs signal handlers on the
 *   first call; uninstall() only removes them when the last instance exits.
 *   The crash flag path is set once on first install and never changes.
 */
class CrashReporter
{
public:
    /** Install crash handlers. Pass the path to the clean-shutdown marker
        that should be deleted on crash. Refcounted — safe to call from
        multiple PluginProcessor instances. */
    static void install (const juce::File& cleanShutdownMarker);

    /** Decrement refcount; only removes signal handlers when count hits zero. */
    static void uninstall();

private:
    CrashReporter() = delete;
};
