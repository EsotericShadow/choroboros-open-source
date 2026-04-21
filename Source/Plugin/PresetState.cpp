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

#include "PresetState.h"
#include "PluginProcessor.h"
#include "../Config/FactoryDefaults.h"
#include <cmath>

//==============================================================================
// Validation

bool PresetState::isValid() const
{
    // Check that all parameters are finite
    if (!std::isfinite(rate) || !std::isfinite(depth) || !std::isfinite(offset)
        || !std::isfinite(width) || !std::isfinite(color) || !std::isfinite(mix))
        return false;

    // Check parameter ranges
    if (rate < ChoroborosAudioProcessor::RATE_MIN || rate > ChoroborosAudioProcessor::RATE_MAX)
        return false;
    if (depth < ChoroborosAudioProcessor::DEPTH_MIN || depth > ChoroborosAudioProcessor::DEPTH_MAX)
        return false;
    if (offset < ChoroborosAudioProcessor::OFFSET_MIN || offset > ChoroborosAudioProcessor::OFFSET_MAX)
        return false;
    if (width < ChoroborosAudioProcessor::WIDTH_MIN || width > ChoroborosAudioProcessor::WIDTH_MAX)
        return false;
    if (color < ChoroborosAudioProcessor::COLOR_MIN || color > ChoroborosAudioProcessor::COLOR_MAX)
        return false;
    if (mix < ChoroborosAudioProcessor::MIX_MIN || mix > ChoroborosAudioProcessor::MIX_MAX)
        return false;
    if (engineColorIndex < 0 || engineColorIndex > 4)
        return false;

    // Version must be positive
    if (version < 0)
        return false;

    return true;
}

//==============================================================================
// JSON Serialization

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

    juce::var buildCoreAssignmentsVar(const choroboros::CoreAssignmentTable& table)
    {
        auto* root = new juce::DynamicObject();
        for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
        {
            auto* engineNode = new juce::DynamicObject();
            engineNode->setProperty("nq", juce::String(choroboros::coreIdToToken(table.get(engine, false))));
            engineNode->setProperty("hq", juce::String(choroboros::coreIdToToken(table.get(engine, true))));
            root->setProperty(juce::String(choroboros::kEngineColorTokens[static_cast<std::size_t>(engine)]), juce::var(engineNode));
        }
        return juce::var(root);
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
}

std::string PresetState::serializeToJson() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", version);
    root->setProperty("rate", rate);
    root->setProperty("depth", depth);
    root->setProperty("offset", offset);
    root->setProperty("width", width);
    root->setProperty("color", color);
    root->setProperty("mix", mix);
    root->setProperty("hqEnabled", hqEnabled);
    root->setProperty("engineColorIndex", engineColorIndex);
    if (!customEngineId.empty())
        root->setProperty("customEngineId", juce::String(customEngineId));
    root->setProperty("modularCoresEnabled", modularCoresEnabled);
    root->setProperty("coreAssignments", buildCoreAssignmentsVar(coreAssignments));

    const juce::var rootVar(root);
    return juce::JSON::toString(rootVar, false).toStdString();
}

std::optional<PresetState> PresetState::deserializeFromJson(const std::string& json)
{
    const auto parsed = juce::JSON::parse(juce::String(json));
    if (parsed.isVoid())
        return std::nullopt;

    const auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return std::nullopt;

    PresetState state;

    // Version (required)
    state.version = static_cast<int>(getNumberOrDefault(parsed, "version", 1));

    // Sound parameters
    state.rate = static_cast<float>(getNumberOrDefault(parsed, "rate", 1.0));
    state.depth = static_cast<float>(getNumberOrDefault(parsed, "depth", 0.5));
    state.offset = static_cast<float>(getNumberOrDefault(parsed, "offset", 90.0));
    state.width = static_cast<float>(getNumberOrDefault(parsed, "width", 1.0));
    state.color = static_cast<float>(getNumberOrDefault(parsed, "color", 0.5));
    state.mix = static_cast<float>(getNumberOrDefault(parsed, "mix", 0.5));

    // Quality
    const auto hqVar = obj->getProperty("hqEnabled");
    state.hqEnabled = (hqVar.isBool() && static_cast<bool>(hqVar))
                   || (hqVar.isDouble() && static_cast<double>(hqVar) >= 0.5)
                   || (hqVar.isInt() && static_cast<int>(hqVar) != 0)
                   || (hqVar.toString().equalsIgnoreCase("true"));
    state.engineColorIndex = juce::jlimit(0, 4,
        static_cast<int>(getNumberOrDefault(parsed, "engineColorIndex", 0)));
    const auto customIdVar = obj->getProperty("customEngineId");
    if (customIdVar.isString())
        state.customEngineId = customIdVar.toString().toStdString();

    // Modular core routing
    const auto modularVar = obj->getProperty("modularCoresEnabled");
    state.modularCoresEnabled = (modularVar.isBool() && static_cast<bool>(modularVar))
                             || (modularVar.isDouble() && static_cast<double>(modularVar) >= 0.5)
                             || (modularVar.isInt() && static_cast<int>(modularVar) != 0)
                             || (modularVar.toString().equalsIgnoreCase("true"));

    // Core assignments
    state.coreAssignments.resetToLegacy();
    const auto coreVar = obj->getProperty("coreAssignments");
    parseCoreAssignmentsFromVar(coreVar, state.coreAssignments);

    // Validate loaded state
    if (!state.isValid())
        return std::nullopt;

    return state;
}

//==============================================================================
// Binary Serialization (wrapper around JSON)

juce::MemoryBlock PresetState::serializeToBinary() const
{
    const auto json = serializeToJson();
    juce::MemoryBlock block(json.data(), json.size());
    return block;
}

std::optional<PresetState> PresetState::deserializeFromBinary(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return std::nullopt;

    // Try to parse as canonical JSON format first
    std::string jsonStr(static_cast<const char*>(data), sizeInBytes);
    if (auto state = deserializeFromJson(jsonStr))
        return state;

    // Fall back to legacy raw APVTS XML format
    std::unique_ptr<juce::XmlElement> xmlState(
        juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr || xmlState->getTagName() != "VALUETREE")
        return std::nullopt;

    // Migrate from legacy APVTS XML to canonical PresetState
    PresetState state;
    state.version = 1;  // Mark as migrated

    // Extract parameters from APVTS ValueTree
    auto readParameter = [&xmlState](const char* paramId, float fallback) -> float
    {
        const auto paramElem = xmlState->getChildByAttribute("id", paramId);
        if (paramElem == nullptr)
            return fallback;

        const auto valueStr = paramElem->getStringAttribute("value", "");
        if (valueStr.isEmpty())
            return fallback;

        return std::stof(valueStr.toStdString());
    };

    state.rate = readParameter(ChoroborosAudioProcessor::RATE_ID, 1.0f);
    state.depth = readParameter(ChoroborosAudioProcessor::DEPTH_ID, 0.5f);
    state.offset = readParameter(ChoroborosAudioProcessor::OFFSET_ID, 90.0f);
    state.width = readParameter(ChoroborosAudioProcessor::WIDTH_ID, 1.0f);
    state.color = readParameter(ChoroborosAudioProcessor::COLOR_ID, 0.5f);
    state.mix = readParameter(ChoroborosAudioProcessor::MIX_ID, 0.5f);

    const auto hqValue = readParameter(ChoroborosAudioProcessor::HQ_ID, 0.0f);
    state.hqEnabled = (hqValue >= 0.5f);

    // Engine color (legacy APVTS stores this as a float choice index)
    const float engineColorValue = readParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID, 0.0f);
    state.engineColorIndex = juce::jlimit(0, 4, static_cast<int>(std::round(engineColorValue)));

    // Custom engine ID (stored as a ValueTree property on the root element)
    if (xmlState->hasAttribute("customEngineId"))
        state.customEngineId = xmlState->getStringAttribute("customEngineId").toStdString();

    // Legacy core assignments (if present)
    state.modularCoresEnabled = false;  // Default: modular mode was off in legacy
    state.coreAssignments.resetToLegacy();

    // Try to read modularCoresEnabled from XML
    const auto modularElem = xmlState->getChildByAttribute("id", "modularCoresEnabled");
    if (modularElem != nullptr)
    {
        const auto valueStr = modularElem->getStringAttribute("value", "");
        if (!valueStr.isEmpty())
        {
            state.modularCoresEnabled = (valueStr == "1" || valueStr.equalsIgnoreCase("true"));
        }
    }

    // Try to read coreAssignmentsJson from XML
    const auto assignmentsElem = xmlState->getChildByAttribute("id", "coreAssignmentsJson");
    if (assignmentsElem != nullptr)
    {
        const auto valueStr = assignmentsElem->getStringAttribute("value", "");
        if (!valueStr.isEmpty())
        {
            const auto parsed = juce::JSON::parse(valueStr);
            parseCoreAssignmentsFromVar(parsed, state.coreAssignments);
        }
    }

    // Validate the migrated state
    if (!state.isValid())
        return std::nullopt;

    return state;
}

//==============================================================================
// File I/O

bool PresetState::saveToFile(const juce::File& file) const
{
    if (!isValid())
        return false;

    const auto json = serializeToJson();
    return file.replaceWithText(juce::String(json), false, false, "utf-8");
}

std::optional<PresetState> PresetState::loadFromFile(const juce::File& file)
{
    if (!file.exists())
        return std::nullopt;

    const auto content = file.loadFileAsString();

    // Try canonical JSON format first
    if (auto state = deserializeFromJson(content.toStdString()))
        return state;

    // Fall back to legacy XML format
    if (auto xml = juce::XmlDocument::parse(file))
    {
        // Wrap XML and deserialize as binary
        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary(*xml, block);
        return deserializeFromBinary(block.getData(), static_cast<int>(block.getSize()));
    }

    return std::nullopt;
}

//==============================================================================
// Factory Presets

std::optional<PresetState> PresetState::makeFactoryPreset(int index)
{
    const auto preset = choroboros::factory::getPreset(index);
    if (!preset.has_value())
        return std::nullopt;

    PresetState state;
    state.version = 1;
    state.rate = preset->rate;
    state.depth = preset->depth;
    state.offset = preset->offset;
    state.width = preset->width;
    state.color = preset->color;
    state.mix = preset->mix;
    state.hqEnabled = preset->hqEnabled;
    state.engineColorIndex = preset->engineColorIndex;
    state.modularCoresEnabled = preset->modularCoresEnabled;
    state.coreAssignments = preset->coreAssignments;

    if (!state.isValid())
        return std::nullopt;

    return state;
}
