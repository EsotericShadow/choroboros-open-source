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

#include "SessionLog.h"

/**
 * Installs platform-specific crash signal handlers that flush the SessionLog
 * to disk before the process terminates.
 *
 * On macOS/Linux: catches SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL.
 * On Windows: installs SetUnhandledExceptionFilter.
 *
 * The handler does minimal work (no allocation):
 *   1. Calls SessionLog::flushToDisk() on the global instance
 *   2. Re-raises the original signal so the OS crash reporter still fires
 *
 * Usage:
 *   CrashReporter::install(&mySessionLog);   // once, during processor init
 *   CrashReporter::uninstall();              // optional, during shutdown
 */
class CrashReporter
{
public:
    /** Install crash handlers. Call once during PluginProcessor construction.
        The sessionLog pointer must remain valid for the lifetime of the plugin. */
    static void install (SessionLog* sessionLog);

    /** Remove crash handlers (optional — safe to skip if process is exiting). */
    static void uninstall();

private:
    CrashReporter() = delete;
};
