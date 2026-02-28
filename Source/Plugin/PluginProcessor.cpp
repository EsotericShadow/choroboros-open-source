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
    r.lpfCutoffHz.store(12956.0f);
    r.lpfQ.store(1.28f);
    r.preEmphasisFreqHz.store(1502.0f);
    r.preEmphasisQ.store(1.706f);
    r.preEmphasisGain.store(1.343f);
    r.preEmphasisLevelSmoothing.store(0.866f);
    r.preEmphasisQuietThreshold.store(0.535f);
    r.preEmphasisMaxAmount.store(0.961f);
    r.compressorAttackMs.store(19.4f);
    r.compressorReleaseMs.store(141.9f);
    r.compressorThresholdDb.store(-18.0f);
    r.compressorRatio.store(3.82f);
    r.saturationDriveScale.store(1.93f);
    r.bbdDelaySmoothingMs.store(52.2f);
    r.bbdDelayMinMs.store(5.9f);
    r.bbdDelayMaxMs.store(86.9f);
    r.bbdCentreBaseMs.store(14.7f);
    r.bbdCentreScale.store(3.28f);
    r.bbdDepthMs.store(13.2f);
    r.bbdClockSmoothingMs.store(41.7f);
    r.bbdFilterSmoothingMs.store(46.7f);
    r.bbdFilterCutoffMinHz.store(469.0f);
    r.bbdFilterCutoffMaxHz.store(12176.0f);
    r.bbdFilterCutoffScale.store(0.531f);
    r.bbdClockMinHz.store(750.0f);
    r.bbdClockMaxRatio.store(1.0f);
    r.bbdStages.store(1024.0f);
    r.bbdFilterMaxRatio.store(0.35f);
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

    bool loadedAnyVariantInternals = false;

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
        loadedAnyVariantInternals = true;
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

    if (loadedAnyVariantInternals)
    {
        const int activeEngine = processor.getCurrentEngineColorIndex();
        const bool activeHQ = processor.isHqEnabled();
        processor.syncEngineInternalsToActiveDsp(activeEngine, activeHQ);
    }

    // Engine parameter profiles are runtime skin-state memory and should boot from
    // factory engine defaults each plugin load. Do not import persisted defaults
    // profiles here, otherwise stale snapshots can override intended base presets.
}
} // namespace

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
    initTuningDefaults();
    initializeEngineInternalProfiles();
    loadPersistedDefaults(*this);
    applyEngineParamProfile(getCurrentEngineColorIndex());
    saveCurrentParamsToEngineProfile(getCurrentEngineColorIndex());
    parameters.addParameterListener(ENGINE_COLOR_ID, this);
    parameters.addParameterListener(RATE_ID, this);
    parameters.addParameterListener(DEPTH_ID, this);
    parameters.addParameterListener(OFFSET_ID, this);
    parameters.addParameterListener(WIDTH_ID, this);
    parameters.addParameterListener(COLOR_ID, this);
    parameters.addParameterListener(MIX_ID, this);
    lastEngineIndex = getCurrentEngineColorIndex();
}

ChoroborosAudioProcessor::~ChoroborosAudioProcessor()
{
    parameters.removeParameterListener(ENGINE_COLOR_ID, this);
    parameters.removeParameterListener(RATE_ID, this);
    parameters.removeParameterListener(DEPTH_ID, this);
    parameters.removeParameterListener(OFFSET_ID, this);
    parameters.removeParameterListener(WIDTH_ID, this);
    parameters.removeParameterListener(COLOR_ID, this);
    parameters.removeParameterListener(MIX_ID, this);
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
    startTimerHz(10);  // Apply runtime tuning on message thread, 10 Hz
}

void ChoroborosAudioProcessor::releaseResources()
{
    stopTimer();
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateDSPParameters();

    {
        juce::ScopedLock sl(dspLock);
        juce::dsp::AudioBlock<float> block(buffer);
        chorusDSP->process(block);
    }
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
    return new ChoroborosPluginEditor (*this);
}

//==============================================================================
void ChoroborosAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChoroborosAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            stateLoadInProgress = true;
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            if (auto* p = parameters.getRawParameterValue(ENGINE_COLOR_ID))
                lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(p->load()));
            stateLoadInProgress = false;
        }
    }
}

void ChoroborosAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (presetLoadInProgress || stateLoadInProgress)
    {
        if (parameterID == ENGINE_COLOR_ID)
            lastEngineIndex = juce::jlimit(0, 4, static_cast<int>(newValue));
        return;
    }

    if (parameterID == ENGINE_COLOR_ID)
    {
        const int newEngine = juce::jlimit(0, 4, static_cast<int>(newValue));
        saveCurrentParamsToEngineProfile(lastEngineIndex);
        lastEngineIndex = newEngine;
        applyEngineParamProfile(newEngine);
        return;
    }

    if (parameterID == RATE_ID
        || parameterID == DEPTH_ID
        || parameterID == OFFSET_ID
        || parameterID == WIDTH_ID
        || parameterID == COLOR_ID
        || parameterID == MIX_ID)
    {
        saveCurrentParamsToEngineProfile(getCurrentEngineColorIndex());
    }
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
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
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
