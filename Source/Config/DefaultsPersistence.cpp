/*
 * Choroboros - Defaults persistence service
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#include "DefaultsPersistence.h"

namespace
{
juce::File getConfigDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Choroboros");
}

juce::File getLegacyDefaultsFilePath()
{
    return getConfigDirectory().getChildFile("defaults.json");
}

bool isValidJsonDocument(const juce::String& json)
{
    return !json.isEmpty() && !juce::JSON::parse(json).isVoid();
}

bool writeJsonDocument(const juce::File& targetFile,
                       const juce::String& json,
                       juce::String* outError,
                       bool writeRecoveryOnFailure)
{
    const auto recovery = DefaultsPersistence::getRecoveryFile(false);
    const auto recoveryTs = DefaultsPersistence::getRecoveryFile(true);

    auto writeRecovery = [&](const juce::String& reason) -> bool
    {
        const bool ok = recovery.replaceWithText(json);
        if (!ok)
            recoveryTs.replaceWithText(json);
        DefaultsPersistence::logFailure(reason + (ok ? "" : " | WARNING: recovery write failed"));
        return ok || recoveryTs.existsAsFile();
    };

    if (!targetFile.getParentDirectory().createDirectory())
    {
        const juce::String err = "Could not create Choroboros config directory";
        if (outError != nullptr)
            *outError = err;
        if (writeRecoveryOnFailure)
            writeRecovery(err);
        else
            DefaultsPersistence::logFailure(err);
        return false;
    }

    if (!isValidJsonDocument(json))
    {
        const juce::String err = "Invalid JSON";
        if (outError != nullptr)
            *outError = err;
        if (writeRecoveryOnFailure)
            writeRecovery(err);
        else
            DefaultsPersistence::logFailure(err);
        return false;
    }

    if (!targetFile.replaceWithText(json))
    {
        const juce::String err = targetFile.getFileName() + " write failed";
        if (outError != nullptr)
            *outError = err;
        if (writeRecoveryOnFailure)
            writeRecovery(err);
        else
            DefaultsPersistence::logFailure(err);
        return false;
    }

    return true;
}
} // namespace

juce::File DefaultsPersistence::getUserDefaultsFile()
{
    return getConfigDirectory().getChildFile("defaults_user.json");
}

juce::File DefaultsPersistence::getFactoryDefaultsFile()
{
    return getConfigDirectory().getChildFile("defaults_factory.json");
}

juce::File DefaultsPersistence::getDefaultsFile()
{
    return getUserDefaultsFile();
}

juce::File DefaultsPersistence::getRecoveryFile(bool timestamped)
{
    auto dir = getConfigDirectory();
    if (!timestamped)
        return dir.getChildFile("defaults_user_recovery_last_attempt.json");
    const auto millis = juce::Time::getCurrentTime().toMilliseconds();
    return dir.getChildFile("defaults_user_recovery_failed_" + juce::String(millis) + ".json");
}

juce::File DefaultsPersistence::getFailureLogFile()
{
    return getConfigDirectory().getChildFile("defaults_save_failures.log");
}

bool DefaultsPersistence::saveUser(const juce::String& json, juce::String* outError)
{
    return writeJsonDocument(getUserDefaultsFile(), json, outError, true);
}

juce::String DefaultsPersistence::loadUser(juce::String* outError)
{
    const auto userFile = getUserDefaultsFile();
    if (userFile.existsAsFile())
    {
        auto content = userFile.loadFileAsString();
        if (content.isEmpty() && userFile.getSize() > 0)
        {
            if (outError != nullptr)
                *outError = "Failed to read user defaults file";
            return {};
        }
        return content;
    }

    // One-way migration path from legacy defaults.json.
    const auto legacyFile = getLegacyDefaultsFilePath();
    if (legacyFile.existsAsFile())
    {
        auto legacyContent = legacyFile.loadFileAsString();
        if (!legacyContent.isEmpty() && isValidJsonDocument(legacyContent))
        {
            juce::String saveErr;
            saveUser(legacyContent, &saveErr);
            return legacyContent;
        }
    }

    if (outError != nullptr)
        *outError = "User defaults file does not exist";
    return {};
}

bool DefaultsPersistence::saveFactory(const juce::String& json, juce::String* outError)
{
    return writeJsonDocument(getFactoryDefaultsFile(), json, outError, false);
}

juce::String DefaultsPersistence::loadFactory(juce::String* outError)
{
    const auto file = getFactoryDefaultsFile();
    if (!file.existsAsFile())
    {
        if (outError != nullptr)
            *outError = "Factory defaults file does not exist";
        return {};
    }

    auto content = file.loadFileAsString();
    if (content.isEmpty() && file.getSize() > 0)
    {
        if (outError != nullptr)
            *outError = "Failed to read factory defaults file";
        return {};
    }
    return content;
}

bool DefaultsPersistence::resetUserToFactory(juce::String* outError)
{
    juce::String factoryErr;
    const auto factoryJson = loadFactory(&factoryErr);
    if (factoryJson.isEmpty())
    {
        if (outError != nullptr)
            *outError = factoryErr.isNotEmpty() ? factoryErr : "Factory defaults unavailable";
        return false;
    }
    return saveUser(factoryJson, outError);
}

bool DefaultsPersistence::save(const juce::String& json, juce::String* outError)
{
    return saveUser(json, outError);
}

juce::String DefaultsPersistence::load(juce::String* outError)
{
    return loadUser(outError);
}

void DefaultsPersistence::logFailure(const juce::String& reason)
{
    const auto now = juce::Time::getCurrentTime().toString(true, true, true, true);
    const auto file = getUserDefaultsFile();
    const auto recovery = getRecoveryFile(false);
    const auto recoveryTs = getRecoveryFile(true);
    const juce::String line = "[" + now + "] " + reason
        + " | userDefaults=" + file.getFullPathName()
        + " | recovery=" + recovery.getFullPathName()
        + " | recoveryTimestamped=" + recoveryTs.getFullPathName() + "\n";

    juce::Logger::writeToLog("DefaultsPersistence: " + line.trimEnd());

    auto logFile = getFailureLogFile();
    logFile.getParentDirectory().createDirectory();
    logFile.appendText(line, false, false, nullptr);
}
