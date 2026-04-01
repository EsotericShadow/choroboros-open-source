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

#include "ConsentService.h"
#include "FeedbackCollector.h"

ConsentService::ConsentService()
{
    loadFromPropertiesFile();
}

void ConsentService::setAnalyticsEnabled(bool enabled)
{
    if (analyticsEnabled_ == enabled)
        return;  // No change

    analyticsEnabled_ = enabled;
    saveToPropertiesFile();

    // On disable, clear existing analytics data
    if (!enabled)
    {
        FeedbackCollector::clearPersistedAnalyticsData();
    }
}

void ConsentService::setDiagnosticsEnabled(bool enabled)
{
    diagnosticsEnabled_ = enabled;
    saveToPropertiesFile();
}

void ConsentService::loadFromPropertiesFile()
{
    auto settingsFile = getSettingsFile();

    juce::PropertiesFile::Options options;
    options.applicationName     = "Choroboros";
    options.filenameSuffix      = "settings";
    options.osxLibrarySubFolder = "Application Support";

    juce::PropertiesFile props (settingsFile, options);

    // Read from properties with explicit defaults (OFF)
    analyticsEnabled_    = props.getBoolValue ("analyticsEnabled", false);
    diagnosticsEnabled_  = props.getBoolValue ("diagnosticsEnabled", false);
}

void ConsentService::saveToPropertiesFile()
{
    auto settingsFile = getSettingsFile();

    juce::PropertiesFile::Options options;
    options.applicationName     = "Choroboros";
    options.filenameSuffix      = "settings";
    options.osxLibrarySubFolder = "Application Support";

    // Ensure parent directory exists
    settingsFile.getParentDirectory().createDirectory();

    juce::PropertiesFile props (settingsFile, options);
    props.setValue ("analyticsEnabled", analyticsEnabled_);
    props.setValue ("diagnosticsEnabled", diagnosticsEnabled_);
    props.save();
}

juce::File ConsentService::getSettingsFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Choroboros")
               .getChildFile ("Choroboros.settings");
}
