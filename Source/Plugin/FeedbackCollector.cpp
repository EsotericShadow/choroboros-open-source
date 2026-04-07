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

#include "FeedbackCollector.h"
#include "SessionLog.h"

//==============================================================================
// Lifecycle
//==============================================================================

FeedbackCollector::FeedbackCollector()
{
    loadStats();
    trackSessionStart();
}

void FeedbackCollector::prepareForShutdown()
{
    trackSessionEnd();
    saveStats();
    shutdownComplete = true;
}

FeedbackCollector::~FeedbackCollector()
{
    if (!shutdownComplete)
    {
        trackSessionEnd();
        saveStats();
    }
}

//==============================================================================
// Event tracking
//==============================================================================

void FeedbackCollector::trackEngineSwitch (int engineIndex, bool hq)
{
    switch (engineIndex)
    {
        case 0: stats.engineGreenCount++;  break;
        case 1: stats.engineBlueCount++;   break;
        case 2: stats.engineRedCount++;    break;
        case 3: stats.enginePurpleCount++; break;
        case 4: stats.engineBlackCount++;  break;
    }

    if (hq)
        stats.hqEnabledCount++;

    // Note: session log events for engine switches are now written by
    // PluginProcessor::parameterChanged(), which covers automation,
    // preset loads, and state restores — not just editor clicks.

    saveStats();
}

void FeedbackCollector::trackPresetLoad (int presetIndex, const juce::String& presetName)
{
    if (presetIndex >= 0 && presetIndex < 7)
    {
        stats.presetLoads[presetIndex]++;

        if (sessionLog != nullptr)
            sessionLog->log (SessionLog::EventType::PresetLoad, presetName);

        saveStats();
    }
}

void FeedbackCollector::trackSessionStart()
{
    sessionStart = juce::Time::getCurrentTime();
    stats.sessionCount++;
}

void FeedbackCollector::trackSessionEnd()
{
    if (sessionStart.toMilliseconds() > 0)
    {
        auto duration = juce::Time::getCurrentTime().toMilliseconds()
                      - sessionStart.toMilliseconds();
        stats.totalSessionTime += duration;
    }
}

//==============================================================================
// Host info
//==============================================================================

void FeedbackCollector::setHostInfo (const juce::String& host,
                                     const juce::String& wrapper,
                                     double sr, int bs)
{
    hostInfo.hostName    = host;
    hostInfo.wrapperType = wrapper;
    hostInfo.sampleRate  = sr;
    hostInfo.blockSize   = bs;
}

//==============================================================================
// SessionLog link
//==============================================================================

void FeedbackCollector::setSessionLog (SessionLog* log)
{
    sessionLog = log;
}

juce::String FeedbackCollector::getSessionLogSummary() const
{
    if (sessionLog == nullptr)
        return {};
    return sessionLog->getSessionSummary();
}

//==============================================================================
// Usage summary
//==============================================================================

juce::String FeedbackCollector::getUsageSummary() const
{
    juce::String s;
    s << "Choroboros Usage Summary\n";
    s << "============================\n";

    // Version
#ifdef CHOROBOROS_VERSION_STRING
    s << "Version: " << juce::String (CHOROBOROS_VERSION_STRING) << "\n";
#else
    s << "Version: 2.04-dev\n";
#endif
    s << "Date: " << juce::Time::getCurrentTime().toString (true, true) << "\n";

    // Platform
#if defined(__APPLE__) && defined(__arm64__)
    s << "Platform: macOS (Apple Silicon)\n";
#elif defined(__APPLE__)
    s << "Platform: macOS (Intel)\n";
#else
    s << "Platform: " << juce::SystemStats::getOperatingSystemName() << "\n";
#endif
    s << "Build: " << __DATE__ << " " << __TIME__ << "\n";

    // Host info (if set)
    if (hostInfo.hostName.isNotEmpty())
    {
        s << "Host: " << hostInfo.hostName;
        if (hostInfo.wrapperType.isNotEmpty())
            s << " (" << hostInfo.wrapperType << ")";
        s << "\n";
    }
    if (hostInfo.sampleRate > 0)
        s << "Sample Rate: " << juce::String (hostInfo.sampleRate, 0) << " Hz"
          << ", Buffer: " << hostInfo.blockSize << "\n";

    // CPU / RAM (concise)
    s << "CPU: " << juce::SystemStats::getCpuModel()
      << " (" << juce::SystemStats::getNumCpus() << " cores)\n";
    s << "RAM: " << (juce::SystemStats::getMemorySizeInMegabytes()) << " MB\n\n";

    // Engine usage
    s << "Engine Usage:\n";
    s << "  Green: " << stats.engineGreenCount  << ", "
      << "Blue: "    << stats.engineBlueCount   << ", "
      << "Red: "     << stats.engineRedCount    << ", "
      << "Purple: "  << stats.enginePurpleCount << ", "
      << "Black: "   << stats.engineBlackCount  << "\n";
    s << "  HQ toggles: " << stats.hqEnabledCount << "\n";

    // Preset usage
    const char* presetNames[] = { "Classic", "Vintage", "Modern", "Psychedelic",
                                  "Core", "Duck", "Ouroboros" };
    bool anyPresets = false;
    for (int i = 0; i < 7; i++)
    {
        if (stats.presetLoads[i] > 0)
        {
            if (! anyPresets) { s << "Presets: "; anyPresets = true; }
            else              { s << ", "; }
            s << presetNames[i] << "(" << stats.presetLoads[i] << ")";
        }
    }
    if (anyPresets) s << "\n";

    // Session stats
    s << "Sessions: " << stats.sessionCount;
    if (stats.sessionCount > 0 && stats.totalSessionTime > 0)
    {
        auto avgSec = (stats.totalSessionTime / stats.sessionCount) / 1000.0;
        s << " (avg " << juce::String (avgSec, 0) << "s)";
    }
    s << "\n";

    return s;
}

//==============================================================================
// File I/O
//==============================================================================

bool FeedbackCollector::saveFeedbackToFile (const juce::String& feedbackText) const
{
    auto feedbackDir = getFeedbackDirectory();
    if (! feedbackDir.createDirectory())
        return false;

    auto timestamp = juce::Time::getCurrentTime()
                         .toString (true, true)
                         .replaceCharacters (":", "-");
    auto feedbackFile = feedbackDir.getChildFile ("feedback_" + timestamp + ".txt");

    return feedbackFile.replaceWithText (feedbackText);
}

juce::File FeedbackCollector::getFeedbackDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Choroboros")
               .getChildFile ("Feedback");
}

//==============================================================================
// Data cleanup (called when user disables analytics consent)
//==============================================================================

void FeedbackCollector::clearPersistedAnalyticsData()
{
    // Delete the stats file and feedback directory
    auto statsFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                         .getChildFile ("Choroboros")
                         .getChildFile ("usage_stats.json");
    statsFile.deleteFile();

    auto feedbackDir = getFeedbackDirectory();
    feedbackDir.deleteRecursively();
}

//==============================================================================
// Stats persistence
//==============================================================================

void FeedbackCollector::loadStats()
{
    auto statsFile = getStatsFile();
    if (! statsFile.existsAsFile())
        return;

    juce::var json = juce::JSON::parse (statsFile);
    if (json.isObject())
    {
        auto obj = json.getDynamicObject();
        if (obj != nullptr)
        {
            stats.engineGreenCount  = obj->getProperty ("engineGreenCount");
            stats.engineBlueCount   = obj->getProperty ("engineBlueCount");
            stats.engineRedCount    = obj->getProperty ("engineRedCount");
            stats.enginePurpleCount = obj->getProperty ("enginePurpleCount");
            stats.engineBlackCount  = obj->getProperty ("engineBlackCount");
            stats.hqEnabledCount    = obj->getProperty ("hqEnabledCount");
            stats.sessionCount      = obj->getProperty ("sessionCount");
            stats.totalSessionTime  = obj->getProperty ("totalSessionTime");

            auto presetArray = obj->getProperty ("presetLoads");
            if (presetArray.isArray())
            {
                auto* arr = presetArray.getArray();
                for (int i = 0; i < juce::jmin (7, arr->size()); i++)
                    stats.presetLoads[i] = arr->getUnchecked (i);
            }
        }
    }
}

void FeedbackCollector::saveStats() const
{
    auto statsFile = getStatsFile();
    if (! statsFile.getParentDirectory().createDirectory())
        return;

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("engineGreenCount",  stats.engineGreenCount);
    obj->setProperty ("engineBlueCount",   stats.engineBlueCount);
    obj->setProperty ("engineRedCount",    stats.engineRedCount);
    obj->setProperty ("enginePurpleCount", stats.enginePurpleCount);
    obj->setProperty ("engineBlackCount",  stats.engineBlackCount);
    obj->setProperty ("hqEnabledCount",    stats.hqEnabledCount);
    obj->setProperty ("sessionCount",      stats.sessionCount);
    obj->setProperty ("totalSessionTime",  stats.totalSessionTime);

    juce::Array<juce::var> presetArray;
    for (int i = 0; i < 7; i++)
        presetArray.add (stats.presetLoads[i]);
    obj->setProperty ("presetLoads", juce::var (presetArray));

    statsFile.replaceWithText (juce::JSON::toString (juce::var (obj.get())));
}

juce::File FeedbackCollector::getStatsFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Choroboros")
               .getChildFile ("usage_stats.json");
}
