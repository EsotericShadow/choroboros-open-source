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

FeedbackCollector::FeedbackCollector()
{
    loadStats();
    trackSessionStart();
}

FeedbackCollector::~FeedbackCollector()
{
    trackSessionEnd();
    saveStats();
}

void FeedbackCollector::trackEngineSwitch(int engineIndex, bool hq)
{
    switch (engineIndex)
    {
        case 0: stats.engineGreenCount++; break;
        case 1: stats.engineBlueCount++; break;
        case 2: stats.engineRedCount++; break;
        case 3: stats.enginePurpleCount++; break;
        case 4: stats.engineBlackCount++; break;
    }
    
    if (hq)
        stats.hqEnabledCount++;
    
    saveStats();
}

void FeedbackCollector::trackPresetLoad(int presetIndex, const juce::String& presetName)
{
    if (presetIndex >= 0 && presetIndex < 7)
    {
        stats.presetLoads[presetIndex]++;
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
        auto sessionDuration = juce::Time::getCurrentTime().toMilliseconds() - sessionStart.toMilliseconds();
        stats.totalSessionTime += sessionDuration;
    }
}

juce::String FeedbackCollector::getUsageSummary() const
{
    juce::String summary;
    summary << "Choroboros Beta Usage Summary\n";
    summary << "============================\n\n";
#ifdef CHOROBOROS_VERSION_STRING
    summary << "Version: " << juce::String(CHOROBOROS_VERSION_STRING) << " (Beta)\n";
#else
    summary << "Version: 2.01 (Beta)\n";
#endif
    summary << "Date: " << juce::Time::getCurrentTime().toString(true, true) << "\n";
#if defined(__APPLE__) && defined(__arm64__)
    summary << "Platform: macOS (Apple Silicon)\n";
#elif defined(__APPLE__)
    summary << "Platform: macOS (Intel)\n";
#else
    summary << "Platform: " << juce::SystemStats::getOperatingSystemName() << "\n";
#endif
    summary << "Build: " << __DATE__ << " " << __TIME__ << "\n\n";
    
    summary << "Engine Usage:\n";
    summary << "  Green: " << stats.engineGreenCount << " switches\n";
    summary << "  Blue: " << stats.engineBlueCount << " switches\n";
    summary << "  Red: " << stats.engineRedCount << " switches\n";
    summary << "  Purple: " << stats.enginePurpleCount << " switches\n";
    summary << "  Black: " << stats.engineBlackCount << " switches\n";
    summary << "  HQ Enabled: " << stats.hqEnabledCount << " times\n\n";
    
    summary << "Preset Usage:\n";
    const char* presetNames[] = {"Classic (Green)", "Vintage (Red)", "Modern (Blue)", "Psychedelic (Purple)", "Core (Black)", "Duck", "Ouroboros"};
    for (int i = 0; i < 7; i++)
    {
        if (stats.presetLoads[i] > 0)
        {
            summary << "  " << presetNames[i] << ": " << stats.presetLoads[i] << " loads\n";
        }
    }
    summary << "\n";
    
    summary << "Sessions: " << stats.sessionCount << "\n";
    if (stats.sessionCount > 0)
    {
        auto avgSessionTime = stats.totalSessionTime / stats.sessionCount;
        summary << "Avg Session Time: " << (avgSessionTime / 1000.0) << " seconds\n";
    }
    
    return summary;
}

bool FeedbackCollector::saveFeedbackToFile(const juce::String& feedbackText) const
{
    auto feedbackDir = getFeedbackDirectory();
    if (!feedbackDir.createDirectory())
        return false;
    
    auto timestamp = juce::Time::getCurrentTime().toString(true, true).replaceCharacters(":", "-");
    auto feedbackFile = feedbackDir.getChildFile("feedback_" + timestamp + ".txt");
    
    juce::String fullFeedback;
    fullFeedback << getUsageSummary() << "\n";
    fullFeedback << "User Feedback:\n";
    fullFeedback << "==============\n";
    fullFeedback << feedbackText << "\n";
    
    return feedbackFile.replaceWithText(fullFeedback);
}

juce::File FeedbackCollector::getFeedbackDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("Feedback");
}

bool FeedbackCollector::isOptedOut()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Choroboros";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    
    juce::PropertiesFile props(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("Choroboros.settings"), options);
    
    return props.getBoolValue("feedbackOptedOut", false);
}

void FeedbackCollector::setOptedOut(bool optedOut)
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Choroboros";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    
    auto settingsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("Choroboros.settings");
    
    juce::PropertiesFile props(settingsFile, options);
    props.setValue("feedbackOptedOut", optedOut);
    props.save();
}

void FeedbackCollector::loadStats()
{
    auto statsFile = getStatsFile();
    if (!statsFile.existsAsFile())
        return;
    
    juce::var json = juce::JSON::parse(statsFile);
    if (json.isObject())
    {
        auto obj = json.getDynamicObject();
        if (obj != nullptr)
        {
            stats.engineGreenCount = obj->getProperty("engineGreenCount");
            stats.engineBlueCount = obj->getProperty("engineBlueCount");
            stats.engineRedCount = obj->getProperty("engineRedCount");
            stats.enginePurpleCount = obj->getProperty("enginePurpleCount");
            stats.engineBlackCount = obj->getProperty("engineBlackCount");
            stats.hqEnabledCount = obj->getProperty("hqEnabledCount");
            stats.sessionCount = obj->getProperty("sessionCount");
            stats.totalSessionTime = obj->getProperty("totalSessionTime");
            
            auto presetArray = obj->getProperty("presetLoads");
            if (presetArray.isArray())
            {
                auto* arr = presetArray.getArray();
                for (int i = 0; i < juce::jmin(7, arr->size()); i++)
                {
                    stats.presetLoads[i] = arr->getUnchecked(i);
                }
            }
        }
    }
}

void FeedbackCollector::saveStats() const
{
    auto statsFile = getStatsFile();
    if (!statsFile.getParentDirectory().createDirectory())
        return;
    
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("engineGreenCount", stats.engineGreenCount);
    obj->setProperty("engineBlueCount", stats.engineBlueCount);
    obj->setProperty("engineRedCount", stats.engineRedCount);
    obj->setProperty("enginePurpleCount", stats.enginePurpleCount);
    obj->setProperty("engineBlackCount", stats.engineBlackCount);
    obj->setProperty("hqEnabledCount", stats.hqEnabledCount);
    obj->setProperty("sessionCount", stats.sessionCount);
    obj->setProperty("totalSessionTime", stats.totalSessionTime);
    
    juce::Array<juce::var> presetArray;
    for (int i = 0; i < 7; i++)
        presetArray.add(stats.presetLoads[i]);
    obj->setProperty("presetLoads", juce::var(presetArray));
    
    statsFile.replaceWithText(juce::JSON::toString(juce::var(obj.get())));
}


juce::File FeedbackCollector::getStatsFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("usage_stats.json");
}
