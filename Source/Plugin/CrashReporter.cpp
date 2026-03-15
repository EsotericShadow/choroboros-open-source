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

#include "CrashReporter.h"

#if JUCE_MAC || JUCE_LINUX
  #include <signal.h>
  #include <cstring>
#elif JUCE_WINDOWS
  #include <windows.h>
#endif

//==============================================================================
// Global state — intentionally simple, lives for process lifetime.
//==============================================================================

static SessionLog* g_sessionLog = nullptr;

#if JUCE_MAC || JUCE_LINUX

// Signals we intercept
static constexpr int kCrashSignals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL };
static constexpr int kNumCrashSignals = 5;

// Saved original handlers so we can chain / restore
static struct sigaction g_oldActions[kNumCrashSignals];
static bool g_installed = false;

static void crashSignalHandler (int sig, siginfo_t* info, void* context)
{
    // Minimal work — no allocation, no locks if possible.
    // flushToDisk() does hold a mutex briefly; acceptable in a crash handler
    // since we're about to die anyway and the alternative is losing the log.
    if (g_sessionLog != nullptr)
        g_sessionLog->flushToDisk();

    // Restore the original handler and re-raise so the OS crash reporter runs
    for (int i = 0; i < kNumCrashSignals; ++i)
    {
        if (kCrashSignals[i] == sig)
        {
            sigaction (sig, &g_oldActions[i], nullptr);
            break;
        }
    }

    raise (sig);
}

void CrashReporter::install (SessionLog* sessionLog)
{
    if (g_installed)
        return;

    g_sessionLog = sessionLog;

    struct sigaction sa;
    std::memset (&sa, 0, sizeof (sa));
    sa.sa_sigaction = crashSignalHandler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);

    for (int i = 0; i < kNumCrashSignals; ++i)
        sigaction (kCrashSignals[i], &sa, &g_oldActions[i]);

    g_installed = true;
}

void CrashReporter::uninstall()
{
    if (! g_installed)
        return;

    for (int i = 0; i < kNumCrashSignals; ++i)
        sigaction (kCrashSignals[i], &g_oldActions[i], nullptr);

    g_sessionLog = nullptr;
    g_installed  = false;
}

#elif JUCE_WINDOWS

static LPTOP_LEVEL_EXCEPTION_FILTER g_oldFilter = nullptr;
static bool g_installed = false;

static LONG WINAPI crashExceptionFilter (EXCEPTION_POINTERS* exInfo)
{
    (void) exInfo;

    if (g_sessionLog != nullptr)
        g_sessionLog->flushToDisk();

    if (g_oldFilter != nullptr)
        return g_oldFilter (exInfo);

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashReporter::install (SessionLog* sessionLog)
{
    if (g_installed)
        return;

    g_sessionLog = sessionLog;
    g_oldFilter  = SetUnhandledExceptionFilter (crashExceptionFilter);
    g_installed  = true;
}

void CrashReporter::uninstall()
{
    if (! g_installed)
        return;

    SetUnhandledExceptionFilter (g_oldFilter);
    g_oldFilter  = nullptr;
    g_sessionLog = nullptr;
    g_installed  = false;
}

#else

// Unsupported platform — no-op
void CrashReporter::install (SessionLog*) {}
void CrashReporter::uninstall() {}

#endif
