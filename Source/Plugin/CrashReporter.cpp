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
#include <atomic>

#if JUCE_MAC || JUCE_LINUX
  #include <signal.h>
  #include <unistd.h>   // unlink() — async-signal-safe per POSIX
  #include <cstring>
#elif JUCE_WINDOWS
  #include <windows.h>
#endif

//==============================================================================
// Global state — intentionally simple, lives for process lifetime.
//==============================================================================

static std::atomic<int> g_refCount { 0 };

// Path to the clean-shutdown marker that will be deleted on crash.
// Stored as a C-string so the signal handler needs no juce::String.
static constexpr int kMaxPathLen = 1024;
static char g_markerPath[kMaxPathLen] = {};

#if JUCE_MAC || JUCE_LINUX

//==============================================================================
// macOS / Linux — signal handlers (async-signal-safe)
//==============================================================================

static constexpr int kCrashSignals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL };
static constexpr int kNumCrashSignals = 5;
static struct sigaction g_oldActions[kNumCrashSignals];
static bool g_handlersInstalled = false;

static void crashSignalHandler (int sig, siginfo_t* /*info*/, void* /*context*/)
{
    // Async-signal-safe: unlink() is guaranteed safe by POSIX.
    // This removes the clean-shutdown marker so next launch knows we crashed.
    // The session log on disk (flushed by the 30 s timer) is already there.
    if (g_markerPath[0] != '\0')
        unlink (g_markerPath);

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

void CrashReporter::install (const juce::File& cleanShutdownMarker)
{
    // Store path as C-string (once, on first install)
    const int prev = g_refCount.fetch_add (1, std::memory_order_acq_rel);
    if (prev > 0)
        return;  // already installed — just bump refcount

    auto pathStr = cleanShutdownMarker.getFullPathName();
    auto pathUtf8 = pathStr.toRawUTF8();
    auto len = std::strlen (pathUtf8);
    if (len < kMaxPathLen)
        std::memcpy (g_markerPath, pathUtf8, len + 1);

    if (g_handlersInstalled)
        return;

    struct sigaction sa;
    std::memset (&sa, 0, sizeof (sa));
    sa.sa_sigaction = crashSignalHandler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);

    for (int i = 0; i < kNumCrashSignals; ++i)
        sigaction (kCrashSignals[i], &sa, &g_oldActions[i]);

    g_handlersInstalled = true;
}

void CrashReporter::uninstall()
{
    const int remaining = g_refCount.fetch_sub (1, std::memory_order_acq_rel) - 1;
    if (remaining > 0)
        return;  // other instances still alive

    if (! g_handlersInstalled)
        return;

    for (int i = 0; i < kNumCrashSignals; ++i)
        sigaction (kCrashSignals[i], &g_oldActions[i], nullptr);

    g_markerPath[0]    = '\0';
    g_handlersInstalled = false;
}

#elif JUCE_WINDOWS

//==============================================================================
// Windows — SetUnhandledExceptionFilter
//==============================================================================

static LPTOP_LEVEL_EXCEPTION_FILTER g_oldFilter = nullptr;
static bool g_handlersInstalled = false;

static LONG WINAPI crashExceptionFilter (EXCEPTION_POINTERS* exInfo)
{
    // DeleteFileA is safe from an exception filter context.
    if (g_markerPath[0] != '\0')
        DeleteFileA (g_markerPath);

    if (g_oldFilter != nullptr)
        return g_oldFilter (exInfo);

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashReporter::install (const juce::File& cleanShutdownMarker)
{
    const int prev = g_refCount.fetch_add (1, std::memory_order_acq_rel);
    if (prev > 0)
        return;

    auto pathStr = cleanShutdownMarker.getFullPathName();
    auto pathUtf8 = pathStr.toRawUTF8();
    auto len = std::strlen (pathUtf8);
    if (len < kMaxPathLen)
        std::memcpy (g_markerPath, pathUtf8, len + 1);

    if (g_handlersInstalled)
        return;

    g_oldFilter = SetUnhandledExceptionFilter (crashExceptionFilter);
    g_handlersInstalled = true;
}

void CrashReporter::uninstall()
{
    const int remaining = g_refCount.fetch_sub (1, std::memory_order_acq_rel) - 1;
    if (remaining > 0)
        return;

    if (! g_handlersInstalled)
        return;

    SetUnhandledExceptionFilter (g_oldFilter);
    g_oldFilter         = nullptr;
    g_markerPath[0]     = '\0';
    g_handlersInstalled = false;
}

#else

// Unsupported platform — no-op
void CrashReporter::install (const juce::File&) {}
void CrashReporter::uninstall() {}

#endif
