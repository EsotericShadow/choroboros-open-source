/*
 * Choroboros - Defaults persistence service
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#include "DefaultsPersistence.h"

juce::File DefaultsPersistence::getDefaultsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("defaults.json");
}

juce::File DefaultsPersistence::getRecoveryFile(bool timestamped)
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Choroboros");
    if (!timestamped)
        return dir.getChildFile("defaults_recovery_last_attempt.json");
    const auto millis = juce::Time::getCurrentTime().toMilliseconds();
    return dir.getChildFile("defaults_recovery_failed_" + juce::String(millis) + ".json");
}

juce::File DefaultsPersistence::getFailureLogFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros")
        .getChildFile("defaults_save_failures.log");
}

bool DefaultsPersistence::save(const juce::String& json, juce::String* outError)
{
    const auto file = getDefaultsFile();
    const auto recovery = getRecoveryFile(false);
    const auto recoveryTs = getRecoveryFile(true);

    auto writeRecovery = [&](const juce::String& reason) -> bool
    {
        const bool ok = recovery.replaceWithText(json);
        if (!ok)
            recoveryTs.replaceWithText(json);
        logFailure(reason + (ok ? "" : " | WARNING: recovery write failed"));
        return ok || recoveryTs.existsAsFile();
    };

    if (!file.getParentDirectory().createDirectory())
    {
        const juce::String err = "Could not create Choroboros config directory";
        if (outError) *outError = err;
        writeRecovery(err);
        return false;
    }

    if (juce::JSON::parse(json).isVoid())
    {
        const juce::String err = "Invalid JSON";
        if (outError) *outError = err;
        writeRecovery(err);
        return false;
    }

    if (!file.replaceWithText(json))
    {
        const juce::String err = "defaults.json write failed";
        if (outError) *outError = err;
        writeRecovery(err);
        return false;
    }

    return true;
}

juce::String DefaultsPersistence::load(juce::String* outError)
{
    const auto file = getDefaultsFile();
    if (!file.existsAsFile())
    {
        if (outError) *outError = "Defaults file does not exist";
        return {};
    }
    auto content = file.loadFileAsString();
    if (content.isEmpty() && file.getSize() > 0)
    {
        if (outError) *outError = "Failed to read defaults file";
        return {};
    }
    return content;
}

void DefaultsPersistence::logFailure(const juce::String& reason)
{
    const auto now = juce::Time::getCurrentTime().toString(true, true, true, true);
    const auto file = getDefaultsFile();
    const auto recovery = getRecoveryFile(false);
    const auto recoveryTs = getRecoveryFile(true);
    const juce::String line = "[" + now + "] " + reason
        + " | defaults=" + file.getFullPathName()
        + " | recovery=" + recovery.getFullPathName()
        + " | recoveryTimestamped=" + recoveryTs.getFullPathName() + "\n";

    juce::Logger::writeToLog("DefaultsPersistence: " + line.trimEnd());

    auto logFile = getFailureLogFile();
    logFile.getParentDirectory().createDirectory();
    logFile.appendText(line, false, false, nullptr);
}
