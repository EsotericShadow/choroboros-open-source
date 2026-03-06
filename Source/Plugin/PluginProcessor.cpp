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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../DSP/ChorusDSP.h"
#include "../Config/DefaultsPersistence.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

namespace BinaryData
{
const char* getNamedResource(const char* resourceNameUTF8, int& dataSizeInBytes);
}

namespace
{
class MetaEngineChoiceParameter final : public juce::AudioParameterChoice
{
public:
    MetaEngineChoiceParameter(const juce::ParameterID& parameterID,
                              const juce::String& parameterName,
                              const juce::StringArray& choices,
                              int defaultItemIndex)
        : juce::AudioParameterChoice(parameterID, parameterName, choices, defaultItemIndex)
    {
    }

    bool isMetaParameter() const override
    {
        return true;
    }
};

std::atomic<std::uint64_t>& getLoadTraceInstanceCounter()
{
    static std::atomic<std::uint64_t> counter { 1 };
    return counter;
}

std::uint64_t nextLoadTraceInstanceId()
{
    return getLoadTraceInstanceCounter().fetch_add(1, std::memory_order_relaxed);
}

juce::CriticalSection& getLoadTraceFileLock()
{
    static juce::CriticalSection lock;
    return lock;
}

juce::String getSafeHostDescription()
{
    const juce::PluginHostType hostType;
    const juce::String hostDescription(hostType.getHostDescription());
    return hostDescription.isNotEmpty() ? hostDescription : juce::String("Unknown");
}

void appendLoadTraceLine(const ChoroborosAudioProcessor& processor,
                         const juce::String& eventName,
                         double elapsedMs,
                         const juce::String& notes)
{
    if (eventName.isEmpty())
        return;

    const juce::ScopedLock lock(getLoadTraceFileLock());
    const auto logFile = ChoroborosAudioProcessor::getLoadTraceLogFile();
    if (!logFile.getParentDirectory().createDirectory())
        return;

    juce::var payload(new juce::DynamicObject());
    auto* object = payload.getDynamicObject();
    if (object == nullptr)
        return;

    const auto wrapperType = juce::PluginHostType::getPluginLoadedAs();
    object->setProperty("tsUtc", juce::Time::getCurrentTime().toISO8601(true));
    object->setProperty("event", eventName);
    object->setProperty("elapsedMs", juce::roundToInt(elapsedMs * 1000.0) / 1000.0);
    object->setProperty("instanceId", static_cast<juce::int64>(processor.getInstanceId()));
    object->setProperty("host", getSafeHostDescription());
    object->setProperty("hostPath", juce::PluginHostType::getHostPath());
    object->setProperty("wrapperType", juce::String(juce::AudioProcessor::getWrapperTypeDescription(wrapperType)));
    object->setProperty("pluginVersion", juce::String(JucePlugin_VersionString));
   #if JUCE_DEBUG
    object->setProperty("buildConfig", "debug");
   #else
    object->setProperty("buildConfig", "release");
   #endif
    object->setProperty("os", juce::SystemStats::getOperatingSystemName());
    object->setProperty("isOS64Bit", juce::SystemStats::isOperatingSystem64Bit());
    object->setProperty("cpuVendor", juce::SystemStats::getCpuVendor());
    object->setProperty("cpuModel", juce::SystemStats::getCpuModel());
    object->setProperty("cpuSpeedMHz", juce::SystemStats::getCpuSpeedInMegahertz());
    object->setProperty("cpuCores", juce::SystemStats::getNumCpus());
    object->setProperty("ramMB", juce::SystemStats::getMemorySizeInMegabytes());
    if (notes.isNotEmpty())
        object->setProperty("notes", notes);

    logFile.appendText(juce::JSON::toString(payload, false) + "\n", false, false, nullptr);
}

juce::String buildStartupParamSnapshotNotes(ChoroborosAudioProcessor& processor, const juce::String& sourceTag)
{
    auto& valueTreeState = processor.getValueTreeState();
    const auto readRaw = [&](const char* paramId, float fallback) -> float
    {
        if (const auto* raw = valueTreeState.getRawParameterValue(paramId))
            return raw->load();
        return fallback;
    };

    const int engine = juce::jlimit(0, 4,
                                    static_cast<int>(std::round(readRaw(ChoroborosAudioProcessor::ENGINE_COLOR_ID, 0.0f))));
    const int hq = readRaw(ChoroborosAudioProcessor::HQ_ID, 0.0f) >= 0.5f ? 1 : 0;
    const float rate = processor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID,
                                                   readRaw(ChoroborosAudioProcessor::RATE_ID, 0.5f));
    const float depth = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID,
                                                    readRaw(ChoroborosAudioProcessor::DEPTH_ID, 0.5f));
    const float offset = processor.mapParameterValue(ChoroborosAudioProcessor::OFFSET_ID,
                                                     readRaw(ChoroborosAudioProcessor::OFFSET_ID, 90.0f));
    const float width = processor.mapParameterValue(ChoroborosAudioProcessor::WIDTH_ID,
                                                    readRaw(ChoroborosAudioProcessor::WIDTH_ID, 1.0f));
    const float color = processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                    readRaw(ChoroborosAudioProcessor::COLOR_ID, 0.5f));
    const float mix = processor.mapParameterValue(ChoroborosAudioProcessor::MIX_ID,
                                                  readRaw(ChoroborosAudioProcessor::MIX_ID, 0.5f));

    return juce::String::formatted("%s,engine=%d,hq=%d,rate=%.4f,depth=%.4f,offset=%.4f,width=%.4f,color=%.4f,mix=%.4f",
                                   sourceTag.toRawUTF8(), engine, hq, rate, depth, offset, width, color, mix);
}

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

juce::String encodeCoreAssignmentsForStateProperty(const choroboros::CoreAssignmentTable& table)
{
    return juce::JSON::toString(buildCoreAssignmentsVar(table), false);
}

bool decodeCoreAssignmentsFromStateProperty(const juce::String& text, choroboros::CoreAssignmentTable& outTable)
{
    const auto parsed = juce::JSON::parse(text);
    if (parsed.isVoid())
        return false;
    return parseCoreAssignmentsFromVar(parsed, outTable);
}

void loadRuntimeTuningFromVar(const juce::var& internalsVar, ChorusDSP::RuntimeTuning& internals)
{
    internals.rateSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "rateSmoothingMs", internals.rateSmoothingMs.load())));
    internals.depthSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "depthSmoothingMs", internals.depthSmoothingMs.load())));
    internals.depthRateLimit.store(static_cast<float>(getNumberOrDefault(internalsVar, "depthRateLimit", internals.depthRateLimit.load())));
    internals.centreDelaySmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "centreDelaySmoothingMs", internals.centreDelaySmoothingMs.load())));
    internals.centreDelayBaseMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "centreDelayBaseMs", internals.centreDelayBaseMs.load())));
    internals.centreDelayScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "centreDelayScale", internals.centreDelayScale.load())));
    internals.colorSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "colorSmoothingMs", internals.colorSmoothingMs.load())));
    internals.widthSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "widthSmoothingMs", internals.widthSmoothingMs.load())));
    internals.hpfCutoffHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "hpfCutoffHz", internals.hpfCutoffHz.load())));
    internals.hpfQ.store(static_cast<float>(getNumberOrDefault(internalsVar, "hpfQ", internals.hpfQ.load())));
    internals.lpfCutoffHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "lpfCutoffHz", internals.lpfCutoffHz.load())));
    internals.lpfQ.store(static_cast<float>(getNumberOrDefault(internalsVar, "lpfQ", internals.lpfQ.load())));
    internals.preEmphasisFreqHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisFreqHz", internals.preEmphasisFreqHz.load())));
    internals.preEmphasisQ.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisQ", internals.preEmphasisQ.load())));
    internals.preEmphasisGain.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisGain", internals.preEmphasisGain.load())));
    internals.preEmphasisLevelSmoothing.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisLevelSmoothing", internals.preEmphasisLevelSmoothing.load())));
    internals.preEmphasisQuietThreshold.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisQuietThreshold", internals.preEmphasisQuietThreshold.load())));
    internals.preEmphasisMaxAmount.store(static_cast<float>(getNumberOrDefault(internalsVar, "preEmphasisMaxAmount", internals.preEmphasisMaxAmount.load())));
    internals.compressorAttackMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "compressorAttackMs", internals.compressorAttackMs.load())));
    internals.compressorReleaseMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "compressorReleaseMs", internals.compressorReleaseMs.load())));
    internals.compressorThresholdDb.store(static_cast<float>(getNumberOrDefault(internalsVar, "compressorThresholdDb", internals.compressorThresholdDb.load())));
    internals.compressorRatio.store(static_cast<float>(getNumberOrDefault(internalsVar, "compressorRatio", internals.compressorRatio.load())));
    internals.saturationDriveScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "saturationDriveScale", internals.saturationDriveScale.load())));
    internals.greenBloomExponent.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomExponent", internals.greenBloomExponent.load())));
    internals.greenBloomDepthScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomDepthScale", internals.greenBloomDepthScale.load())));
    internals.greenBloomCentreOffsetMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomCentreOffsetMs", internals.greenBloomCentreOffsetMs.load())));
    internals.greenBloomCutoffMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomCutoffMaxHz", internals.greenBloomCutoffMaxHz.load())));
    internals.greenBloomCutoffMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomCutoffMinHz", internals.greenBloomCutoffMinHz.load())));
    internals.greenBloomWetBlend.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomWetBlend", internals.greenBloomWetBlend.load())));
    internals.greenBloomGain.store(static_cast<float>(getNumberOrDefault(internalsVar, "greenBloomGain", internals.greenBloomGain.load())));
    internals.blueFocusExponent.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusExponent", internals.blueFocusExponent.load())));
    internals.blueFocusHpMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusHpMinHz", internals.blueFocusHpMinHz.load())));
    internals.blueFocusHpMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusHpMaxHz", internals.blueFocusHpMaxHz.load())));
    internals.blueFocusLpMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusLpMaxHz", internals.blueFocusLpMaxHz.load())));
    internals.blueFocusLpMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusLpMinHz", internals.blueFocusLpMinHz.load())));
    internals.bluePresenceFreqMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "bluePresenceFreqMinHz", internals.bluePresenceFreqMinHz.load())));
    internals.bluePresenceFreqMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "bluePresenceFreqMaxHz", internals.bluePresenceFreqMaxHz.load())));
    internals.bluePresenceQMin.store(static_cast<float>(getNumberOrDefault(internalsVar, "bluePresenceQMin", internals.bluePresenceQMin.load())));
    internals.bluePresenceQMax.store(static_cast<float>(getNumberOrDefault(internalsVar, "bluePresenceQMax", internals.bluePresenceQMax.load())));
    internals.bluePresenceGainMaxDb.store(static_cast<float>(getNumberOrDefault(internalsVar, "bluePresenceGainMaxDb", internals.bluePresenceGainMaxDb.load())));
    internals.blueFocusWetBlend.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusWetBlend", internals.blueFocusWetBlend.load())));
    internals.blueFocusOutputGain.store(static_cast<float>(getNumberOrDefault(internalsVar, "blueFocusOutputGain", internals.blueFocusOutputGain.load())));
    internals.purpleWarpA.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleWarpA", internals.purpleWarpA.load())));
    internals.purpleWarpB.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleWarpB", internals.purpleWarpB.load())));
    internals.purpleWarpKBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleWarpKBase", internals.purpleWarpKBase.load())));
    internals.purpleWarpKScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleWarpKScale", internals.purpleWarpKScale.load())));
    internals.purpleWarpDelaySmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleWarpDelaySmoothingMs", internals.purpleWarpDelaySmoothingMs.load())));
    internals.purpleOrbitEccentricity.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitEccentricity", internals.purpleOrbitEccentricity.load())));
    internals.purpleOrbitThetaRateBaseHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitThetaRateBaseHz", internals.purpleOrbitThetaRateBaseHz.load())));
    internals.purpleOrbitThetaRateScaleHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitThetaRateScaleHz", internals.purpleOrbitThetaRateScaleHz.load())));
    internals.purpleOrbitThetaRate2Ratio.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitThetaRate2Ratio", internals.purpleOrbitThetaRate2Ratio.load())));
    internals.purpleOrbitEccentricity2Ratio.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitEccentricity2Ratio", internals.purpleOrbitEccentricity2Ratio.load())));
    internals.purpleOrbitMix1.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitMix1", internals.purpleOrbitMix1.load())));
    internals.purpleOrbitStereoThetaOffset.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitStereoThetaOffset", internals.purpleOrbitStereoThetaOffset.load())));
    internals.purpleOrbitDelaySmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "purpleOrbitDelaySmoothingMs", internals.purpleOrbitDelaySmoothingMs.load())));
    internals.blackNqDepthBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackNqDepthBase", internals.blackNqDepthBase.load())));
    internals.blackNqDepthScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackNqDepthScale", internals.blackNqDepthScale.load())));
    internals.blackNqDelayGlideMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackNqDelayGlideMs", internals.blackNqDelayGlideMs.load())));
    internals.blackHqTap2MixBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqTap2MixBase", internals.blackHqTap2MixBase.load())));
    internals.blackHqTap2MixScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqTap2MixScale", internals.blackHqTap2MixScale.load())));
    internals.blackHqSecondTapDepthBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqSecondTapDepthBase", internals.blackHqSecondTapDepthBase.load())));
    internals.blackHqSecondTapDepthScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqSecondTapDepthScale", internals.blackHqSecondTapDepthScale.load())));
    internals.blackHqSecondTapDelayOffsetBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqSecondTapDelayOffsetBase", internals.blackHqSecondTapDelayOffsetBase.load())));
    internals.blackHqSecondTapDelayOffsetScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "blackHqSecondTapDelayOffsetScale", internals.blackHqSecondTapDelayOffsetScale.load())));
    internals.bbdDelaySmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdDelaySmoothingMs", internals.bbdDelaySmoothingMs.load())));
    internals.bbdDelayMinMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdDelayMinMs", internals.bbdDelayMinMs.load())));
    internals.bbdDelayMaxMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdDelayMaxMs", internals.bbdDelayMaxMs.load())));
    internals.bbdCentreBaseMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdCentreBaseMs", internals.bbdCentreBaseMs.load())));
    internals.bbdCentreScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdCentreScale", internals.bbdCentreScale.load())));
    internals.bbdDepthMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdDepthMs", internals.bbdDepthMs.load())));
    internals.bbdClockSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdClockSmoothingMs", internals.bbdClockSmoothingMs.load())));
    internals.bbdFilterSmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdFilterSmoothingMs", internals.bbdFilterSmoothingMs.load())));
    internals.bbdFilterCutoffMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdFilterCutoffMinHz", internals.bbdFilterCutoffMinHz.load())));
    internals.bbdFilterCutoffMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdFilterCutoffMaxHz", internals.bbdFilterCutoffMaxHz.load())));
    internals.bbdFilterCutoffScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdFilterCutoffScale", internals.bbdFilterCutoffScale.load())));
    internals.bbdClockMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdClockMinHz", internals.bbdClockMinHz.load())));
    internals.bbdClockMaxRatio.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdClockMaxRatio", internals.bbdClockMaxRatio.load())));
    internals.bbdStages.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdStages", internals.bbdStages.load())));
    internals.bbdFilterMaxRatio.store(static_cast<float>(getNumberOrDefault(internalsVar, "bbdFilterMaxRatio", internals.bbdFilterMaxRatio.load())));
    internals.tapeDelaySmoothingMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeDelaySmoothingMs", internals.tapeDelaySmoothingMs.load())));
    internals.tapeCentreBaseMs.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeCentreBaseMs", internals.tapeCentreBaseMs.load())));
    internals.tapeCentreScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeCentreScale", internals.tapeCentreScale.load())));
    internals.tapeToneMaxHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeToneMaxHz", internals.tapeToneMaxHz.load())));
    internals.tapeToneMinHz.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeToneMinHz", internals.tapeToneMinHz.load())));
    internals.tapeToneSmoothingCoeff.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeToneSmoothingCoeff", internals.tapeToneSmoothingCoeff.load())));
    internals.tapeDriveScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeDriveScale", internals.tapeDriveScale.load())));
    internals.tapeLfoRatioScale.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeLfoRatioScale", internals.tapeLfoRatioScale.load())));
    internals.tapeLfoModSmoothingCoeff.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeLfoModSmoothingCoeff", internals.tapeLfoModSmoothingCoeff.load())));
    internals.tapeRatioSmoothingCoeff.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeRatioSmoothingCoeff", internals.tapeRatioSmoothingCoeff.load())));
    internals.tapePhaseDamping.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapePhaseDamping", internals.tapePhaseDamping.load())));
    internals.tapeWowFreqBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeWowFreqBase", internals.tapeWowFreqBase.load())));
    internals.tapeWowFreqSpread.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeWowFreqSpread", internals.tapeWowFreqSpread.load())));
    internals.tapeFlutterFreqBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeFlutterFreqBase", internals.tapeFlutterFreqBase.load())));
    internals.tapeFlutterFreqSpread.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeFlutterFreqSpread", internals.tapeFlutterFreqSpread.load())));
    internals.tapeWowDepthBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeWowDepthBase", internals.tapeWowDepthBase.load())));
    internals.tapeWowDepthSpread.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeWowDepthSpread", internals.tapeWowDepthSpread.load())));
    internals.tapeFlutterDepthBase.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeFlutterDepthBase", internals.tapeFlutterDepthBase.load())));
    internals.tapeFlutterDepthSpread.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeFlutterDepthSpread", internals.tapeFlutterDepthSpread.load())));
    internals.tapeRatioMin.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeRatioMin", internals.tapeRatioMin.load())));
    internals.tapeRatioMax.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeRatioMax", internals.tapeRatioMax.load())));
    internals.tapeWetGain.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeWetGain", internals.tapeWetGain.load())));
    internals.tapeHermiteTension.store(static_cast<float>(getNumberOrDefault(internalsVar, "tapeHermiteTension", internals.tapeHermiteTension.load())));
}

void setRedNQDefaults(ChorusDSP::RuntimeTuning& r)
{
    r.rateSmoothingMs.store(23.1f);
    r.depthSmoothingMs.store(43.2f);
    r.depthRateLimit.store(5.0f);
    r.centreDelaySmoothingMs.store(152.4f);
    r.centreDelayBaseMs.store(9.3f);
    r.centreDelayScale.store(1.6f);
    r.colorSmoothingMs.store(127.4f);
    r.widthSmoothingMs.store(98.7f);
    r.hpfCutoffHz.store(82.2f);
    r.hpfQ.store(1.184f);
    r.lpfCutoffHz.store(17000.0f);
    r.lpfQ.store(0.95f);
    r.preEmphasisFreqHz.store(1502.0f);
    r.preEmphasisQ.store(1.706f);
    r.preEmphasisGain.store(1.343f);
    r.preEmphasisLevelSmoothing.store(0.866f);
    r.preEmphasisQuietThreshold.store(0.535f);
    r.preEmphasisMaxAmount.store(0.961f);
    r.compressorAttackMs.store(19.4f);
    r.compressorReleaseMs.store(141.9f);
    r.compressorThresholdDb.store(-12.0f);
    r.compressorRatio.store(2.8f);
    r.saturationDriveScale.store(1.65f);
    r.bbdDelaySmoothingMs.store(52.2f);
    r.bbdDelayMinMs.store(5.9f);
    r.bbdDelayMaxMs.store(86.9f);
    r.bbdCentreBaseMs.store(14.7f);
    r.bbdCentreScale.store(3.28f);
    r.bbdDepthMs.store(13.2f);
    r.bbdClockSmoothingMs.store(41.7f);
    r.bbdFilterSmoothingMs.store(46.7f);
    r.bbdFilterCutoffMinHz.store(900.0f);
    r.bbdFilterCutoffMaxHz.store(16000.0f);
    r.bbdFilterCutoffScale.store(0.62f);
    r.bbdClockMinHz.store(1200.0f);
    r.bbdClockMaxRatio.store(1.0f);
    r.bbdStages.store(1024.0f);
    r.bbdFilterMaxRatio.store(0.42f);
    r.tapeDelaySmoothingMs.store(180.0f);
    r.tapeCentreBaseMs.store(16.0f);
    r.tapeCentreScale.store(2.0f);
    r.tapeToneMaxHz.store(16000.0f);
    r.tapeToneMinHz.store(12000.0f);
    r.tapeToneSmoothingCoeff.store(0.08f);
    r.tapeDriveScale.store(0.35f);
    r.tapeLfoRatioScale.store(0.05f);
    r.tapeLfoModSmoothingCoeff.store(0.0015f);
    r.tapeRatioSmoothingCoeff.store(0.004f);
    r.tapePhaseDamping.store(1.0f);
    r.tapeWowFreqBase.store(0.33f);
    r.tapeWowFreqSpread.store(0.03f);
    r.tapeFlutterFreqBase.store(5.8f);
    r.tapeFlutterFreqSpread.store(0.2f);
    r.tapeWowDepthBase.store(0.0022f);
    r.tapeWowDepthSpread.store(0.0002f);
    r.tapeFlutterDepthBase.store(0.0011f);
    r.tapeFlutterDepthSpread.store(0.0001f);
    r.tapeRatioMin.store(0.96f);
    r.tapeRatioMax.store(1.04f);
    r.tapeWetGain.store(1.15f);
    r.tapeHermiteTension.store(0.75f);
}

void setRedHQDefaults(ChorusDSP::RuntimeTuning& r)
{
    r.rateSmoothingMs.store(23.1f);
    r.depthSmoothingMs.store(43.2f);
    r.depthRateLimit.store(5.0f);
    r.centreDelaySmoothingMs.store(152.4f);
    r.centreDelayBaseMs.store(9.3f);
    r.centreDelayScale.store(1.6f);
    r.colorSmoothingMs.store(110.0f);
    r.widthSmoothingMs.store(90.0f);
    r.hpfCutoffHz.store(45.0f);
    r.hpfQ.store(0.9f);
    r.lpfCutoffHz.store(19500.0f);
    r.lpfQ.store(0.8f);
    r.preEmphasisFreqHz.store(3200.0f);
    r.preEmphasisQ.store(0.8f);
    r.preEmphasisGain.store(1.15f);
    r.preEmphasisLevelSmoothing.store(0.9f);
    r.preEmphasisQuietThreshold.store(0.2f);
    r.preEmphasisMaxAmount.store(0.35f);
    r.compressorAttackMs.store(18.0f);
    r.compressorReleaseMs.store(160.0f);
    r.compressorThresholdDb.store(-10.0f);
    r.compressorRatio.store(2.4f);
    r.saturationDriveScale.store(1.65f);
    r.bbdDelaySmoothingMs.store(52.2f);
    r.bbdDelayMinMs.store(5.9f);
    r.bbdDelayMaxMs.store(86.9f);
    r.bbdCentreBaseMs.store(14.7f);
    r.bbdCentreScale.store(3.28f);
    r.bbdDepthMs.store(13.2f);
    r.bbdClockSmoothingMs.store(41.7f);
    r.bbdFilterSmoothingMs.store(46.7f);
    r.bbdFilterCutoffMinHz.store(900.0f);
    r.bbdFilterCutoffMaxHz.store(16000.0f);
    r.bbdFilterCutoffScale.store(0.62f);
    r.bbdClockMinHz.store(1200.0f);
    r.bbdClockMaxRatio.store(1.0f);
    r.bbdStages.store(1024.0f);
    r.bbdFilterMaxRatio.store(0.42f);
    r.tapeDelaySmoothingMs.store(140.0f);
    r.tapeCentreBaseMs.store(15.0f);
    r.tapeCentreScale.store(2.6f);
    r.tapeToneMaxHz.store(20000.0f);
    r.tapeToneMinHz.store(14000.0f);
    r.tapeToneSmoothingCoeff.store(0.06f);
    r.tapeDriveScale.store(0.22f);
    r.tapeLfoRatioScale.store(0.045f);
    r.tapeLfoModSmoothingCoeff.store(0.0015f);
    r.tapeRatioSmoothingCoeff.store(0.004f);
    r.tapePhaseDamping.store(1.0f);
    r.tapeWowFreqBase.store(0.33f);
    r.tapeWowFreqSpread.store(0.03f);
    r.tapeFlutterFreqBase.store(5.8f);
    r.tapeFlutterFreqSpread.store(0.2f);
    r.tapeWowDepthBase.store(0.0022f);
    r.tapeWowDepthSpread.store(0.0002f);
    r.tapeFlutterDepthBase.store(0.0011f);
    r.tapeFlutterDepthSpread.store(0.0001f);
    r.tapeRatioMin.store(0.97f);
    r.tapeRatioMax.store(1.03f);
    r.tapeWetGain.store(1.08f);
    r.tapeHermiteTension.store(0.75f);
}

bool isKnownBadRedNQProfile(const ChorusDSP::RuntimeTuning& r)
{
    // Detect the previously reported broken Red NQ/BBD profile shape:
    // zero BBD depth + max stages + very low clock min + oversized delay window.
    return r.bbdDepthMs.load() <= 0.01f
        && r.bbdStages.load() >= 2048.0f
        && r.bbdClockMinHz.load() <= 250.0f
        && r.bbdDelayMaxMs.load() >= 120.0f
        && r.bbdDelaySmoothingMs.load() >= 150.0f;
}

void copyRuntimeTuningValues(const ChorusDSP::RuntimeTuning& src, ChorusDSP::RuntimeTuning& dst)
{
    dst.rateSmoothingMs.store(src.rateSmoothingMs.load());
    dst.depthSmoothingMs.store(src.depthSmoothingMs.load());
    dst.depthRateLimit.store(src.depthRateLimit.load());
    dst.centreDelaySmoothingMs.store(src.centreDelaySmoothingMs.load());
    dst.centreDelayBaseMs.store(src.centreDelayBaseMs.load());
    dst.centreDelayScale.store(src.centreDelayScale.load());
    dst.colorSmoothingMs.store(src.colorSmoothingMs.load());
    dst.widthSmoothingMs.store(src.widthSmoothingMs.load());
    dst.hpfCutoffHz.store(src.hpfCutoffHz.load());
    dst.hpfQ.store(src.hpfQ.load());
    dst.lpfCutoffHz.store(src.lpfCutoffHz.load());
    dst.lpfQ.store(src.lpfQ.load());
    dst.preEmphasisFreqHz.store(src.preEmphasisFreqHz.load());
    dst.preEmphasisQ.store(src.preEmphasisQ.load());
    dst.preEmphasisGain.store(src.preEmphasisGain.load());
    dst.preEmphasisLevelSmoothing.store(src.preEmphasisLevelSmoothing.load());
    dst.preEmphasisQuietThreshold.store(src.preEmphasisQuietThreshold.load());
    dst.preEmphasisMaxAmount.store(src.preEmphasisMaxAmount.load());
    dst.compressorAttackMs.store(src.compressorAttackMs.load());
    dst.compressorReleaseMs.store(src.compressorReleaseMs.load());
    dst.compressorThresholdDb.store(src.compressorThresholdDb.load());
    dst.compressorRatio.store(src.compressorRatio.load());
    dst.saturationDriveScale.store(src.saturationDriveScale.load());
    dst.greenBloomExponent.store(src.greenBloomExponent.load());
    dst.greenBloomDepthScale.store(src.greenBloomDepthScale.load());
    dst.greenBloomCentreOffsetMs.store(src.greenBloomCentreOffsetMs.load());
    dst.greenBloomCutoffMaxHz.store(src.greenBloomCutoffMaxHz.load());
    dst.greenBloomCutoffMinHz.store(src.greenBloomCutoffMinHz.load());
    dst.greenBloomWetBlend.store(src.greenBloomWetBlend.load());
    dst.greenBloomGain.store(src.greenBloomGain.load());
    dst.blueFocusExponent.store(src.blueFocusExponent.load());
    dst.blueFocusHpMinHz.store(src.blueFocusHpMinHz.load());
    dst.blueFocusHpMaxHz.store(src.blueFocusHpMaxHz.load());
    dst.blueFocusLpMaxHz.store(src.blueFocusLpMaxHz.load());
    dst.blueFocusLpMinHz.store(src.blueFocusLpMinHz.load());
    dst.bluePresenceFreqMinHz.store(src.bluePresenceFreqMinHz.load());
    dst.bluePresenceFreqMaxHz.store(src.bluePresenceFreqMaxHz.load());
    dst.bluePresenceQMin.store(src.bluePresenceQMin.load());
    dst.bluePresenceQMax.store(src.bluePresenceQMax.load());
    dst.bluePresenceGainMaxDb.store(src.bluePresenceGainMaxDb.load());
    dst.blueFocusWetBlend.store(src.blueFocusWetBlend.load());
    dst.blueFocusOutputGain.store(src.blueFocusOutputGain.load());
    dst.purpleWarpA.store(src.purpleWarpA.load());
    dst.purpleWarpB.store(src.purpleWarpB.load());
    dst.purpleWarpKBase.store(src.purpleWarpKBase.load());
    dst.purpleWarpKScale.store(src.purpleWarpKScale.load());
    dst.purpleWarpDelaySmoothingMs.store(src.purpleWarpDelaySmoothingMs.load());
    dst.purpleOrbitEccentricity.store(src.purpleOrbitEccentricity.load());
    dst.purpleOrbitThetaRateBaseHz.store(src.purpleOrbitThetaRateBaseHz.load());
    dst.purpleOrbitThetaRateScaleHz.store(src.purpleOrbitThetaRateScaleHz.load());
    dst.purpleOrbitThetaRate2Ratio.store(src.purpleOrbitThetaRate2Ratio.load());
    dst.purpleOrbitEccentricity2Ratio.store(src.purpleOrbitEccentricity2Ratio.load());
    dst.purpleOrbitMix1.store(src.purpleOrbitMix1.load());
    dst.purpleOrbitStereoThetaOffset.store(src.purpleOrbitStereoThetaOffset.load());
    dst.purpleOrbitDelaySmoothingMs.store(src.purpleOrbitDelaySmoothingMs.load());
    dst.blackNqDepthBase.store(src.blackNqDepthBase.load());
    dst.blackNqDepthScale.store(src.blackNqDepthScale.load());
    dst.blackNqDelayGlideMs.store(src.blackNqDelayGlideMs.load());
    dst.blackHqTap2MixBase.store(src.blackHqTap2MixBase.load());
    dst.blackHqTap2MixScale.store(src.blackHqTap2MixScale.load());
    dst.blackHqSecondTapDepthBase.store(src.blackHqSecondTapDepthBase.load());
    dst.blackHqSecondTapDepthScale.store(src.blackHqSecondTapDepthScale.load());
    dst.blackHqSecondTapDelayOffsetBase.store(src.blackHqSecondTapDelayOffsetBase.load());
    dst.blackHqSecondTapDelayOffsetScale.store(src.blackHqSecondTapDelayOffsetScale.load());
    dst.bbdDelaySmoothingMs.store(src.bbdDelaySmoothingMs.load());
    dst.bbdDelayMinMs.store(src.bbdDelayMinMs.load());
    dst.bbdDelayMaxMs.store(src.bbdDelayMaxMs.load());
    dst.bbdCentreBaseMs.store(src.bbdCentreBaseMs.load());
    dst.bbdCentreScale.store(src.bbdCentreScale.load());
    dst.bbdDepthMs.store(src.bbdDepthMs.load());
    dst.bbdClockSmoothingMs.store(src.bbdClockSmoothingMs.load());
    dst.bbdFilterSmoothingMs.store(src.bbdFilterSmoothingMs.load());
    dst.bbdFilterCutoffMinHz.store(src.bbdFilterCutoffMinHz.load());
    dst.bbdFilterCutoffMaxHz.store(src.bbdFilterCutoffMaxHz.load());
    dst.bbdFilterCutoffScale.store(src.bbdFilterCutoffScale.load());
    dst.bbdClockMinHz.store(src.bbdClockMinHz.load());
    dst.bbdClockMaxRatio.store(src.bbdClockMaxRatio.load());
    dst.bbdStages.store(src.bbdStages.load());
    dst.bbdFilterMaxRatio.store(src.bbdFilterMaxRatio.load());
    dst.tapeDelaySmoothingMs.store(src.tapeDelaySmoothingMs.load());
    dst.tapeCentreBaseMs.store(src.tapeCentreBaseMs.load());
    dst.tapeCentreScale.store(src.tapeCentreScale.load());
    dst.tapeToneMaxHz.store(src.tapeToneMaxHz.load());
    dst.tapeToneMinHz.store(src.tapeToneMinHz.load());
    dst.tapeToneSmoothingCoeff.store(src.tapeToneSmoothingCoeff.load());
    dst.tapeDriveScale.store(src.tapeDriveScale.load());
    dst.tapeLfoRatioScale.store(src.tapeLfoRatioScale.load());
    dst.tapeLfoModSmoothingCoeff.store(src.tapeLfoModSmoothingCoeff.load());
    dst.tapeRatioSmoothingCoeff.store(src.tapeRatioSmoothingCoeff.load());
    dst.tapePhaseDamping.store(src.tapePhaseDamping.load());
    dst.tapeWowFreqBase.store(src.tapeWowFreqBase.load());
    dst.tapeWowFreqSpread.store(src.tapeWowFreqSpread.load());
    dst.tapeFlutterFreqBase.store(src.tapeFlutterFreqBase.load());
    dst.tapeFlutterFreqSpread.store(src.tapeFlutterFreqSpread.load());
    dst.tapeWowDepthBase.store(src.tapeWowDepthBase.load());
    dst.tapeWowDepthSpread.store(src.tapeWowDepthSpread.load());
    dst.tapeFlutterDepthBase.store(src.tapeFlutterDepthBase.load());
    dst.tapeFlutterDepthSpread.store(src.tapeFlutterDepthSpread.load());
    dst.tapeRatioMin.store(src.tapeRatioMin.load());
    dst.tapeRatioMax.store(src.tapeRatioMax.load());
    dst.tapeWetGain.store(src.tapeWetGain.load());
    dst.tapeHermiteTension.store(src.tapeHermiteTension.load());
}

void loadPersistedDefaults(ChoroborosAudioProcessor& processor)
{
    const auto json = DefaultsPersistence::load();
    if (json.isEmpty())
        return;

    const auto parsed = juce::JSON::parse(json);
    if (parsed.isVoid())
        return;

    const auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return;

    choroboros::CoreAssignmentTable loadedAssignments;
    bool assignmentsLoaded = false;
    if (root->hasProperty("coreAssignments"))
        assignmentsLoaded = parseCoreAssignmentsFromVar(root->getProperty("coreAssignments"), loadedAssignments);
    if (!assignmentsLoaded)
        loadedAssignments.resetToLegacy();
    processor.setCoreAssignments(loadedAssignments);

    bool modularEnabled = false;
    if (root->hasProperty("modularCoresEnabled"))
    {
        const auto value = root->getProperty("modularCoresEnabled");
        if (value.isBool())
            modularEnabled = static_cast<bool>(value);
        else if (value.isInt() || value.isInt64() || value.isDouble())
            modularEnabled = static_cast<double>(value) >= 0.5;
        else
            modularEnabled = value.toString().equalsIgnoreCase("true") || value.toString() == "1";
    }
    processor.setModularCoresEnabled(modularEnabled);

    if (root->hasProperty("tuning"))
    {
        const juce::var tuningVar = root->getProperty("tuning");
        const auto* tuningObj = tuningVar.getDynamicObject();
        if (tuningObj != nullptr)
        {
            auto& tuning = processor.getTuningState();
            auto loadParam = [](const juce::var& parent, const juce::String& name, ChoroborosAudioProcessor::ParamTuning& param)
            {
                const auto* parentObj = parent.getDynamicObject();
                if (parentObj == nullptr || !parentObj->hasProperty(name))
                    return;

                const juce::var paramVar = parentObj->getProperty(name);
                const auto* paramObj = paramVar.getDynamicObject();
                if (paramObj == nullptr)
                    return;

                float loadedMin = static_cast<float>(getNumberOrDefault(paramVar, "min", static_cast<double>(param.min.load())));
                float loadedMax = static_cast<float>(getNumberOrDefault(paramVar, "max", static_cast<double>(param.max.load())));
                // Migration: color min was 0.1286 (12.86%), now allow 0
                if (name == "color" && loadedMin > 0.0f && loadedMin < 0.2f)
                    loadedMin = 0.0f;
                param.min.store(loadedMin);
                param.max.store(loadedMax);
                param.curve.store(static_cast<float>(getNumberOrDefault(paramVar, "curve", static_cast<double>(param.curve.load()))));
                param.uiSkew.store(static_cast<float>(getNumberOrDefault(paramVar, "uiSkew", static_cast<double>(param.uiSkew.load()))));
            };

            loadParam(tuningVar, "rate", tuning.rate);
            loadParam(tuningVar, "depth", tuning.depth);
            loadParam(tuningVar, "offset", tuning.offset);
            loadParam(tuningVar, "width", tuning.width);
            loadParam(tuningVar, "color", tuning.color);
            loadParam(tuningVar, "mix", tuning.mix);
        }
    }

    bool loadedRedNqVariantInternals = false;
    bool loadedRedHqVariantInternals = false;

    if (root->hasProperty("internals"))
    {
        const juce::var internalsVar = root->getProperty("internals");
        loadRuntimeTuningFromVar(internalsVar, processor.getDspInternals());

        // Backward compatibility: if only legacy internals exist, seed all profiles from it.
        for (int color = 0; color < 5; ++color)
        {
            loadRuntimeTuningFromVar(internalsVar, processor.getEngineDspInternals(color, false));
            loadRuntimeTuningFromVar(internalsVar, processor.getEngineDspInternals(color, true));
        }
    }

    const auto loadVariant = [&](const juce::Identifier& key, int colorIndex, bool hqEnabled)
    {
        if (!root->hasProperty(key))
            return;
        loadRuntimeTuningFromVar(root->getProperty(key), processor.getEngineDspInternals(colorIndex, hqEnabled));
        if (colorIndex == 2 && !hqEnabled)
            loadedRedNqVariantInternals = true;
        if (colorIndex == 2 && hqEnabled)
            loadedRedHqVariantInternals = true;
    };

    loadVariant("internalsGreen", 0, false);
    loadVariant("internalsBlue", 1, false);
    loadVariant("internalsRed", 2, false);
    loadVariant("internalsPurple", 3, false);
    loadVariant("internalsBlack", 4, false);

    loadVariant("internalsGreenHQ", 0, true);
    loadVariant("internalsBlueHQ", 1, true);
    loadVariant("internalsRedHQ", 2, true);
    loadVariant("internalsPurpleHQ", 3, true);
    loadVariant("internalsBlackHQ", 4, true);

    // If red variants were never explicitly stored, restore tuned factory red profiles
    // instead of inheriting a legacy "internals" blob that can make both modes too dark.
    if (!loadedRedNqVariantInternals)
        setRedNQDefaults(processor.getEngineDspInternals(2, false));
    if (!loadedRedHqVariantInternals)
        setRedHQDefaults(processor.getEngineDspInternals(2, true));
    if (isKnownBadRedNQProfile(processor.getEngineDspInternals(2, false)))
        setRedNQDefaults(processor.getEngineDspInternals(2, false));

    const int activeEngine = processor.getCurrentEngineColorIndex();
    const bool activeHQ = processor.isHqEnabled();
    processor.syncEngineInternalsToActiveDsp(activeEngine, activeHQ);

    if (root->hasProperty("engineParamProfiles"))
        processor.loadEngineParamProfilesFromVar(root->getProperty("engineParamProfiles"));
}

void seedPersistedDefaultsFromBundledWindowsFactory()
{
#if JUCE_WINDOWS
    const auto userFile = DefaultsPersistence::getUserDefaultsFile();
    const auto factoryFile = DefaultsPersistence::getFactoryDefaultsFile();
    const bool seedUser = !userFile.existsAsFile() || userFile.getSize() <= 0;
    const bool seedFactory = !factoryFile.existsAsFile() || factoryFile.getSize() <= 0;
    if (!seedUser && !seedFactory)
        return;

    int dataSize = 0;
    const char* data = BinaryData::getNamedResource("windows_factory_defaults_json", dataSize);
    if (data == nullptr || dataSize <= 0)
        return;

    const juce::String bundledJson = juce::String::fromUTF8(data, dataSize);
    if (bundledJson.isEmpty() || juce::JSON::parse(bundledJson).isVoid())
        return;

    juce::String writeError;
    if (seedFactory)
        DefaultsPersistence::saveFactory(bundledJson, &writeError);
    if (seedUser)
        DefaultsPersistence::saveUser(bundledJson, &writeError);
#endif
}
} // namespace

void ChoroborosAudioProcessor::StereoTapRingBuffer::clear() noexcept
{
    std::fill(left.begin(), left.end(), 0.0f);
    std::fill(right.begin(), right.end(), 0.0f);
    writeIndex.store(0, std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::StereoTapRingBuffer::push(const float* leftIn, const float* rightIn, int numSamples) noexcept
{
    if (numSamples <= 0 || leftIn == nullptr)
        return;

    auto write = writeIndex.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        const std::uint32_t slot = static_cast<std::uint32_t>((write + static_cast<std::uint64_t>(i)) & (capacity - 1u));
        left[slot] = leftIn[i];
        right[slot] = (rightIn != nullptr) ? rightIn[i] : leftIn[i];
    }
    writeIndex.store(write + static_cast<std::uint64_t>(numSamples), std::memory_order_release);
}

void ChoroborosAudioProcessor::StereoTapRingBuffer::copyLatest(float* leftOut, float* rightOut, int numSamples) const noexcept
{
    if (numSamples <= 0 || leftOut == nullptr || rightOut == nullptr)
        return;

    const int clampedSamples = juce::jmin(numSamples, static_cast<int>(capacity));
    const auto write = writeIndex.load(std::memory_order_acquire);
    const int available = static_cast<int>(juce::jmin<std::uint64_t>(write, capacity));
    const int toCopy = juce::jmin(clampedSamples, available);
    const int zeroPrefix = clampedSamples - toCopy;

    for (int i = 0; i < zeroPrefix; ++i)
    {
        leftOut[i] = 0.0f;
        rightOut[i] = 0.0f;
    }

    const auto readStart = write - static_cast<std::uint64_t>(toCopy);
    for (int i = 0; i < toCopy; ++i)
    {
        const std::uint32_t slot = static_cast<std::uint32_t>((readStart + static_cast<std::uint64_t>(i)) & (capacity - 1u));
        leftOut[zeroPrefix + i] = left[slot];
        rightOut[zeroPrefix + i] = right[slot];
    }
}

class ChoroborosAudioProcessor::AnalyzerWorker final : public juce::Thread
{
public:
    explicit AnalyzerWorker(ChoroborosAudioProcessor& ownerIn)
        : juce::Thread("ChoroborosAnalyzer"),
          owner(ownerIn)
    {
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            const auto& flags = owner.getDiagnosticFeatureFlags();
            const bool anyDemand = flags.modulationCardEnabled.load(std::memory_order_relaxed)
                                || flags.spectrumCardEnabled.load(std::memory_order_relaxed)
                                || flags.transferCardEnabled.load(std::memory_order_relaxed)
                                || flags.telemetryCardEnabled.load(std::memory_order_relaxed);

            if (flags.analyzersEnabled.load(std::memory_order_relaxed) && anyDemand)
                owner.runAnalyzerPass();

            const int hz = owner.getAnalyzerRefreshHz();
            const int sleepMs = anyDemand
                ? juce::jmax(8, 1000 / juce::jlimit(5, 60, hz))
                : 120;
            wait(sleepMs);
        }
    }

private:
    ChoroborosAudioProcessor& owner;
};

//==============================================================================
ChoroborosAudioProcessor::ChoroborosAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("Choroboros"), createParameterLayout()),
    chorusDSP(std::make_unique<ChorusDSP>()),
    feedbackCollector(std::make_unique<FeedbackCollector>())
{
    instanceId = nextLoadTraceInstanceId();
    const double ctorStartMs = juce::Time::getMillisecondCounterHiRes();

    initTuningDefaults();
    logLoadTraceEvent("processor_init_tuning_ms",
                      juce::Time::getMillisecondCounterHiRes() - ctorStartMs);

    const double internalsInitStartMs = juce::Time::getMillisecondCounterHiRes();
    initializeEngineInternalProfiles();
    logLoadTraceEvent("processor_init_internals_ms",
                      juce::Time::getMillisecondCounterHiRes() - internalsInitStartMs);

    chorusDSP->setCoreAssignments(coreAssignments);
    chorusDSP->setModularCoreModeEnabled(modularCoresEnabled);

    const double bundledSeedStartMs = juce::Time::getMillisecondCounterHiRes();
    seedPersistedDefaultsFromBundledWindowsFactory();
    logLoadTraceEvent("processor_seed_windows_defaults_ms",
                      juce::Time::getMillisecondCounterHiRes() - bundledSeedStartMs);

    const double persistedDefaultsStartMs = juce::Time::getMillisecondCounterHiRes();
    loadPersistedDefaults(*this);
    logLoadTraceEvent("processor_load_persisted_defaults_ms",
                      juce::Time::getMillisecondCounterHiRes() - persistedDefaultsStartMs);

    const double profileApplyStartMs = juce::Time::getMillisecondCounterHiRes();
    applyEngineParamProfile(getCurrentEngineColorIndex());
    saveCurrentParamsToEngineProfile(getCurrentEngineColorIndex());
    logLoadTraceEvent("processor_apply_active_profile_ms",
                      juce::Time::getMillisecondCounterHiRes() - profileApplyStartMs);
    logLoadTraceEvent("processor_startup_params",
                      0.0,
                      buildStartupParamSnapshotNotes(*this, "source=persisted_defaults"));

    parameters.addParameterListener(ENGINE_COLOR_ID, this);
    parameters.addParameterListener(RATE_ID, this);
    parameters.addParameterListener(DEPTH_ID, this);
    parameters.addParameterListener(OFFSET_ID, this);
    parameters.addParameterListener(WIDTH_ID, this);
    parameters.addParameterListener(COLOR_ID, this);
    parameters.addParameterListener(HQ_ID, this);
    parameters.addParameterListener(MIX_ID, this);
    lastEngineIndex = getCurrentEngineColorIndex();
    analyzerWorker = std::make_unique<AnalyzerWorker>(*this);

    logLoadTraceEvent("processor_ctor_total_ms",
                      juce::Time::getMillisecondCounterHiRes() - ctorStartMs);
}

ChoroborosAudioProcessor::~ChoroborosAudioProcessor()
{
    if (analyzerWorker != nullptr)
        analyzerWorker->stopThread(1500);

    parameters.removeParameterListener(ENGINE_COLOR_ID, this);
    parameters.removeParameterListener(RATE_ID, this);
    parameters.removeParameterListener(DEPTH_ID, this);
    parameters.removeParameterListener(OFFSET_ID, this);
    parameters.removeParameterListener(WIDTH_ID, this);
    parameters.removeParameterListener(COLOR_ID, this);
    parameters.removeParameterListener(HQ_ID, this);
    parameters.removeParameterListener(MIX_ID, this);
}

juce::File ChoroborosAudioProcessor::getLoadTraceLogFile()
{
    return DefaultsPersistence::getUserDefaultsFile()
        .getParentDirectory()
        .getChildFile("load_trace.ndjson");
}

void ChoroborosAudioProcessor::logLoadTraceEvent(const juce::String& eventName,
                                                 double elapsedMs,
                                                 const juce::String& notes) const
{
    appendLoadTraceLine(*this, eventName, elapsedMs, notes);
}

//==============================================================================
const juce::String ChoroborosAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChoroborosAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ChoroborosAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ChoroborosAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ChoroborosAudioProcessor::getTailLengthSeconds() const
{
    return 0.1; // Small tail for delay
}

int ChoroborosAudioProcessor::getNumPrograms()
{
    return 7; // Classic, Vintage, Modern, Psychedelic, Core, Duck, Ouroboros
}

int ChoroborosAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void ChoroborosAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    
    presetLoadInProgress = true;
    currentProgram = index;
    
    // Track preset load for feedback
    if (feedbackCollector)
    {
        feedbackCollector->trackPresetLoad(index, getProgramName(index));
    }

    // Preset targets are defined in mapped/display space.
    const auto setMappedParam = [this](const juce::String& paramId, float mappedValue)
    {
        if (auto* param = parameters.getParameter(paramId))
        {
            const float rawValue = unmapParameterValue(paramId, mappedValue);
            float normalizedValue = param->convertTo0to1(rawValue);
            normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
            param->setValueNotifyingHost(normalizedValue);
        }
    };
    
    // Apply preset values
    if (index == 0) // Classic (Green): NQ, R=1.2Hz, D=21%, O=33°, W=150%, M=50%, C=16%
    {
        setMappedParam(RATE_ID, 1.2f);
        setMappedParam(DEPTH_ID, 0.21f);
        setMappedParam(OFFSET_ID, 33.0f);
        setMappedParam(WIDTH_ID, 1.5f);
        setMappedParam(MIX_ID, 0.5f);
        setMappedParam(COLOR_ID, 0.16f);
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(0.0f);
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(0.0f);
    }
    else if (index == 1) // Vintage (Red): HQ, R=0.62Hz, D=21%, O=56°, W=150%, M=50%, C=50%
    {
        setMappedParam(RATE_ID, 0.62f);
        setMappedParam(DEPTH_ID, 0.21f);
        setMappedParam(OFFSET_ID, 56.0f);
        setMappedParam(WIDTH_ID, 1.5f);
        setMappedParam(MIX_ID, 0.5f);
        setMappedParam(COLOR_ID, 0.5f);
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(2.0f));
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 2) // Modern (Blue): HQ, R=0.26Hz, D=53%, O=59°, W=100%, M=50%, C=41%
    {
        setMappedParam(RATE_ID, 0.26f);
        setMappedParam(DEPTH_ID, 0.53f);
        setMappedParam(OFFSET_ID, 59.0f);
        setMappedParam(WIDTH_ID, 1.0f);
        setMappedParam(MIX_ID, 0.5f);
        setMappedParam(COLOR_ID, 0.41f);
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(1.0f));
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 3) // Psychedelic (Purple): NQ, R=0.12Hz, D=52%, O=52°, W=200%, M=69%, C=13%
    {
        setMappedParam(RATE_ID, 0.12f);
        setMappedParam(DEPTH_ID, 0.52f);
        setMappedParam(OFFSET_ID, 52.0f);
        setMappedParam(WIDTH_ID, 2.0f);
        setMappedParam(MIX_ID, 0.69f);
        setMappedParam(COLOR_ID, 0.13f);
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(3.0f));
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(0.0f);
    }
    else if (index == 4) // Core (Black): HQ, R=1.2Hz, D=35%, O=41°, W=159%, M=50%, C=28%
    {
        setMappedParam(RATE_ID, 1.2f);
        setMappedParam(DEPTH_ID, 0.35f);
        setMappedParam(OFFSET_ID, 41.0f);
        setMappedParam(WIDTH_ID, 1.59f);
        setMappedParam(MIX_ID, 0.5f);
        setMappedParam(COLOR_ID, 0.28f);
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(4.0f));
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 5) // Duck preset
    {
        // Rate: 10.0 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(10.0f));
        
        // Depth: 14% (old percentage) - For Purple, this means 0.14 actual depth
        // Purple maps 0-1 to 0-0.45, so to get 0.14 actual: 0.14 / 0.45 = 0.311
        if (auto* param = parameters.getParameter(DEPTH_ID))
        {
            // Set to 0.311 so that after Purple mapping (0.311 * 0.45 = 0.14), we get 14% actual depth
            param->setValueNotifyingHost(0.311f);
        }
        
        // Offset: 50°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(50.0f));
        
        // Width: 50%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(0.5f));
        
        // Color: 10%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.10f);
        
        // Engine Color: Purple (3)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(3.0f));
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
        
        // Mix: 100%
        if (auto* param = parameters.getParameter(MIX_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 6) // Ouroboros preset
    {
        // Rate: 2.0 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(2.0f));
        
        // Depth: 11%
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(0.11f);
        
        // Offset: 33°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(33.0f));
        
        // Width: 33%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(0.33f));
        
        // Color: 65%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.65f);
        
        // Engine Color: Blue (1)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(ENGINE_COLOR_ID).convertTo0to1(1.0f));
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
        
        // Mix: 100%
        if (auto* param = parameters.getParameter(MIX_ID))
            param->setValueNotifyingHost(1.0f);
    }
    if (auto* p = parameters.getRawParameterValue(ENGINE_COLOR_ID))
        lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(p->load()));
    presetLoadInProgress = false;
}

const juce::String ChoroborosAudioProcessor::getProgramName (int index)
{
    if (index == 0)
        return "Classic (Green)";
    else if (index == 1)
        return "Vintage (Red)";
    else if (index == 2)
        return "Modern (Blue)";
    else if (index == 3)
        return "Psychedelic (Purple)";
    else if (index == 4)
        return "Core (Black)";
    else if (index == 5)
        return "Duck";
    else if (index == 6)
        return "Ouroboros";
    return {};
}

void ChoroborosAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ChoroborosAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    // Some hosts can deliver larger blocks than the initial samplesPerBlock.
    // Use a safety ceiling to avoid any chance of buffer underruns / asserts.
    constexpr juce::uint32 safetyMaxBlock = 4096; // bump to 8192 if you want ultra-safe
    spec.maximumBlockSize = juce::jmax(static_cast<juce::uint32>(samplesPerBlock), safetyMaxBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    chorusDSP->prepare(spec);
    constexpr int diagnosticBufferCeiling = 8192;
    const int diagnosticBufferSize = juce::jmax<int>(samplesPerBlock, diagnosticBufferCeiling);
    dryTapBuffer.setSize(2, diagnosticBufferSize, false, true, true);
    wetEstimateBuffer.setSize(2, diagnosticBufferSize, false, true, true);
    inputTapRing.clear();
    wetTapRing.clear();
    outputTapRing.clear();
    activeAnalyzerSnapshotIndex.store(0, std::memory_order_relaxed);
    analyzerSnapshots[0] = {};
    analyzerSnapshots[1] = {};
    analyzerSequenceCounter.store(0, std::memory_order_relaxed);

    if (analyzerWorker != nullptr && !analyzerWorker->isThreadRunning())
        analyzerWorker->startThread(juce::Thread::Priority::low);

    startTimerHz(10);  // Apply runtime tuning on message thread, 10 Hz
}

void ChoroborosAudioProcessor::releaseResources()
{
    stopTimer();
    if (analyzerWorker != nullptr)
        analyzerWorker->stopThread(1500);
    chorusDSP->reset();
}

void ChoroborosAudioProcessor::timerCallback()
{
    if (dspLock.tryEnter())
    {
        if (chorusDSP)
            chorusDSP->applyRuntimeTuning();
        dspLock.exit();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChoroborosAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void ChoroborosAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    const int tapSamples = juce::jmin(numSamples, dryTapBuffer.getNumSamples());
    const bool analyzerEnabled = diagnosticFeatureFlags.analyzersEnabled.load(std::memory_order_relaxed);
    const bool needAnalyzerAudioTaps = analyzerEnabled
        && (diagnosticFeatureFlags.spectrumCardEnabled.load(std::memory_order_relaxed)
            || diagnosticFeatureFlags.transferCardEnabled.load(std::memory_order_relaxed)
            || diagnosticFeatureFlags.telemetryCardEnabled.load(std::memory_order_relaxed));

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (needAnalyzerAudioTaps && tapSamples > 0)
    {
        const float* inL = (totalNumInputChannels > 0) ? buffer.getReadPointer(0) : nullptr;
        const float* inR = (totalNumInputChannels > 1) ? buffer.getReadPointer(1) : inL;
        auto* dryL = dryTapBuffer.getWritePointer(0);
        auto* dryR = dryTapBuffer.getWritePointer(1);
        for (int i = 0; i < tapSamples; ++i)
        {
            const float sampleL = (inL != nullptr) ? inL[i] : 0.0f;
            const float sampleR = (inR != nullptr) ? inR[i] : sampleL;
            dryL[i] = sampleL;
            dryR[i] = sampleR;
        }
        inputTapRing.push(dryL, dryR, tapSamples);
    }

    const float inputPeakL = (totalNumInputChannels > 0) ? buffer.getMagnitude(0, 0, numSamples) : 0.0f;
    const float inputPeakR = (totalNumInputChannels > 1) ? buffer.getMagnitude(1, 0, numSamples) : inputPeakL;

    const auto startTicks = juce::Time::getHighResolutionTicks();
    updateDSPParameters();

    {
        juce::ScopedLock sl(dspLock);
        juce::dsp::AudioBlock<float> block(buffer);
        chorusDSP->process(block);
    }

    const auto elapsedTicks = juce::Time::getHighResolutionTicks() - startTicks;
    const float processMs = static_cast<float>(juce::Time::highResolutionTicksToSeconds(elapsedTicks) * 1000.0);
    const float outputPeakL = (totalNumOutputChannels > 0) ? buffer.getMagnitude(0, 0, numSamples) : 0.0f;
    const float outputPeakR = (totalNumOutputChannels > 1) ? buffer.getMagnitude(1, 0, numSamples) : outputPeakL;

    if (needAnalyzerAudioTaps && tapSamples > 0)
    {
        const float* outL = (totalNumOutputChannels > 0) ? buffer.getReadPointer(0) : nullptr;
        const float* outR = (totalNumOutputChannels > 1) ? buffer.getReadPointer(1) : outL;
        outputTapRing.push(outL, outR, tapSamples);

        float mixMapped = 0.5f;
        if (const auto* mixParam = parameters.getRawParameterValue(MIX_ID))
            mixMapped = juce::jlimit(0.0f, 1.0f, mapParameterValue(MIX_ID, mixParam->load()));

        auto* wetL = wetEstimateBuffer.getWritePointer(0);
        auto* wetR = wetEstimateBuffer.getWritePointer(1);
        const float* dryL = dryTapBuffer.getReadPointer(0);
        const float* dryR = dryTapBuffer.getReadPointer(1);
        const float invMix = (mixMapped > 1.0e-5f) ? (1.0f / mixMapped) : 0.0f;
        const float dryScale = 1.0f - mixMapped;
        for (int i = 0; i < tapSamples; ++i)
        {
            if (mixMapped <= 1.0e-5f)
            {
                wetL[i] = 0.0f;
                wetR[i] = 0.0f;
                continue;
            }

            const float outSampleL = (outL != nullptr) ? outL[i] : 0.0f;
            const float outSampleR = (outR != nullptr) ? outR[i] : outSampleL;
            wetL[i] = juce::jlimit(-2.0f, 2.0f, (outSampleL - dryL[i] * dryScale) * invMix);
            wetR[i] = juce::jlimit(-2.0f, 2.0f, (outSampleR - dryR[i] * dryScale) * invMix);
        }
        wetTapRing.push(wetL, wetR, tapSamples);
    }

    auto updatePeakHold = [](std::atomic<float>& target, float measured)
    {
        const float previous = target.load(std::memory_order_relaxed);
        const float held = juce::jmax(measured, previous * 0.96f);
        target.store(held, std::memory_order_relaxed);
    };

    updatePeakHold(liveTelemetry.inputPeakL, inputPeakL);
    updatePeakHold(liveTelemetry.inputPeakR, inputPeakR);
    updatePeakHold(liveTelemetry.outputPeakL, outputPeakL);
    updatePeakHold(liveTelemetry.outputPeakR, outputPeakR);
    liveTelemetry.lastProcessMs.store(processMs, std::memory_order_relaxed);

    float prevMax = liveTelemetry.maxProcessMs.load(std::memory_order_relaxed);
    while (processMs > prevMax
        && !liveTelemetry.maxProcessMs.compare_exchange_weak(prevMax, processMs, std::memory_order_relaxed))
    {
    }

    liveTelemetry.processBlockCount.fetch_add(1, std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::initTuningDefaults()
{
    auto initParam = [](ParamTuning& tuningParam, float minValue, float maxValue)
    {
        tuningParam.min.store(minValue);
        tuningParam.max.store(maxValue);
        tuningParam.curve.store(1.0f);
        tuningParam.uiSkew.store(1.0f);
    };

    initParam(tuning.rate, RATE_MIN, RATE_MAX);
    initParam(tuning.depth, DEPTH_MIN, DEPTH_MAX);
    initParam(tuning.offset, OFFSET_MIN, OFFSET_MAX);
    initParam(tuning.width, WIDTH_MIN, WIDTH_MAX);
    initParam(tuning.color, COLOR_MIN, COLOR_MAX);
    initParam(tuning.mix, MIX_MIN, MIX_MAX);
}

float ChoroborosAudioProcessor::mapTunedValue(float rawValue, float baseMin, float baseMax, const ParamTuning& tuningParam) const
{
    if (baseMax <= baseMin)
        return rawValue;

    const float normalised = juce::jlimit(0.0f, 1.0f, (rawValue - baseMin) / (baseMax - baseMin));
    const float curve = juce::jmax(0.001f, tuningParam.curve.load());
    const float shaped = std::pow(normalised, curve);

    float tunedMin = tuningParam.min.load();
    float tunedMax = tuningParam.max.load();
    if (tunedMax < tunedMin)
        std::swap(tunedMax, tunedMin);

    return tunedMin + (tunedMax - tunedMin) * shaped;
}

float ChoroborosAudioProcessor::unmapTunedValue(float mappedValue, float baseMin, float baseMax, const ParamTuning& tuningParam) const
{
    if (baseMax <= baseMin)
        return mappedValue;

    float tunedMin = tuningParam.min.load();
    float tunedMax = tuningParam.max.load();
    if (tunedMax < tunedMin)
        std::swap(tunedMax, tunedMin);

    const float tunedSpan = tunedMax - tunedMin;
    if (tunedSpan <= 1.0e-6f)
        return baseMin;

    const float shaped = juce::jlimit(0.0f, 1.0f, (mappedValue - tunedMin) / tunedSpan);
    const float curve = juce::jmax(0.001f, tuningParam.curve.load());
    const float normalised = std::pow(shaped, 1.0f / curve);
    return baseMin + (baseMax - baseMin) * normalised;
}

float ChoroborosAudioProcessor::mapParameterValue(const juce::String& paramId, float rawValue) const
{
    if (paramId == RATE_ID)
        return mapTunedValue(rawValue, RATE_MIN, RATE_MAX, tuning.rate);
    if (paramId == DEPTH_ID)
        return mapTunedValue(rawValue, DEPTH_MIN, DEPTH_MAX, tuning.depth);
    if (paramId == OFFSET_ID)
        return mapTunedValue(rawValue, OFFSET_MIN, OFFSET_MAX, tuning.offset);
    if (paramId == WIDTH_ID)
        return mapTunedValue(rawValue, WIDTH_MIN, WIDTH_MAX, tuning.width);
    if (paramId == COLOR_ID)
        return mapTunedValue(rawValue, COLOR_MIN, COLOR_MAX, tuning.color);
    if (paramId == MIX_ID)
        return mapTunedValue(rawValue, MIX_MIN, MIX_MAX, tuning.mix);

    return rawValue;
}

float ChoroborosAudioProcessor::unmapParameterValue(const juce::String& paramId, float mappedValue) const
{
    if (paramId == RATE_ID)
        return unmapTunedValue(mappedValue, RATE_MIN, RATE_MAX, tuning.rate);
    if (paramId == DEPTH_ID)
        return unmapTunedValue(mappedValue, DEPTH_MIN, DEPTH_MAX, tuning.depth);
    if (paramId == OFFSET_ID)
        return unmapTunedValue(mappedValue, OFFSET_MIN, OFFSET_MAX, tuning.offset);
    if (paramId == WIDTH_ID)
        return unmapTunedValue(mappedValue, WIDTH_MIN, WIDTH_MAX, tuning.width);
    if (paramId == COLOR_ID)
        return unmapTunedValue(mappedValue, COLOR_MIN, COLOR_MAX, tuning.color);
    if (paramId == MIX_ID)
        return unmapTunedValue(mappedValue, MIX_MIN, MIX_MAX, tuning.mix);

    return mappedValue;
}

void ChoroborosAudioProcessor::updateDSPParameters()
{
    // Avoid exposing partial/transactional APVTS states to DSP while a preset/state/profile
    // write is in progress on another thread. Apply once after the transaction completes.
    if (presetLoadInProgress || stateLoadInProgress || engineProfileApplyInProgress)
        return;

    auto rateParam = parameters.getRawParameterValue(RATE_ID);
    auto depthParam = parameters.getRawParameterValue(DEPTH_ID);
    auto offsetParam = parameters.getRawParameterValue(OFFSET_ID);
    auto widthParam = parameters.getRawParameterValue(WIDTH_ID);
    auto colorParam = parameters.getRawParameterValue(COLOR_ID);
    auto engineColorParam = parameters.getRawParameterValue(ENGINE_COLOR_ID);
    auto hqParam = parameters.getRawParameterValue(HQ_ID);
    
    if (rateParam) chorusDSP->setRate(mapParameterValue(RATE_ID, rateParam->load()));
    if (depthParam) chorusDSP->setDepth(mapParameterValue(DEPTH_ID, depthParam->load()));
    if (offsetParam) chorusDSP->setOffset(mapParameterValue(OFFSET_ID, offsetParam->load()));
    if (widthParam) chorusDSP->setWidth(mapParameterValue(WIDTH_ID, widthParam->load()));
    if (colorParam) chorusDSP->setColor(mapParameterValue(COLOR_ID, colorParam->load()));
    
    const int colorIndex = engineColorParam ? juce::jlimit(0, 4, static_cast<int>(engineColorParam->load()))
                                            : getCurrentEngineColorIndex();
    const bool hqEnabled = hqParam ? (hqParam->load() >= 0.5f) : false;

    if (colorIndex != activeInternalsEngine || hqEnabled != activeInternalsHQ)
    {
        persistActiveEngineInternalsFromDsp();
        restoreEngineInternalsToDsp(colorIndex, hqEnabled);
        activeInternalsEngine = colorIndex;
        activeInternalsHQ = hqEnabled;
        startTimer(0);
    }

    chorusDSP->setEngineColor(colorIndex);
    chorusDSP->setQualityEnabled(hqEnabled);
    
    auto mixParam = parameters.getRawParameterValue(MIX_ID);
    if (mixParam) chorusDSP->setMix(mapParameterValue(MIX_ID, mixParam->load()));
}

ChorusDSP::RuntimeTuning& ChoroborosAudioProcessor::getEngineDspInternals(int colorIndex, bool hqEnabled)
{
    const int clampedColor = juce::jlimit(0, 4, colorIndex);
    return engineInternals[clampedColor][hqEnabled ? 1 : 0];
}

const ChorusDSP::RuntimeTuning& ChoroborosAudioProcessor::getEngineDspInternals(int colorIndex, bool hqEnabled) const
{
    const int clampedColor = juce::jlimit(0, 4, colorIndex);
    return engineInternals[clampedColor][hqEnabled ? 1 : 0];
}

void ChoroborosAudioProcessor::setModularCoresEnabled(bool enabled)
{
    if (modularCoresEnabled == enabled)
        return;

    modularCoresEnabled = enabled;
    if (chorusDSP != nullptr)
        chorusDSP->setModularCoreModeEnabled(modularCoresEnabled);
}

void ChoroborosAudioProcessor::setCoreAssignments(const choroboros::CoreAssignmentTable& assignments)
{
    coreAssignments = assignments;
    if (chorusDSP != nullptr)
        chorusDSP->setCoreAssignments(coreAssignments);
}

bool ChoroborosAudioProcessor::setCoreAssignment(int colorIndex, bool hqEnabled, choroboros::CoreId coreId)
{
    const int safeEngine = juce::jlimit(0, 4, colorIndex);
    const std::size_t coreIndex = static_cast<std::size_t>(coreId);
    const choroboros::CoreId safeCore = coreIndex < choroboros::coreIdCount()
        ? coreId
        : coreAssignments.get(safeEngine, hqEnabled);

    const bool duplicate = choroboros::assignmentIsDuplicate(coreAssignments, safeEngine, hqEnabled, safeCore);
    coreAssignments.set(safeEngine, hqEnabled, safeCore);
    if (chorusDSP != nullptr)
        chorusDSP->setCoreAssignment(safeEngine, hqEnabled, safeCore);
    return duplicate;
}

std::vector<choroboros::SlotAssignment> ChoroborosAudioProcessor::getDuplicateAssignmentWarnings() const
{
    if (chorusDSP != nullptr)
        return chorusDSP->getDuplicateAssignmentWarnings();

    std::vector<choroboros::SlotAssignment> warnings;
    for (std::size_t i = 0; i < choroboros::coreIdCount(); ++i)
    {
        const auto coreId = static_cast<choroboros::CoreId>(i);
        const auto assignments = choroboros::findAssignmentsForCore(coreAssignments, coreId);
        if (assignments.size() > 1)
            warnings.insert(warnings.end(), assignments.begin(), assignments.end());
    }
    return warnings;
}

int ChoroborosAudioProcessor::getCurrentEngineColorIndex() const
{
    if (auto* engineColorParam = parameters.getRawParameterValue(ENGINE_COLOR_ID))
        return juce::jlimit(0, 4, static_cast<int>(engineColorParam->load()));
    return 0;
}

bool ChoroborosAudioProcessor::isHqEnabled() const
{
    if (auto* hqParam = parameters.getRawParameterValue(HQ_ID))
        return hqParam->load() >= 0.5f;
    return false;
}

void ChoroborosAudioProcessor::syncEngineInternalsToActiveDsp(int colorIndex, bool hqEnabled)
{
    const int clamped = juce::jlimit(0, 4, colorIndex);
    if (clamped == activeInternalsEngine && hqEnabled == activeInternalsHQ)
        copyRuntimeTuningValues(engineInternals[clamped][hqEnabled ? 1 : 0], chorusDSP->getRuntimeTuning());
}

void ChoroborosAudioProcessor::initializeEngineInternalProfiles()
{
    for (int color = 0; color < 5; ++color)
    {
        copyRuntimeTuningValues(chorusDSP->getRuntimeTuning(), engineInternals[color][0]);
        copyRuntimeTuningValues(chorusDSP->getRuntimeTuning(), engineInternals[color][1]);
    }
    setRedNQDefaults(engineInternals[2][0]);
    setRedHQDefaults(engineInternals[2][1]);
    activeInternalsEngine = getCurrentEngineColorIndex();
    activeInternalsHQ = isHqEnabled();
    restoreEngineInternalsToDsp(activeInternalsEngine, activeInternalsHQ);
}

void ChoroborosAudioProcessor::persistActiveEngineInternalsFromDsp()
{
    const int clampedColor = juce::jlimit(0, 4, activeInternalsEngine);
    copyRuntimeTuningValues(chorusDSP->getRuntimeTuning(), engineInternals[clampedColor][activeInternalsHQ ? 1 : 0]);
}

void ChoroborosAudioProcessor::restoreEngineInternalsToDsp(int colorIndex, bool hqEnabled)
{
    copyRuntimeTuningValues(engineInternals[juce::jlimit(0, 4, colorIndex)][hqEnabled ? 1 : 0], chorusDSP->getRuntimeTuning());
}

//==============================================================================
bool ChoroborosAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ChoroborosAudioProcessor::createEditor()
{
    const double editorCreateStartMs = juce::Time::getMillisecondCounterHiRes();
    auto* editor = new ChoroborosPluginEditor(*this);
    logLoadTraceEvent("processor_create_editor_ms",
                      juce::Time::getMillisecondCounterHiRes() - editorCreateStartMs);
    return editor;
}

//==============================================================================
void ChoroborosAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty("modularCoresEnabled", modularCoresEnabled, nullptr);
    state.setProperty("coreAssignmentsJson", encodeCoreAssignmentsForStateProperty(coreAssignments), nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChoroborosAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const double stateLoadStartMs = juce::Time::getMillisecondCounterHiRes();
    bool stateApplied = false;
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            stateLoadInProgress = true;
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            if (auto* p = parameters.getRawParameterValue(ENGINE_COLOR_ID))
                lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(p->load()));

            bool restoredModular = false;
            if (parameters.state.hasProperty("modularCoresEnabled"))
            {
                const auto value = parameters.state.getProperty("modularCoresEnabled");
                if (value.isBool())
                    restoredModular = static_cast<bool>(value);
                else if (value.isInt() || value.isInt64() || value.isDouble())
                    restoredModular = static_cast<double>(value) >= 0.5;
                else
                    restoredModular = value.toString().equalsIgnoreCase("true") || value.toString() == "1";
            }
            setModularCoresEnabled(restoredModular);

            choroboros::CoreAssignmentTable restoredAssignments;
            restoredAssignments.resetToLegacy();
            bool loadedAssignments = false;
            if (parameters.state.hasProperty("coreAssignmentsJson"))
            {
                loadedAssignments = decodeCoreAssignmentsFromStateProperty(
                    parameters.state.getProperty("coreAssignmentsJson").toString(),
                    restoredAssignments);
            }
            if (parameters.state.hasProperty("coreAssignments"))
            {
                loadedAssignments = parseCoreAssignmentsFromVar(
                    parameters.state.getProperty("coreAssignments"),
                    restoredAssignments) || loadedAssignments;
            }
            if (!loadedAssignments)
                restoredAssignments.resetToLegacy();
            setCoreAssignments(restoredAssignments);

            stateLoadInProgress = false;
            stateApplied = true;
        }
    }

    logLoadTraceEvent("processor_set_state_information_ms",
                      juce::Time::getMillisecondCounterHiRes() - stateLoadStartMs,
                      juce::String("bytes=") + juce::String(sizeInBytes)
                          + ",applied=" + (stateApplied ? "1" : "0"));
    if (stateApplied)
    {
        logLoadTraceEvent("processor_post_state_params",
                          0.0,
                          buildStartupParamSnapshotNotes(*this, "source=host_state"));
    }
}

void ChoroborosAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (presetLoadInProgress || stateLoadInProgress || engineProfileApplyInProgress)
    {
        if (parameterID == ENGINE_COLOR_ID)
            lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(newValue));
        return;
    }

    if (parameterID == ENGINE_COLOR_ID)
    {
        liveTelemetry.parameterWriteCount.fetch_add(1, std::memory_order_relaxed);
        liveTelemetry.engineSwitchCount.fetch_add(1, std::memory_order_relaxed);
        struct ScopedFlag
        {
            std::atomic<bool>& flagRef;
            explicit ScopedFlag(std::atomic<bool>& flag) : flagRef(flag) { flagRef.store(true); }
            ~ScopedFlag() { flagRef.store(false); }
        } scopedApplyFlag(engineProfileApplyInProgress);

        const int newEngine = juce::jlimit(0, 4, static_cast<int>(newValue));
        saveCurrentParamsToEngineProfile(lastEngineIndex);
        lastEngineIndex = newEngine;
        applyEngineParamProfile(newEngine);
        return;
    }

    if (parameterID == HQ_ID)
    {
        liveTelemetry.parameterWriteCount.fetch_add(1, std::memory_order_relaxed);
        liveTelemetry.hqToggleCount.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    if (parameterID == RATE_ID
        || parameterID == DEPTH_ID
        || parameterID == OFFSET_ID
        || parameterID == WIDTH_ID
        || parameterID == COLOR_ID
        || parameterID == MIX_ID)
    {
        liveTelemetry.parameterWriteCount.fetch_add(1, std::memory_order_relaxed);
        saveCurrentParamsToEngineProfile(getCurrentEngineColorIndex());
    }
}

void ChoroborosAudioProcessor::resetLiveTelemetryPeakHold()
{
    liveTelemetry.inputPeakL.store(0.0f, std::memory_order_relaxed);
    liveTelemetry.inputPeakR.store(0.0f, std::memory_order_relaxed);
    liveTelemetry.outputPeakL.store(0.0f, std::memory_order_relaxed);
    liveTelemetry.outputPeakR.store(0.0f, std::memory_order_relaxed);
    liveTelemetry.maxProcessMs.store(0.0f, std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::resetToFactoryDefaults()
{
    stateLoadInProgress.store(true, std::memory_order_relaxed);

    initTuningDefaults();
    initializeEngineInternalProfiles();

    for (int i = 0; i < 5; ++i)
        engineParamProfiles[static_cast<size_t>(i)] = getEngineDefaults(i);

    coreAssignments.resetToLegacy();
    modularCoresEnabled = false;
    if (chorusDSP != nullptr)
    {
        chorusDSP->setCoreAssignments(coreAssignments);
        chorusDSP->setModularCoreModeEnabled(modularCoresEnabled);
    }

    const auto setToDefault = [this](const juce::String& parameterId)
    {
        if (auto* param = parameters.getParameter(parameterId))
            param->setValueNotifyingHost(param->getDefaultValue());
    };

    setToDefault(ENGINE_COLOR_ID);
    setToDefault(HQ_ID);
    setToDefault(RATE_ID);
    setToDefault(DEPTH_ID);
    setToDefault(OFFSET_ID);
    setToDefault(WIDTH_ID);
    setToDefault(COLOR_ID);
    setToDefault(MIX_ID);

    if (auto* p = parameters.getRawParameterValue(ENGINE_COLOR_ID))
        lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(p->load()));
    saveCurrentParamsToEngineProfile(lastEngineIndex);

    syncEngineInternalsToActiveDsp(getCurrentEngineColorIndex(), isHqEnabled());
    stateLoadInProgress.store(false, std::memory_order_relaxed);

    updateDSPParameters();
    resetLiveTelemetryPeakHold();
}

bool ChoroborosAudioProcessor::getAnalyzerSnapshot(AnalyzerSnapshot& outSnapshot) const
{
    const int idx = activeAnalyzerSnapshotIndex.load(std::memory_order_acquire);
    outSnapshot = analyzerSnapshots[juce::jlimit(0, 1, idx)];
    return outSnapshot.valid;
}

void ChoroborosAudioProcessor::setAnalyzerFrozen(bool shouldFreeze)
{
    analyzerRuntimeConfig.freeze.store(shouldFreeze, std::memory_order_relaxed);
}

bool ChoroborosAudioProcessor::isAnalyzerFrozen() const
{
    return analyzerRuntimeConfig.freeze.load(std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::setAnalyzerPeakHoldEnabled(bool shouldHold)
{
    analyzerRuntimeConfig.peakHold.store(shouldHold, std::memory_order_relaxed);
}

bool ChoroborosAudioProcessor::isAnalyzerPeakHoldEnabled() const
{
    return analyzerRuntimeConfig.peakHold.load(std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::setAnalyzerRefreshHz(int hz)
{
    analyzerRuntimeConfig.refreshHz.store(juce::jlimit(5, 60, hz), std::memory_order_relaxed);
}

int ChoroborosAudioProcessor::getAnalyzerRefreshHz() const
{
    return juce::jlimit(5, 60, analyzerRuntimeConfig.refreshHz.load(std::memory_order_relaxed));
}

void ChoroborosAudioProcessor::setAnalyzerCardDemand(bool modulationVisible, bool spectrumVisible,
                                                     bool transferVisible, bool telemetryVisible)
{
    diagnosticFeatureFlags.modulationCardEnabled.store(modulationVisible, std::memory_order_relaxed);
    diagnosticFeatureFlags.spectrumCardEnabled.store(spectrumVisible, std::memory_order_relaxed);
    diagnosticFeatureFlags.transferCardEnabled.store(transferVisible, std::memory_order_relaxed);
    diagnosticFeatureFlags.telemetryCardEnabled.store(telemetryVisible, std::memory_order_relaxed);
}

void ChoroborosAudioProcessor::runAnalyzerPass()
{
    if (analyzerRuntimeConfig.freeze.load(std::memory_order_relaxed))
        return;

    constexpr int fftSize = ANALYZER_FFT_SIZE;
    constexpr int bins = ANALYZER_FFT_SIZE / 2;
    constexpr int waveformPoints = ANALYZER_WAVEFORM_POINTS;
    constexpr int transferPoints = ANALYZER_TRANSFER_POINTS;

    const bool needModulation = diagnosticFeatureFlags.modulationCardEnabled.load(std::memory_order_relaxed);
    const bool needSpectrum = diagnosticFeatureFlags.spectrumCardEnabled.load(std::memory_order_relaxed);
    const bool needTransfer = diagnosticFeatureFlags.transferCardEnabled.load(std::memory_order_relaxed);
    const bool needTelemetry = diagnosticFeatureFlags.telemetryCardEnabled.load(std::memory_order_relaxed);

    if (!(needModulation || needSpectrum || needTransfer || needTelemetry))
        return;

    static thread_local juce::dsp::FFT fft(ANALYZER_FFT_ORDER);
    static thread_local juce::dsp::WindowingFunction<float> window(fftSize, juce::dsp::WindowingFunction<float>::hann, true);
    static thread_local std::array<float, fftSize * 2> fftData {};

    const int currentFront = activeAnalyzerSnapshotIndex.load(std::memory_order_acquire);
    const AnalyzerSnapshot previous = analyzerSnapshots[juce::jlimit(0, 1, currentFront)];
    const bool peakHoldEnabled = analyzerRuntimeConfig.peakHold.load(std::memory_order_relaxed);
    const bool needAudioTaps = needSpectrum || needTransfer || needTelemetry;

    std::array<float, fftSize> inputL {};
    std::array<float, fftSize> inputR {};
    std::array<float, fftSize> wetL {};
    std::array<float, fftSize> wetR {};
    std::array<float, fftSize> outputL {};
    std::array<float, fftSize> outputR {};

    if (needAudioTaps)
    {
        inputTapRing.copyLatest(inputL.data(), inputR.data(), fftSize);
        wetTapRing.copyLatest(wetL.data(), wetR.data(), fftSize);
        outputTapRing.copyLatest(outputL.data(), outputR.data(), fftSize);
    }

    auto computeSpectrum = [&](const std::array<float, fftSize>& left,
                               const std::array<float, fftSize>& right,
                               std::array<float, bins>& destination,
                               const std::array<float, bins>& previousSpectrum)
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[static_cast<size_t>(i)] = 0.5f * (left[static_cast<size_t>(i)] + right[static_cast<size_t>(i)]);
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int i = 0; i < bins; ++i)
        {
            const float gain = juce::jmax(1.0e-7f, fftData[static_cast<size_t>(i)] / static_cast<float>(fftSize));
            const float db = juce::Decibels::gainToDecibels(gain, -100.0f);
            float normalized = juce::jlimit(0.0f, 1.0f, (db + 100.0f) * 0.01f);
            if (peakHoldEnabled && previous.valid)
                normalized = juce::jmax(normalized, previousSpectrum[static_cast<size_t>(i)] * 0.985f);
            destination[static_cast<size_t>(i)] = normalized;
        }
    };

    auto fillWaveform = [fftSize, waveformPoints](const std::array<float, fftSize>& left,
                                                  const std::array<float, fftSize>& right,
                                                  std::array<float, waveformPoints>& destination)
    {
        for (int i = 0; i < waveformPoints; ++i)
        {
            const int src = (i * (fftSize - 1)) / (waveformPoints - 1);
            destination[static_cast<size_t>(i)] = 0.5f * (left[static_cast<size_t>(src)] + right[static_cast<size_t>(src)]);
        }
    };

    auto computePeakDb = [fftSize](const std::array<float, fftSize>& left, const std::array<float, fftSize>& right)
    {
        float peak = 0.0f;
        for (int i = 0; i < fftSize; ++i)
        {
            peak = juce::jmax(peak, std::abs(left[static_cast<size_t>(i)]));
            peak = juce::jmax(peak, std::abs(right[static_cast<size_t>(i)]));
        }
        return juce::Decibels::gainToDecibels(juce::jmax(peak, 1.0e-6f));
    };

    const int nextIndex = 1 - currentFront;
    auto& snapshot = analyzerSnapshots[nextIndex];
    snapshot = previous;
    snapshot.valid = true;
    snapshot.sequence = analyzerSequenceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
    snapshot.sampleRate = getSampleRate();
    snapshot.blockSize = getBlockSize();

    if (needAudioTaps)
    {
        fillWaveform(inputL, inputR, snapshot.inputWaveform);
        fillWaveform(wetL, wetR, snapshot.wetWaveform);
        fillWaveform(outputL, outputR, snapshot.outputWaveform);
    }

    if (needSpectrum)
    {
        computeSpectrum(inputL, inputR, snapshot.inputSpectrum, previous.inputSpectrum);
        computeSpectrum(wetL, wetR, snapshot.wetSpectrum, previous.wetSpectrum);
        computeSpectrum(outputL, outputR, snapshot.outputSpectrum, previous.outputSpectrum);
    }

    if (needTelemetry)
    {
        snapshot.inputPeakDb = computePeakDb(inputL, inputR);
        snapshot.wetPeakDb = computePeakDb(wetL, wetR);
        snapshot.outputPeakDb = computePeakDb(outputL, outputR);
    }

    auto readRaw = [this](const char* paramId) -> float
    {
        if (const auto* param = parameters.getRawParameterValue(paramId))
            return param->load();
        return 0.0f;
    };

    const float rateMapped = mapParameterValue(RATE_ID, readRaw(RATE_ID));
    const float depthMapped = mapParameterValue(DEPTH_ID, readRaw(DEPTH_ID));
    const float offsetDeg = mapParameterValue(OFFSET_ID, readRaw(OFFSET_ID));
    const float offsetRad = juce::degreesToRadians(offsetDeg);
    const auto& rt = getDspInternals();
    const bool isRedNQ = getCurrentEngineColorIndex() == 2 && !isHqEnabled();

    float centerDelayMs = rt.centreDelayBaseMs.load() + rt.centreDelayScale.load() * depthMapped;
    float modulationDepthMs = juce::jmax(0.02f, rt.centreDelayScale.load() * depthMapped * 0.25f);
    float bbdMinMs = 0.0f;
    float bbdMaxMs = 0.0f;
    if (isRedNQ)
    {
        centerDelayMs = rt.bbdCentreBaseMs.load() + rt.bbdCentreScale.load() * depthMapped;
        modulationDepthMs = juce::jmax(0.02f, rt.bbdDepthMs.load() * depthMapped);
        bbdMinMs = rt.bbdDelayMinMs.load();
        bbdMaxMs = rt.bbdDelayMaxMs.load();
    }

    if (needModulation || needTelemetry)
    {
        snapshot.centerDelayMs = centerDelayMs;
        snapshot.modulationDepthMs = modulationDepthMs;
    }

    if (needModulation)
    {
        const float clampedRate = juce::jlimit(0.05f, 10.0f, rateMapped);
        const float phaseScale = juce::MathConstants<float>::twoPi * clampedRate;
        for (int i = 0; i < waveformPoints; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(waveformPoints - 1);
            const float phase = phaseScale * t;
            const float lfoL = std::sin(phase) * depthMapped;
            const float lfoR = std::sin(phase + offsetRad) * depthMapped;
            snapshot.lfoLeft[static_cast<size_t>(i)] = lfoL;
            snapshot.lfoRight[static_cast<size_t>(i)] = lfoR;

            float delayMs = centerDelayMs + lfoL * modulationDepthMs;
            if (isRedNQ)
                delayMs = juce::jlimit(bbdMinMs, bbdMaxMs, delayMs);
            snapshot.delayTrajectoryMs[static_cast<size_t>(i)] = delayMs;
        }
    }

    if (needTransfer)
    {
        std::array<float, transferPoints> transferSum {};
        std::array<int, transferPoints> transferCount {};
        for (int i = 0; i < fftSize; ++i)
        {
            const float x = juce::jlimit(-1.0f, 1.0f, 0.5f * (inputL[static_cast<size_t>(i)] + inputR[static_cast<size_t>(i)]));
            const float y = juce::jlimit(-1.0f, 1.0f, 0.5f * (outputL[static_cast<size_t>(i)] + outputR[static_cast<size_t>(i)]));
            const float xNorm = 0.5f * (x + 1.0f);
            const int bin = juce::jlimit(0, transferPoints - 1, static_cast<int>(std::round(xNorm * static_cast<float>(transferPoints - 1))));
            transferSum[static_cast<size_t>(bin)] += y;
            transferCount[static_cast<size_t>(bin)] += 1;
        }

        for (int i = 0; i < transferPoints; ++i)
        {
            snapshot.transferInput[static_cast<size_t>(i)] =
                juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(transferPoints - 1), -1.0f, 1.0f);

            if (transferCount[static_cast<size_t>(i)] > 0)
            {
                snapshot.transferOutput[static_cast<size_t>(i)] =
                    transferSum[static_cast<size_t>(i)] / static_cast<float>(transferCount[static_cast<size_t>(i)]);
            }
            else if (previous.valid)
            {
                snapshot.transferOutput[static_cast<size_t>(i)] = previous.transferOutput[static_cast<size_t>(i)] * 0.995f;
            }
            else
            {
                snapshot.transferOutput[static_cast<size_t>(i)] = snapshot.transferInput[static_cast<size_t>(i)];
            }
        }
    }

    activeAnalyzerSnapshotIndex.store(nextIndex, std::memory_order_release);
}

void ChoroborosAudioProcessor::saveCurrentParamsToEngineProfile(int engineIndex)
{
    if (engineIndex < 0 || engineIndex >= 5)
        return;
    auto& p = engineParamProfiles[engineIndex];
    if (auto* r = parameters.getRawParameterValue(RATE_ID))
        p.rate = r->load();
    if (auto* d = parameters.getRawParameterValue(DEPTH_ID))
        p.depth = d->load();
    if (auto* o = parameters.getRawParameterValue(OFFSET_ID))
        p.offset = o->load();
    if (auto* w = parameters.getRawParameterValue(WIDTH_ID))
        p.width = w->load();
    if (auto* m = parameters.getRawParameterValue(MIX_ID))
        p.mix = m->load();
    if (auto* c = parameters.getRawParameterValue(COLOR_ID))
        p.color = c->load();
    p.valid = true;
}

void ChoroborosAudioProcessor::applyEngineParamProfile(int engineIndex)
{
    if (engineIndex < 0 || engineIndex >= 5)
        return;

    const EngineParamProfile& prof = engineParamProfiles[engineIndex].valid
        ? engineParamProfiles[engineIndex]
        : getEngineDefaults(engineIndex);
    if (auto* param = parameters.getParameter(RATE_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(prof.rate));
    if (auto* param = parameters.getParameter(DEPTH_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(DEPTH_ID).convertTo0to1(prof.depth));
    if (auto* param = parameters.getParameter(OFFSET_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(prof.offset));
    if (auto* param = parameters.getParameter(WIDTH_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(prof.width));
    if (auto* param = parameters.getParameter(MIX_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(MIX_ID).convertTo0to1(prof.mix));
    if (auto* param = parameters.getParameter(COLOR_ID))
        param->setValueNotifyingHost(parameters.getParameterRange(COLOR_ID).convertTo0to1(prof.color));
}

void ChoroborosAudioProcessor::loadEngineParamProfilesFromVar(const juce::var& profilesVar)
{
    const auto* obj = profilesVar.getDynamicObject();
    if (obj == nullptr)
        return;
    auto getNum = [](const juce::var& v, const char* k, double def) -> float
    {
        if (const auto* o = v.getDynamicObject())
        {
            const auto val = o->getProperty(juce::Identifier(k));
            if (val.isDouble() || val.isInt() || val.isInt64())
                return static_cast<float>(static_cast<double>(val));
        }
        return static_cast<float>(def);
    };
    const char* keys[] = { "green", "blue", "red", "purple", "black" };
    for (int i = 0; i < 5; ++i)
    {
        const juce::var engVar = obj->getProperty(keys[i]);
        const auto* engObj = engVar.getDynamicObject();
        if (engObj == nullptr)
            continue;
        auto& p = engineParamProfiles[i];
        p.valid = static_cast<bool>(engObj->getProperty("valid"));
        p.rate = getNum(engVar, "rate", p.rate);
        p.depth = getNum(engVar, "depth", p.depth);
        p.offset = getNum(engVar, "offset", p.offset);
        p.width = getNum(engVar, "width", p.width);
        p.mix = getNum(engVar, "mix", p.mix);
        p.color = getNum(engVar, "color", p.color);
    }
}

EngineParamProfile ChoroborosAudioProcessor::getEngineDefaults(int engineIndex) const
{
    EngineParamProfile mappedDefaults;
    mappedDefaults.valid = true;
    switch (engineIndex)
    {
        case 0: mappedDefaults.rate = 1.2f;  mappedDefaults.depth = 0.21f; mappedDefaults.offset = 33.0f; mappedDefaults.width = 1.5f;  mappedDefaults.mix = 0.5f;  mappedDefaults.color = 0.16f; break;
        case 1: mappedDefaults.rate = 0.26f; mappedDefaults.depth = 0.53f; mappedDefaults.offset = 59.0f; mappedDefaults.width = 1.0f;  mappedDefaults.mix = 0.5f;  mappedDefaults.color = 0.41f; break;
        case 2: mappedDefaults.rate = 0.62f; mappedDefaults.depth = 0.21f; mappedDefaults.offset = 56.0f; mappedDefaults.width = 1.5f;  mappedDefaults.mix = 0.5f;  mappedDefaults.color = 0.5f;  break;
        case 3: mappedDefaults.rate = 0.12f; mappedDefaults.depth = 0.52f; mappedDefaults.offset = 52.0f; mappedDefaults.width = 2.0f;  mappedDefaults.mix = 0.69f; mappedDefaults.color = 0.13f; break;
        case 4: mappedDefaults.rate = 1.2f;  mappedDefaults.depth = 0.35f; mappedDefaults.offset = 41.0f; mappedDefaults.width = 1.59f; mappedDefaults.mix = 0.5f;  mappedDefaults.color = 0.28f; break;
        default: mappedDefaults.rate = 0.5f; mappedDefaults.depth = 0.5f; mappedDefaults.offset = 90.0f; mappedDefaults.width = 1.0f; mappedDefaults.mix = 0.5f; mappedDefaults.color = 0.5f;
    }

    EngineParamProfile rawDefaults;
    rawDefaults.valid = true;
    rawDefaults.rate = unmapParameterValue(RATE_ID, mappedDefaults.rate);
    rawDefaults.depth = unmapParameterValue(DEPTH_ID, mappedDefaults.depth);
    rawDefaults.offset = unmapParameterValue(OFFSET_ID, mappedDefaults.offset);
    rawDefaults.width = unmapParameterValue(WIDTH_ID, mappedDefaults.width);
    rawDefaults.mix = unmapParameterValue(MIX_ID, mappedDefaults.mix);
    rawDefaults.color = unmapParameterValue(COLOR_ID, mappedDefaults.color);
    return rawDefaults;
}

juce::AudioProcessorValueTreeState::ParameterLayout ChoroborosAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Rate: 0.01 Hz → 10.0 Hz
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), // Skewed for more resolution at low end
        0.5f  // Default: 0.5 Hz
    ));
    
    // Depth: 0% → 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.7f), // Slightly non-linear
        0.5f
    ));
    
    // Offset: 0° → 180°
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        OFFSET_ID, "Offset",
        juce::NormalisableRange<float>(0.0f, 180.0f, 1.0f),
        90.0f
    ));
    
    // Width: 0% → 200%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));
    
    // Color: 0% → 100% (engine-specific character control)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        COLOR_ID, "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));
    
    // Engine Color: 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    params.push_back(std::make_unique<MetaEngineChoiceParameter>(
        ENGINE_COLOR_ID, "Engine Color",
        juce::StringArray { "Green", "Blue", "Red", "Purple", "Black" },
        0 // Default: Green
    ));
    
    // HQ: High quality mode (false=Normal, true=HQ)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        HQ_ID, "HQ", false
    ));
    
    // Mix: Dry/wet mix (0.0 = dry, 1.0 = wet)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: 50% wet
    ));
    
    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChoroborosAudioProcessor();
}
