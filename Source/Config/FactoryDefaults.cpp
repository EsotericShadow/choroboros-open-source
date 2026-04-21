#include "FactoryDefaults.h"
#include "DefaultsPersistence.h"

namespace choroboros::factory
{
namespace
{
double getNumberOrDefault(const juce::var& objectVar, const juce::Identifier& key, double fallback)
{
    if (const auto* object = objectVar.getDynamicObject())
    {
        const auto value = object->getProperty(key);
        if (value.isDouble() || value.isInt() || value.isInt64())
            return static_cast<double>(value);
    }
    return fallback;
}

bool getBoolOrDefault(const juce::var& objectVar, const juce::Identifier& key, bool fallback)
{
    if (const auto* object = objectVar.getDynamicObject())
    {
        const auto value = object->getProperty(key);
        if (value.isBool())
            return static_cast<bool>(value);
        if (value.isDouble() || value.isInt() || value.isInt64())
            return static_cast<double>(value) >= 0.5;
        const auto text = value.toString().trim();
        if (text.equalsIgnoreCase("true") || text == "1")
            return true;
        if (text.equalsIgnoreCase("false") || text == "0")
            return false;
    }
    return fallback;
}

const char* const kEngineKeys[] = { "green", "blue", "red", "purple", "black" };

std::optional<juce::var> loadFactoryRoot()
{
    juce::String error;
    const auto json = DefaultsPersistence::loadFactory(&error);
    if (json.isEmpty())
        return std::nullopt;

    const auto parsed = juce::JSON::parse(json);
    if (parsed.isVoid() || parsed.getDynamicObject() == nullptr)
        return std::nullopt;

    return parsed;
}

bool parseCoreAssignmentsFromVar(const juce::var& assignmentsVar, choroboros::CoreAssignmentTable& outTable)
{
    outTable.resetToLegacy();
    const auto* root = assignmentsVar.getDynamicObject();
    if (root == nullptr)
        return false;

    bool anyLoaded = false;
    for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
    {
        const juce::String engineToken(choroboros::kEngineColorTokens[static_cast<std::size_t>(engine)]);
        const juce::var engineVar = root->getProperty(engineToken);
        const auto* engineObj = engineVar.getDynamicObject();

        auto parseMode = [&](bool hqEnabled) -> bool
        {
            juce::String tokenText;
            if (engineObj != nullptr)
            {
                const juce::Identifier key(hqEnabled ? "hq" : "nq");
                tokenText = engineObj->getProperty(key).toString().trim();
            }
            if (tokenText.isEmpty())
                tokenText = root->getProperty(engineToken + "_" + (hqEnabled ? "hq" : "nq")).toString().trim();
            if (tokenText.isEmpty())
                return false;

            choroboros::CoreId parsed = choroboros::CoreId::lagrange3;
            if (!choroboros::parseCoreIdToken(tokenText.toStdString(), parsed))
                return false;

            outTable.set(engine, hqEnabled, parsed);
            return true;
        };

        anyLoaded = parseMode(false) || anyLoaded;
        anyLoaded = parseMode(true) || anyLoaded;
    }

    return anyLoaded;
}
} // namespace

std::optional<EngineProfile> getEngineProfile(int engineIndex)
{
    if (engineIndex < 0 || engineIndex >= static_cast<int>(std::size(kEngineKeys)))
        return std::nullopt;

    const auto rootVar = loadFactoryRoot();
    if (!rootVar.has_value())
        return std::nullopt;

    const auto* root = rootVar->getDynamicObject();
    if (root == nullptr || !root->hasProperty("engineParamProfiles"))
        return std::nullopt;

    const auto profilesVar = root->getProperty("engineParamProfiles");
    const auto* profilesObj = profilesVar.getDynamicObject();
    if (profilesObj == nullptr)
        return std::nullopt;

    const auto profileVar = profilesObj->getProperty(kEngineKeys[engineIndex]);
    const auto* profileObj = profileVar.getDynamicObject();
    if (profileObj == nullptr)
        return std::nullopt;

    EngineProfile profile;
    profile.valid = getBoolOrDefault(profileVar, "valid", true);
    profile.rate = static_cast<float>(getNumberOrDefault(profileVar, "rate", profile.rate));
    profile.depth = static_cast<float>(getNumberOrDefault(profileVar, "depth", profile.depth));
    profile.offset = static_cast<float>(getNumberOrDefault(profileVar, "offset", profile.offset));
    profile.width = static_cast<float>(getNumberOrDefault(profileVar, "width", profile.width));
    profile.mix = static_cast<float>(getNumberOrDefault(profileVar, "mix", profile.mix));
    profile.color = static_cast<float>(getNumberOrDefault(profileVar, "color", profile.color));
    return profile;
}

int getPresetCount()
{
    const auto rootVar = loadFactoryRoot();
    if (!rootVar.has_value())
        return 0;

    const auto* root = rootVar->getDynamicObject();
    if (root == nullptr || !root->hasProperty("factoryPresets"))
        return 0;

    if (const auto* array = root->getProperty("factoryPresets").getArray())
        return static_cast<int>(array->size());

    return 0;
}

juce::String getPresetName(int index)
{
    const auto preset = getPreset(index);
    return preset.has_value() ? preset->name : juce::String{};
}

std::optional<PresetDefinition> getPreset(int index)
{
    if (index < 0)
        return std::nullopt;

    const auto rootVar = loadFactoryRoot();
    if (!rootVar.has_value())
        return std::nullopt;

    const auto* root = rootVar->getDynamicObject();
    if (root == nullptr || !root->hasProperty("factoryPresets"))
        return std::nullopt;

    const auto presetsVar = root->getProperty("factoryPresets");
    const auto* presetsArray = presetsVar.getArray();
    if (presetsArray == nullptr || index >= static_cast<int>(presetsArray->size()))
        return std::nullopt;

    const auto presetVar = presetsArray->getReference(index);
    const auto* presetObj = presetVar.getDynamicObject();
    if (presetObj == nullptr)
        return std::nullopt;

    PresetDefinition preset;
    preset.name = presetObj->getProperty("name").toString();
    preset.rate = static_cast<float>(getNumberOrDefault(presetVar, "rate", preset.rate));
    preset.depth = static_cast<float>(getNumberOrDefault(presetVar, "depth", preset.depth));
    preset.offset = static_cast<float>(getNumberOrDefault(presetVar, "offset", preset.offset));
    preset.width = static_cast<float>(getNumberOrDefault(presetVar, "width", preset.width));
    preset.color = static_cast<float>(getNumberOrDefault(presetVar, "color", preset.color));
    preset.mix = static_cast<float>(getNumberOrDefault(presetVar, "mix", preset.mix));
    preset.hqEnabled = getBoolOrDefault(presetVar, "hqEnabled", preset.hqEnabled);
    preset.engineColorIndex = juce::jlimit(0, 4, static_cast<int>(getNumberOrDefault(presetVar, "engineColorIndex", preset.engineColorIndex)));

    preset.modularCoresEnabled = getBoolOrDefault(presetVar, "modularCoresEnabled",
                                                  getBoolOrDefault(*rootVar, "modularCoresEnabled", false));

    preset.coreAssignments.resetToLegacy();
    if (!parseCoreAssignmentsFromVar(root->getProperty("coreAssignments"), preset.coreAssignments))
        preset.coreAssignments.resetToLegacy();
    if (presetObj->hasProperty("coreAssignments"))
    {
        choroboros::CoreAssignmentTable presetAssignments;
        if (parseCoreAssignmentsFromVar(presetObj->getProperty("coreAssignments"), presetAssignments))
            preset.coreAssignments = presetAssignments;
    }

    return preset;
}
} // namespace choroboros::factory
