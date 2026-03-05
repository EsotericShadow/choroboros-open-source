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

#include "ChorusDSP.h"
#include "ChorusDSPPrepare.h"
#include "ChorusDSPProcess.h"
#include "../Cores/ChorusCore.h"
#include "../Cores/green_engine_classic/ChorusCoreLagrange3rd.h"
#include "../Cores/green_engine_classic/ChorusCoreLagrange5th.h"
#include "../Cores/blue_engine_modern/ChorusCoreCubic.h"
#include "../Cores/blue_engine_modern/ChorusCoreThiran.h"
#include "../Cores/red_engine_vintage/ChorusCoreBBD.h"
#include "../Cores/red_engine_vintage/ChorusCoreTape.h"
#include "../Cores/purple_engine_experimental/ChorusCorePhaseWarped.h"
#include "../Cores/purple_engine_experimental/ChorusCoreOrbit.h"
#include "../Cores/black_engine_linear/ChorusCoreLinear.h"
#include "../Cores/black_engine_linear/ChorusCoreLinearEnsemble.h"
#include <cmath>

namespace
{
choroboros::CoreId legacyCoreIdForSlot(int colorIndex, bool hqEnabled)
{
    switch (juce::jlimit(0, 4, colorIndex))
    {
        case 0: return hqEnabled ? choroboros::CoreId::lagrange5 : choroboros::CoreId::lagrange3;
        case 1: return hqEnabled ? choroboros::CoreId::thiran : choroboros::CoreId::cubic;
        case 2: return hqEnabled ? choroboros::CoreId::tape : choroboros::CoreId::bbd;
        case 3: return hqEnabled ? choroboros::CoreId::orbit : choroboros::CoreId::phase_warp;
        case 4: default: return hqEnabled ? choroboros::CoreId::ensemble : choroboros::CoreId::linear;
    }
}

std::unique_ptr<ChorusCore> createCoreForId(choroboros::CoreId coreId)
{
    switch (coreId)
    {
        case choroboros::CoreId::lagrange3: return std::make_unique<ChorusCoreLagrange3rd>();
        case choroboros::CoreId::lagrange5: return std::make_unique<ChorusCoreLagrange5th>();
        case choroboros::CoreId::cubic: return std::make_unique<ChorusCoreCubic>();
        case choroboros::CoreId::thiran: return std::make_unique<ChorusCoreThiran>();
        case choroboros::CoreId::bbd: return std::make_unique<ChorusCoreBBD>();
        case choroboros::CoreId::tape: return std::make_unique<ChorusCoreTape>();
        case choroboros::CoreId::phase_warp: return std::make_unique<ChorusCorePhaseWarped>();
        case choroboros::CoreId::orbit: return std::make_unique<ChorusCoreOrbit>();
        case choroboros::CoreId::linear: return std::make_unique<ChorusCoreLinear>();
        case choroboros::CoreId::ensemble: return std::make_unique<ChorusCoreLinearEnsemble>();
        case choroboros::CoreId::count: break;
    }

    return std::make_unique<ChorusCoreLagrange3rd>();
}
}

ChorusDSP::ChorusDSP()
{
    // Pre-create all engine/HQ core variants to avoid heap allocation on the audio thread.
    for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
    {
        for (int mode = 0; mode < choroboros::kEngineModeCount; ++mode)
        {
            const bool hqEnabled = (mode == 1);
            const choroboros::CoreId legacyCore = legacyCoreIdForSlot(engine, hqEnabled);
            coreVariants[static_cast<size_t>(getCoreVariantIndex(engine, hqEnabled))] = createCoreForId(legacyCore);
        }
    }

    // Build a fully prewarmed modular pool: each slot/mode gets all assignable cores.
    for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
    {
        for (int mode = 0; mode < choroboros::kEngineModeCount; ++mode)
        {
            for (std::size_t coreIndex = 0; coreIndex < kNumAssignableCores; ++coreIndex)
            {
                const auto coreId = static_cast<choroboros::CoreId>(coreIndex);
                modularCorePool[static_cast<size_t>(engine)]
                              [static_cast<size_t>(mode)]
                              [coreIndex] = createCoreForId(coreId);
            }
        }
    }

    // Start with Green Normal (Lagrange3rd)
    currentColorIndex = 0;
    currentQualityHQ = false;
    coreAssignments.resetToLegacy();
    currentCore = resolveCorePointer(currentColorIndex, currentQualityHQ, &currentCoreId);
    pendingCoreId = currentCoreId;
}

ChorusDSP::~ChorusDSP()
{
}

void ChorusDSP::applyRuntimeTuning()
{
    if (spec.sampleRate <= 0.0)
        return;

    auto clampMs = [](float value, float minValue, float maxValue)
    {
        return juce::jlimit(minValue, maxValue, value);
    };

    auto clamp01 = [](float value)
    {
        return juce::jlimit(0.0f, 1.0f, value);
    };

    runtimeTuningSnapshot.rateSmoothingMs = clampMs(runtimeTuning.rateSmoothingMs.load(), 0.0f, 1000.0f);
    runtimeTuningSnapshot.depthSmoothingMs = clampMs(runtimeTuning.depthSmoothingMs.load(), 0.0f, 2000.0f);
    runtimeTuningSnapshot.depthRateLimit = juce::jmax(0.0f, runtimeTuning.depthRateLimit.load());
    runtimeTuningSnapshot.centreDelaySmoothingMs = clampMs(runtimeTuning.centreDelaySmoothingMs.load(), 0.0f, 2000.0f);
    runtimeTuningSnapshot.colorSmoothingMs = clampMs(runtimeTuning.colorSmoothingMs.load(), 0.0f, 1000.0f);
    runtimeTuningSnapshot.widthSmoothingMs = clampMs(runtimeTuning.widthSmoothingMs.load(), 0.0f, 1000.0f);
    runtimeTuningSnapshot.centreDelayBaseMs = runtimeTuning.centreDelayBaseMs.load();
    runtimeTuningSnapshot.centreDelayScale = runtimeTuning.centreDelayScale.load();

    runtimeTuningSnapshot.hpfCutoffHz = juce::jmax(5.0f, runtimeTuning.hpfCutoffHz.load());
    runtimeTuningSnapshot.hpfQ = juce::jmax(0.1f, runtimeTuning.hpfQ.load());
    runtimeTuningSnapshot.lpfCutoffHz = juce::jlimit(20.0f, 20000.0f, runtimeTuning.lpfCutoffHz.load());
    runtimeTuningSnapshot.lpfQ = juce::jmax(0.1f, runtimeTuning.lpfQ.load());
    runtimeTuningSnapshot.preEmphasisFreqHz = juce::jmax(20.0f, runtimeTuning.preEmphasisFreqHz.load());
    runtimeTuningSnapshot.preEmphasisQ = juce::jmax(0.1f, runtimeTuning.preEmphasisQ.load());
    runtimeTuningSnapshot.preEmphasisGain = juce::jmax(0.01f, runtimeTuning.preEmphasisGain.load());
    runtimeTuningSnapshot.preEmphasisLevelSmoothing = clamp01(runtimeTuning.preEmphasisLevelSmoothing.load());
    runtimeTuningSnapshot.preEmphasisQuietThreshold = juce::jmax(0.0f, runtimeTuning.preEmphasisQuietThreshold.load());
    runtimeTuningSnapshot.preEmphasisMaxAmount = juce::jmax(0.0f, runtimeTuning.preEmphasisMaxAmount.load());
    runtimeTuningSnapshot.compressorAttackMs = juce::jmax(0.1f, runtimeTuning.compressorAttackMs.load());
    runtimeTuningSnapshot.compressorReleaseMs = juce::jmax(0.1f, runtimeTuning.compressorReleaseMs.load());
    runtimeTuningSnapshot.compressorThresholdDb = runtimeTuning.compressorThresholdDb.load();
    runtimeTuningSnapshot.compressorRatio = juce::jmax(1.0f, runtimeTuning.compressorRatio.load());
    runtimeTuningSnapshot.saturationDriveScale = juce::jmax(0.0f, runtimeTuning.saturationDriveScale.load());

    runtimeTuningSnapshot.greenBloomExponent = juce::jlimit(0.1f, 4.0f, runtimeTuning.greenBloomExponent.load());
    runtimeTuningSnapshot.greenBloomDepthScale = juce::jmax(0.0f, runtimeTuning.greenBloomDepthScale.load());
    runtimeTuningSnapshot.greenBloomCentreOffsetMs = juce::jmax(0.0f, runtimeTuning.greenBloomCentreOffsetMs.load());
    runtimeTuningSnapshot.greenBloomCutoffMaxHz = juce::jmax(20.0f, runtimeTuning.greenBloomCutoffMaxHz.load());
    runtimeTuningSnapshot.greenBloomCutoffMinHz = juce::jlimit(20.0f, runtimeTuningSnapshot.greenBloomCutoffMaxHz, runtimeTuning.greenBloomCutoffMinHz.load());
    runtimeTuningSnapshot.greenBloomWetBlend = juce::jlimit(0.0f, 1.0f, runtimeTuning.greenBloomWetBlend.load());
    runtimeTuningSnapshot.greenBloomGain = juce::jmax(0.0f, runtimeTuning.greenBloomGain.load());

    runtimeTuningSnapshot.blueFocusExponent = juce::jlimit(0.1f, 4.0f, runtimeTuning.blueFocusExponent.load());
    runtimeTuningSnapshot.blueFocusHpMinHz = juce::jmax(20.0f, runtimeTuning.blueFocusHpMinHz.load());
    runtimeTuningSnapshot.blueFocusHpMaxHz = juce::jmax(runtimeTuningSnapshot.blueFocusHpMinHz, runtimeTuning.blueFocusHpMaxHz.load());
    runtimeTuningSnapshot.blueFocusLpMaxHz = juce::jmax(20.0f, runtimeTuning.blueFocusLpMaxHz.load());
    runtimeTuningSnapshot.blueFocusLpMinHz = juce::jlimit(20.0f, runtimeTuningSnapshot.blueFocusLpMaxHz, runtimeTuning.blueFocusLpMinHz.load());
    runtimeTuningSnapshot.bluePresenceFreqMinHz = juce::jmax(20.0f, runtimeTuning.bluePresenceFreqMinHz.load());
    runtimeTuningSnapshot.bluePresenceFreqMaxHz = juce::jmax(runtimeTuningSnapshot.bluePresenceFreqMinHz, runtimeTuning.bluePresenceFreqMaxHz.load());
    runtimeTuningSnapshot.bluePresenceQMin = juce::jmax(0.1f, runtimeTuning.bluePresenceQMin.load());
    runtimeTuningSnapshot.bluePresenceQMax = juce::jmax(runtimeTuningSnapshot.bluePresenceQMin, runtimeTuning.bluePresenceQMax.load());
    runtimeTuningSnapshot.bluePresenceGainMaxDb = juce::jmax(0.0f, runtimeTuning.bluePresenceGainMaxDb.load());
    runtimeTuningSnapshot.blueFocusWetBlend = juce::jlimit(0.0f, 1.0f, runtimeTuning.blueFocusWetBlend.load());
    runtimeTuningSnapshot.blueFocusOutputGain = juce::jmax(0.0f, runtimeTuning.blueFocusOutputGain.load());

    runtimeTuningSnapshot.purpleWarpA = juce::jmax(0.0f, runtimeTuning.purpleWarpA.load());
    runtimeTuningSnapshot.purpleWarpB = juce::jmax(0.0f, runtimeTuning.purpleWarpB.load());
    runtimeTuningSnapshot.purpleWarpKBase = juce::jmax(0.1f, runtimeTuning.purpleWarpKBase.load());
    runtimeTuningSnapshot.purpleWarpKScale = juce::jmax(0.0f, runtimeTuning.purpleWarpKScale.load());
    runtimeTuningSnapshot.purpleWarpDelaySmoothingMs = clampMs(runtimeTuning.purpleWarpDelaySmoothingMs.load(), 0.0f, 2000.0f);

    runtimeTuningSnapshot.purpleOrbitEccentricity = juce::jmax(0.0f, runtimeTuning.purpleOrbitEccentricity.load());
    runtimeTuningSnapshot.purpleOrbitThetaRateBaseHz = juce::jmax(0.0f, runtimeTuning.purpleOrbitThetaRateBaseHz.load());
    runtimeTuningSnapshot.purpleOrbitThetaRateScaleHz = juce::jmax(0.0f, runtimeTuning.purpleOrbitThetaRateScaleHz.load());
    runtimeTuningSnapshot.purpleOrbitThetaRate2Ratio = juce::jmax(0.1f, runtimeTuning.purpleOrbitThetaRate2Ratio.load());
    runtimeTuningSnapshot.purpleOrbitEccentricity2Ratio = juce::jmax(0.0f, runtimeTuning.purpleOrbitEccentricity2Ratio.load());
    runtimeTuningSnapshot.purpleOrbitMix1 = juce::jlimit(0.0f, 1.0f, runtimeTuning.purpleOrbitMix1.load());
    runtimeTuningSnapshot.purpleOrbitStereoThetaOffset = runtimeTuning.purpleOrbitStereoThetaOffset.load();
    runtimeTuningSnapshot.purpleOrbitDelaySmoothingMs = clampMs(runtimeTuning.purpleOrbitDelaySmoothingMs.load(), 0.0f, 2000.0f);

    runtimeTuningSnapshot.blackNqDepthBase = juce::jmax(0.0f, runtimeTuning.blackNqDepthBase.load());
    runtimeTuningSnapshot.blackNqDepthScale = juce::jmax(0.0f, runtimeTuning.blackNqDepthScale.load());
    runtimeTuningSnapshot.blackNqDelayGlideMs = clampMs(runtimeTuning.blackNqDelayGlideMs.load(), 0.0f, 2000.0f);

    runtimeTuningSnapshot.blackHqTap2MixBase = juce::jlimit(0.0f, 1.0f, runtimeTuning.blackHqTap2MixBase.load());
    runtimeTuningSnapshot.blackHqTap2MixScale = juce::jmax(0.0f, runtimeTuning.blackHqTap2MixScale.load());
    runtimeTuningSnapshot.blackHqSecondTapDepthBase = juce::jmax(0.0f, runtimeTuning.blackHqSecondTapDepthBase.load());
    runtimeTuningSnapshot.blackHqSecondTapDepthScale = juce::jmax(0.0f, runtimeTuning.blackHqSecondTapDepthScale.load());
    runtimeTuningSnapshot.blackHqSecondTapDelayOffsetBase = juce::jmax(0.0f, runtimeTuning.blackHqSecondTapDelayOffsetBase.load());
    runtimeTuningSnapshot.blackHqSecondTapDelayOffsetScale = juce::jmax(0.0f, runtimeTuning.blackHqSecondTapDelayOffsetScale.load());

    runtimeTuningSnapshot.bbdDelaySmoothingMs = clampMs(runtimeTuning.bbdDelaySmoothingMs.load(), 0.0f, 2000.0f);
    runtimeTuningSnapshot.bbdDelayMinMs = juce::jmax(0.0f, runtimeTuning.bbdDelayMinMs.load());
    runtimeTuningSnapshot.bbdDelayMaxMs = juce::jmax(runtimeTuningSnapshot.bbdDelayMinMs, runtimeTuning.bbdDelayMaxMs.load());
    runtimeTuningSnapshot.bbdCentreBaseMs = runtimeTuning.bbdCentreBaseMs.load();
    runtimeTuningSnapshot.bbdCentreScale = runtimeTuning.bbdCentreScale.load();
    runtimeTuningSnapshot.bbdDepthMs = juce::jmax(0.0f, runtimeTuning.bbdDepthMs.load());
    runtimeTuningSnapshot.bbdClockSmoothingMs = clampMs(runtimeTuning.bbdClockSmoothingMs.load(), 0.0f, 2000.0f);
    runtimeTuningSnapshot.bbdFilterSmoothingMs = clampMs(runtimeTuning.bbdFilterSmoothingMs.load(), 0.0f, 2000.0f);
    runtimeTuningSnapshot.bbdFilterCutoffMinHz = juce::jmax(20.0f, runtimeTuning.bbdFilterCutoffMinHz.load());
    runtimeTuningSnapshot.bbdFilterCutoffMaxHz = juce::jmax(runtimeTuningSnapshot.bbdFilterCutoffMinHz, runtimeTuning.bbdFilterCutoffMaxHz.load());
    runtimeTuningSnapshot.bbdFilterCutoffScale = juce::jmax(0.0f, runtimeTuning.bbdFilterCutoffScale.load());
    runtimeTuningSnapshot.bbdClockMinHz = juce::jmax(20.0f, runtimeTuning.bbdClockMinHz.load());
    runtimeTuningSnapshot.bbdClockMaxRatio = clamp01(runtimeTuning.bbdClockMaxRatio.load());
    runtimeTuningSnapshot.bbdStages = juce::jlimit(256.0f, 2048.0f, runtimeTuning.bbdStages.load());
    runtimeTuningSnapshot.bbdFilterMaxRatio = juce::jlimit(0.1f, 0.5f, runtimeTuning.bbdFilterMaxRatio.load());

    runtimeTuningSnapshot.tapeDelaySmoothingMs = clampMs(runtimeTuning.tapeDelaySmoothingMs.load(), 0.0f, 5000.0f);
    runtimeTuningSnapshot.tapeCentreBaseMs = runtimeTuning.tapeCentreBaseMs.load();
    runtimeTuningSnapshot.tapeCentreScale = runtimeTuning.tapeCentreScale.load();
    runtimeTuningSnapshot.tapeToneMaxHz = juce::jmax(20.0f, runtimeTuning.tapeToneMaxHz.load());
    runtimeTuningSnapshot.tapeToneMinHz = juce::jmax(20.0f, runtimeTuning.tapeToneMinHz.load());
    runtimeTuningSnapshot.tapeToneSmoothingCoeff = clamp01(runtimeTuning.tapeToneSmoothingCoeff.load());
    runtimeTuningSnapshot.tapeDriveScale = juce::jmax(0.0f, runtimeTuning.tapeDriveScale.load());
    runtimeTuningSnapshot.tapeLfoRatioScale = runtimeTuning.tapeLfoRatioScale.load();
    runtimeTuningSnapshot.tapeLfoModSmoothingCoeff = clamp01(runtimeTuning.tapeLfoModSmoothingCoeff.load());
    runtimeTuningSnapshot.tapeRatioSmoothingCoeff = clamp01(runtimeTuning.tapeRatioSmoothingCoeff.load());
    runtimeTuningSnapshot.tapePhaseDamping = clamp01(runtimeTuning.tapePhaseDamping.load());
    runtimeTuningSnapshot.tapeWowFreqBase = juce::jmax(0.0f, runtimeTuning.tapeWowFreqBase.load());
    runtimeTuningSnapshot.tapeWowFreqSpread = runtimeTuning.tapeWowFreqSpread.load();
    runtimeTuningSnapshot.tapeFlutterFreqBase = juce::jmax(0.0f, runtimeTuning.tapeFlutterFreqBase.load());
    runtimeTuningSnapshot.tapeFlutterFreqSpread = runtimeTuning.tapeFlutterFreqSpread.load();
    runtimeTuningSnapshot.tapeWowDepthBase = juce::jmax(0.0f, runtimeTuning.tapeWowDepthBase.load());
    runtimeTuningSnapshot.tapeWowDepthSpread = runtimeTuning.tapeWowDepthSpread.load();
    runtimeTuningSnapshot.tapeFlutterDepthBase = juce::jmax(0.0f, runtimeTuning.tapeFlutterDepthBase.load());
    runtimeTuningSnapshot.tapeFlutterDepthSpread = runtimeTuning.tapeFlutterDepthSpread.load();
    runtimeTuningSnapshot.tapeRatioMin = runtimeTuning.tapeRatioMin.load();
    runtimeTuningSnapshot.tapeRatioMax = runtimeTuning.tapeRatioMax.load();
    runtimeTuningSnapshot.tapeWetGain = juce::jmax(0.0f, runtimeTuning.tapeWetGain.load());
    runtimeTuningSnapshot.tapeHermiteTension = juce::jlimit(0.0f, 1.0f, runtimeTuning.tapeHermiteTension.load());

    const bool forceApply = !runtimeTuningApplied;

    if (forceApply || runtimeTuningSnapshot.rateSmoothingMs != lastAppliedTuningSnapshot.rateSmoothingMs)
    {
        const float current = smoothedRate.getCurrentValue();
        const float target = smoothedRate.getTargetValue();
        smoothedRate.reset(spec.sampleRate, runtimeTuningSnapshot.rateSmoothingMs * 0.001f);
        smoothedRate.setCurrentAndTargetValue(current);
        smoothedRate.setTargetValue(target);
        lastAppliedTuningSnapshot.rateSmoothingMs = runtimeTuningSnapshot.rateSmoothingMs;
    }

    if (forceApply || runtimeTuningSnapshot.depthSmoothingMs != lastAppliedTuningSnapshot.depthSmoothingMs)
    {
        const float depthSmoothingMs = juce::jmax(0.001f, runtimeTuningSnapshot.depthSmoothingMs);
        depthSmoothingCoeff = std::exp(-1.0f / (depthSmoothingMs * 0.001f * spec.sampleRate));
        lastAppliedTuningSnapshot.depthSmoothingMs = runtimeTuningSnapshot.depthSmoothingMs;
    }

    if (forceApply || runtimeTuningSnapshot.depthRateLimit != lastAppliedTuningSnapshot.depthRateLimit)
    {
        depthRateLimit = runtimeTuningSnapshot.depthRateLimit;
        depthRateLimitPerSample = depthRateLimit / static_cast<float>(spec.sampleRate);
        lastAppliedTuningSnapshot.depthRateLimit = runtimeTuningSnapshot.depthRateLimit;
    }

    if (forceApply || runtimeTuningSnapshot.centreDelaySmoothingMs != lastAppliedTuningSnapshot.centreDelaySmoothingMs)
    {
        const float current = smoothedCentreDelay.getCurrentValue();
        const float target = smoothedCentreDelay.getTargetValue();
        smoothedCentreDelay.reset(spec.sampleRate, runtimeTuningSnapshot.centreDelaySmoothingMs * 0.001f);
        smoothedCentreDelay.setCurrentAndTargetValue(current);
        smoothedCentreDelay.setTargetValue(target);
        lastAppliedTuningSnapshot.centreDelaySmoothingMs = runtimeTuningSnapshot.centreDelaySmoothingMs;
    }

    if (forceApply || runtimeTuningSnapshot.colorSmoothingMs != lastAppliedTuningSnapshot.colorSmoothingMs)
    {
        const float current = smoothedColor.getCurrentValue();
        const float target = smoothedColor.getTargetValue();
        smoothedColor.reset(spec.sampleRate, runtimeTuningSnapshot.colorSmoothingMs * 0.001f);
        smoothedColor.setCurrentAndTargetValue(current);
        smoothedColor.setTargetValue(target);
        lastAppliedTuningSnapshot.colorSmoothingMs = runtimeTuningSnapshot.colorSmoothingMs;
    }

    if (forceApply || runtimeTuningSnapshot.widthSmoothingMs != lastAppliedTuningSnapshot.widthSmoothingMs)
    {
        const float current = smoothedWidth.getCurrentValue();
        const float target = smoothedWidth.getTargetValue();
        smoothedWidth.reset(spec.sampleRate, runtimeTuningSnapshot.widthSmoothingMs * 0.001f);
        smoothedWidth.setCurrentAndTargetValue(current);
        smoothedWidth.setTargetValue(target);
        lastAppliedTuningSnapshot.widthSmoothingMs = runtimeTuningSnapshot.widthSmoothingMs;
    }

    if (forceApply
        || runtimeTuningSnapshot.hpfCutoffHz != lastAppliedTuningSnapshot.hpfCutoffHz
        || runtimeTuningSnapshot.hpfQ != lastAppliedTuningSnapshot.hpfQ)
    {
        hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            spec.sampleRate, runtimeTuningSnapshot.hpfCutoffHz, runtimeTuningSnapshot.hpfQ);
        hpf.coefficients = hpfCoeffs;
        lastAppliedTuningSnapshot.hpfCutoffHz = runtimeTuningSnapshot.hpfCutoffHz;
        lastAppliedTuningSnapshot.hpfQ = runtimeTuningSnapshot.hpfQ;
    }

    if (forceApply
        || runtimeTuningSnapshot.lpfCutoffHz != lastAppliedTuningSnapshot.lpfCutoffHz
        || runtimeTuningSnapshot.lpfQ != lastAppliedTuningSnapshot.lpfQ)
    {
        lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            spec.sampleRate, runtimeTuningSnapshot.lpfCutoffHz, runtimeTuningSnapshot.lpfQ);
        lpf.coefficients = lpfCoeffs;
        lastAppliedTuningSnapshot.lpfCutoffHz = runtimeTuningSnapshot.lpfCutoffHz;
        lastAppliedTuningSnapshot.lpfQ = runtimeTuningSnapshot.lpfQ;
    }

    if (forceApply
        || runtimeTuningSnapshot.preEmphasisFreqHz != lastAppliedTuningSnapshot.preEmphasisFreqHz
        || runtimeTuningSnapshot.preEmphasisQ != lastAppliedTuningSnapshot.preEmphasisQ
        || runtimeTuningSnapshot.preEmphasisGain != lastAppliedTuningSnapshot.preEmphasisGain)
    {
        preEmphasisCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec.sampleRate,
            runtimeTuningSnapshot.preEmphasisFreqHz,
            runtimeTuningSnapshot.preEmphasisQ,
            runtimeTuningSnapshot.preEmphasisGain);
        preEmphasis.coefficients = preEmphasisCoeffs;
        lastAppliedTuningSnapshot.preEmphasisFreqHz = runtimeTuningSnapshot.preEmphasisFreqHz;
        lastAppliedTuningSnapshot.preEmphasisQ = runtimeTuningSnapshot.preEmphasisQ;
        lastAppliedTuningSnapshot.preEmphasisGain = runtimeTuningSnapshot.preEmphasisGain;
    }

    if (forceApply
        || runtimeTuningSnapshot.compressorAttackMs != lastAppliedTuningSnapshot.compressorAttackMs
        || runtimeTuningSnapshot.compressorReleaseMs != lastAppliedTuningSnapshot.compressorReleaseMs
        || runtimeTuningSnapshot.compressorThresholdDb != lastAppliedTuningSnapshot.compressorThresholdDb
        || runtimeTuningSnapshot.compressorRatio != lastAppliedTuningSnapshot.compressorRatio)
    {
        compressor.setAttack(runtimeTuningSnapshot.compressorAttackMs);
        compressor.setRelease(runtimeTuningSnapshot.compressorReleaseMs);
        compressor.setThreshold(runtimeTuningSnapshot.compressorThresholdDb);
        compressor.setRatio(runtimeTuningSnapshot.compressorRatio);
        lastAppliedTuningSnapshot.compressorAttackMs = runtimeTuningSnapshot.compressorAttackMs;
        lastAppliedTuningSnapshot.compressorReleaseMs = runtimeTuningSnapshot.compressorReleaseMs;
        lastAppliedTuningSnapshot.compressorThresholdDb = runtimeTuningSnapshot.compressorThresholdDb;
        lastAppliedTuningSnapshot.compressorRatio = runtimeTuningSnapshot.compressorRatio;
    }

    lastAppliedTuningSnapshot.bbdDelaySmoothingMs = runtimeTuningSnapshot.bbdDelaySmoothingMs;
    lastAppliedTuningSnapshot.bbdClockSmoothingMs = runtimeTuningSnapshot.bbdClockSmoothingMs;
    lastAppliedTuningSnapshot.bbdFilterSmoothingMs = runtimeTuningSnapshot.bbdFilterSmoothingMs;
    lastAppliedTuningSnapshot.tapeDelaySmoothingMs = runtimeTuningSnapshot.tapeDelaySmoothingMs;
    lastAppliedTuningSnapshot.tapeToneSmoothingCoeff = runtimeTuningSnapshot.tapeToneSmoothingCoeff;
    lastAppliedTuningSnapshot.tapeLfoModSmoothingCoeff = runtimeTuningSnapshot.tapeLfoModSmoothingCoeff;
    lastAppliedTuningSnapshot.tapeRatioSmoothingCoeff = runtimeTuningSnapshot.tapeRatioSmoothingCoeff;
    lastAppliedTuningSnapshot.tapePhaseDamping = runtimeTuningSnapshot.tapePhaseDamping;
    lastAppliedTuningSnapshot.centreDelayBaseMs = runtimeTuningSnapshot.centreDelayBaseMs;
    lastAppliedTuningSnapshot.centreDelayScale = runtimeTuningSnapshot.centreDelayScale;
    lastAppliedTuningSnapshot.preEmphasisLevelSmoothing = runtimeTuningSnapshot.preEmphasisLevelSmoothing;
    lastAppliedTuningSnapshot.preEmphasisQuietThreshold = runtimeTuningSnapshot.preEmphasisQuietThreshold;
    lastAppliedTuningSnapshot.preEmphasisMaxAmount = runtimeTuningSnapshot.preEmphasisMaxAmount;
    lastAppliedTuningSnapshot.saturationDriveScale = runtimeTuningSnapshot.saturationDriveScale;
    runtimeTuningApplied = true;
}

void ChorusDSP::prepare(const juce::dsp::ProcessSpec& processSpec)
{
    spec = processSpec;

    for (auto& core : coreVariants)
        if (core)
            core->prepare(spec, this);

    for (auto& perEngine : modularCorePool)
        for (auto& perMode : perEngine)
            for (auto& core : perMode)
                if (core)
                    core->prepare(spec, this);
    
    ChorusDSPPrepare::prepareLFOs(*this, spec);
    ChorusDSPPrepare::prepareBuffers(*this, spec);
    ChorusDSPPrepare::prepareFilters(*this, spec);
    maxBlockSize = static_cast<int>(spec.maximumBlockSize);
    coreCrossfadeBufferA.setSize(static_cast<int>(spec.numChannels), maxBlockSize, false, false, true);
    coreCrossfadeBufferB.setSize(static_cast<int>(spec.numChannels), maxBlockSize, false, false, true);

    const size_t channelCount = static_cast<size_t>(juce::jmax(0, static_cast<int>(spec.numChannels)));
    greenWetLPState.assign(channelCount, 0.0f);
    blueWetHPState.assign(channelCount, 0.0f);
    blueWetLPState.assign(channelCount, 0.0f);
    bluePresenceState.assign(channelCount, BiquadState{});
    bluePresenceB0 = 1.0f;
    bluePresenceB1 = 0.0f;
    bluePresenceB2 = 0.0f;
    bluePresenceA1 = 0.0f;
    bluePresenceA2 = 0.0f;
    bluePresenceCachedFreqHz = -1.0f;
    bluePresenceCachedQ = -1.0f;
    bluePresenceCachedGainDb = -1000.0f;
    
    // Initialize parameter smoothers
    // CRITICAL: Smooth ALL delay-related parameters to prevent read pointer discontinuities
    applyRuntimeTuning();

    // Map depth to engine-specific range
    smoothedDepthValue = mapDepthToEngineRange(depth);
    currentDepthTarget = mapDepthToEngineRange(depth);
    
    // Set initial values
    smoothedRate.setCurrentAndTargetValue(rateHz);
    smoothedCentreDelay.setCurrentAndTargetValue(calculateCentreDelay(depth));
    smoothedColor.setCurrentAndTargetValue(color);
    colorBlockValue = smoothedColor.getCurrentValue();
    smoothedWidth.setCurrentAndTargetValue(width);
    smoothedOffset.reset(spec.sampleRate, 0.06);
    smoothedOffset.setCurrentAndTargetValue(offsetDegrees);
    smoothedMix.reset(spec.sampleRate, 0.06);
    smoothedMix.setCurrentAndTargetValue(mix);
    
    // Set initial LFO and delay values
    lfo.setFrequency(rateHz);
    float mappedDepth = mapDepthToEngineRange(depth);
    oscVolume.setCurrentAndTargetValue(mappedDepth * 0.5f);  // oscVolumeMultiplier = 0.5
    
    reset();
}

void ChorusDSP::reset()
{
    for (auto& core : coreVariants)
        if (core)
            core->reset();

    for (auto& perEngine : modularCorePool)
        for (auto& perMode : perEngine)
            for (auto& core : perMode)
                if (core)
                    core->reset();
    
    lfo.reset();
    lfoCos.reset();
    // Keep oscVolume instant (depth already has heavy smoothing + rate limit)
    oscVolume.reset(spec.sampleRate, 0.0);
    oscVolume.setCurrentAndTargetValue(depth * 0.5f);  // oscVolumeMultiplier = 0.5
    dryWet.reset();
    smoothedOffset.setCurrentAndTargetValue(offsetDegrees);
    smoothedMix.setCurrentAndTargetValue(mix);
    dryWet.setWetMixProportion(mix);
    
    hpf.reset();
    lpf.reset();
    preEmphasis.reset();
    widthMidFilter1.reset();
    widthMidFilter2.reset();
    widthSideFilter1.reset();
    widthSideFilter2.reset();
    compressor.reset();
    
    inputLevel = 0.0f;
    coreSwitchCrossfadeActive = false;
    coreSwitchCrossfadeSamplesRemaining = 0;
    coreSwitchCrossfadeTotalSamples = 0;
    coreSwitchTargetCrossfadeSamples = 0;
    coreSwitchWarmupSamplesRemaining = 0;
    coreSwitchWarmupTotalSamples = 0;
    coreSwitchOldParamsSnapshotValid = false;
    coreSwitchOldRateHz = smoothedRate.getCurrentValue();
    coreSwitchOldDepth = smoothedDepthValue;
    coreSwitchOldCentreDelayMs = smoothedCentreDelay.getCurrentValue();
    coreSwitchOldColor = smoothedColor.getCurrentValue();
    coreSwitchOldOffsetDegrees = lfoPhaseOffset;
    coreSwitchOldBasePhaseRad = 0.0f;
    coreSwitchOldLfoAmplitude = 0.0f;
    lastBaseLfoPhaseRad = 0.0f;
    lastLfoAmplitude = 0.0f;
    previousCore = nullptr;
    pendingCore = nullptr;
    currentCore = resolveCorePointer(currentColorIndex, currentQualityHQ, &currentCoreId);
    pendingCoreId = currentCoreId;

    std::fill(greenWetLPState.begin(), greenWetLPState.end(), 0.0f);
    std::fill(blueWetHPState.begin(), blueWetHPState.end(), 0.0f);
    std::fill(blueWetLPState.begin(), blueWetLPState.end(), 0.0f);
    for (auto& state : bluePresenceState)
        state = {};
    bluePresenceCachedFreqHz = -1.0f;
    bluePresenceCachedQ = -1.0f;
    bluePresenceCachedGainDb = -1000.0f;
    colorBlockValue = smoothedColor.getCurrentValue();
    
    // Reset smoothed depth and rate-limited target to current value (mapped to engine range)
    smoothedDepthValue = mapDepthToEngineRange(depth);
    currentDepthTarget = mapDepthToEngineRange(depth);
}

float ChorusDSP::calculateCentreDelay(float depthValue)
{
    return runtimeTuningSnapshot.centreDelayBaseMs + runtimeTuningSnapshot.centreDelayScale * depthValue;
}

float ChorusDSP::mapColorToEngineRange(float normalizedColor) const
{
    // Map normalized color (0-1) to engine-specific ranges
    // Each engine interprets color differently
    switch (currentColorIndex)
    {
        case 0: // Green - Bloom macro (wet-only)
            // Direct mapping: 0-1 stays 0-1 (interpreted in processGreenBloomWet)
            return normalizedColor;
            
        case 1: // Blue - Focus macro (wet-only)
            // Direct mapping: 0-1 stays 0-1 (interpreted in processBlueFocusWet)
            return normalizedColor;
            
        case 2: // Red - NQ: post-chorus drive, HQ: tape tone + drive
            // Direct mapping: 0-1 stays 0-1
            return normalizedColor;
            
        case 3: // Purple - warp/orbit shape
            // Direct mapping: 0-1 stays 0-1 (used for warp amount or orbit eccentricity)
            return normalizedColor;

        case 4: // Black - modulation intensity / ensemble spread
            return normalizedColor;
            
        default:
            return normalizedColor;
    }
}

float ChorusDSP::mapRateToEngineRange(float normalizedRate) const
{
    // Map normalized rate (0-1) to engine-specific ranges
    // Currently all engines use the same rate range (0.01-10.0 Hz)
    // This can be customized per engine if needed
    return normalizedRate;
}

float ChorusDSP::mapDepthToEngineRange(float normalizedDepth) const
{
    // Map normalized depth (0-1) to engine-specific ranges
    if (modularCoreModeEnabled)
    {
        if (descriptorForResolvedCore().depthCompression)
            return normalizedDepth * 0.45f;
        return normalizedDepth;
    }

    if (currentColorIndex == 3) // Purple
    {
        // For Purple: map 0-1 (0-100% UI) to 0-0.45 (0-45% actual depth)
        // This means 100% on the knob = what was previously 45% depth
        return normalizedDepth * 0.45f;
    }
    // Other engines use full range (0-100%)
    return normalizedDepth;
}

float ChorusDSP::applySaturation(float sample, float colorValue)
{
    const float color = juce::jlimit(0.0f, 1.0f, colorValue);
    if (color <= 0.0f)
        return sample;

    // Red NQ color controls wet-only saturation amount.
    // Drive increases with color, and color crossfades dry->saturated so 0 is exact bypass.
    const float drive = 1.0f + runtimeTuningSnapshot.saturationDriveScale * color;
    const float saturated = std::tanh(sample * drive);
    return sample + color * (saturated - sample);
}

void ChorusDSP::processGreenBloomWet(juce::dsp::AudioBlock<float>& block, float colorValue)
{
    if (block.getNumSamples() == 0 || spec.sampleRate <= 0.0)
        return;

    const int numChannels = static_cast<int>(block.getNumChannels());
    if (numChannels <= 0)
        return;

    const auto& tuning = runtimeTuningSnapshot;
    const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
    const float color = juce::jlimit(0.0f, 1.0f, colorValue);
    const float bloom = std::pow(color, bloomExp);
    if (bloom <= 1.0e-4f)
        return;

    const float fs = static_cast<float>(spec.sampleRate);
    const float cutoffMaxHz = juce::jmax(20.0f, tuning.greenBloomCutoffMaxHz);
    const float cutoffMinHz = juce::jlimit(20.0f, cutoffMaxHz, tuning.greenBloomCutoffMinHz);
    const float cutoffHz = juce::jmap(bloom, 0.0f, 1.0f, cutoffMaxHz, cutoffMinHz);
    const float onePole = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / fs);
    const float wetBlend = juce::jlimit(0.0f, 1.0f, tuning.greenBloomWetBlend) * bloom;
    const float bloomGain = 1.0f + juce::jmax(0.0f, tuning.greenBloomGain) * bloom;
    const int numSamples = static_cast<int>(block.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        float lpState = greenWetLPState[static_cast<size_t>(ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            const float input = data[i];
            lpState = (1.0f - onePole) * input + onePole * lpState;
            const float bloomed = input + wetBlend * (lpState - input);
            data[i] = bloomed * bloomGain;
        }

        greenWetLPState[static_cast<size_t>(ch)] = lpState;
    }
}

void ChorusDSP::processBlueFocusWet(juce::dsp::AudioBlock<float>& block, float colorValue)
{
    if (block.getNumSamples() == 0 || spec.sampleRate <= 0.0)
        return;

    const int numChannels = static_cast<int>(block.getNumChannels());
    if (numChannels <= 0)
        return;

    const auto& tuning = runtimeTuningSnapshot;
    const float focusExp = juce::jmax(0.1f, tuning.blueFocusExponent);
    const float color = juce::jlimit(0.0f, 1.0f, colorValue);
    const float focus = std::pow(color, focusExp);
    if (focus <= 1.0e-4f)
        return;

    const float fs = static_cast<float>(spec.sampleRate);
    const float hpMinHz = juce::jmax(20.0f, tuning.blueFocusHpMinHz);
    const float hpMaxHz = juce::jmax(hpMinHz, tuning.blueFocusHpMaxHz);
    const float lpMaxHz = juce::jmax(20.0f, tuning.blueFocusLpMaxHz);
    const float lpMinHz = juce::jlimit(20.0f, lpMaxHz, tuning.blueFocusLpMinHz);
    const float hpCutoffHz = juce::jmap(focus, 0.0f, 1.0f, hpMinHz, hpMaxHz);
    const float lpCutoffHz = juce::jmap(focus, 0.0f, 1.0f, lpMaxHz, lpMinHz);
    const float hpAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * hpCutoffHz / fs);
    const float lpAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * lpCutoffHz / fs);

    const float presenceFreqMinHz = juce::jmax(20.0f, tuning.bluePresenceFreqMinHz);
    const float presenceFreqMaxHz = juce::jmax(presenceFreqMinHz, tuning.bluePresenceFreqMaxHz);
    const float presenceQMin = juce::jmax(0.1f, tuning.bluePresenceQMin);
    const float presenceQMax = juce::jmax(presenceQMin, tuning.bluePresenceQMax);
    const float presenceGainMaxDb = juce::jmax(0.0f, tuning.bluePresenceGainMaxDb);
    const float presenceFreqHz = juce::jmap(focus, 0.0f, 1.0f, presenceFreqMinHz, presenceFreqMaxHz);
    const float presenceQ = juce::jmap(focus, 0.0f, 1.0f, presenceQMin, presenceQMax);
    const float presenceGainDb = juce::jmap(focus, 0.0f, 1.0f, 0.0f, presenceGainMaxDb);
    const bool needsCoeffUpdate =
        std::abs(presenceFreqHz - bluePresenceCachedFreqHz) > 1.0f
        || std::abs(presenceQ - bluePresenceCachedQ) > 0.005f
        || std::abs(presenceGainDb - bluePresenceCachedGainDb) > 0.02f;

    if (needsCoeffUpdate)
    {
        const float A = std::pow(10.0f, presenceGainDb / 40.0f);
        const float w0 = 2.0f * juce::MathConstants<float>::pi * (presenceFreqHz / fs);
        const float cosW0 = std::cos(w0);
        const float sinW0 = std::sin(w0);
        const float alpha = sinW0 / (2.0f * juce::jmax(0.1f, presenceQ));

        const float b0 = 1.0f + alpha * A;
        const float b1 = -2.0f * cosW0;
        const float b2 = 1.0f - alpha * A;
        const float a0 = 1.0f + alpha / A;
        const float a1 = -2.0f * cosW0;
        const float a2 = 1.0f - alpha / A;
        const float invA0 = 1.0f / a0;

        bluePresenceB0 = b0 * invA0;
        bluePresenceB1 = b1 * invA0;
        bluePresenceB2 = b2 * invA0;
        bluePresenceA1 = a1 * invA0;
        bluePresenceA2 = a2 * invA0;
        bluePresenceCachedFreqHz = presenceFreqHz;
        bluePresenceCachedQ = presenceQ;
        bluePresenceCachedGainDb = presenceGainDb;
    }

    const float wetBlend = juce::jlimit(0.0f, 1.0f, tuning.blueFocusWetBlend) * focus;
    const float outputGain = 1.0f + juce::jmax(0.0f, tuning.blueFocusOutputGain) * focus;
    const int numSamples = static_cast<int>(block.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        float hpState = blueWetHPState[static_cast<size_t>(ch)];
        float lpState = blueWetLPState[static_cast<size_t>(ch)];
        BiquadState& presenceState = bluePresenceState[static_cast<size_t>(ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            const float input = data[i];
            hpState = (1.0f - hpAlpha) * input + hpAlpha * hpState;
            const float highPassed = input - hpState;
            lpState = (1.0f - lpAlpha) * highPassed + lpAlpha * lpState;

            const float peqOut =
                bluePresenceB0 * lpState
                + bluePresenceB1 * presenceState.x1
                + bluePresenceB2 * presenceState.x2
                - bluePresenceA1 * presenceState.y1
                - bluePresenceA2 * presenceState.y2;
            presenceState.x2 = presenceState.x1;
            presenceState.x1 = lpState;
            presenceState.y2 = presenceState.y1;
            presenceState.y1 = peqOut;

            const float focused = input + wetBlend * (peqOut - input);
            data[i] = focused * outputGain;
        }

        blueWetHPState[static_cast<size_t>(ch)] = hpState;
        blueWetLPState[static_cast<size_t>(ch)] = lpState;
    }
}

void ChorusDSP::processWidth(juce::dsp::AudioBlock<float>& block)
{
    if (block.getNumChannels() < 2)
        return;
    
    auto* left = block.getChannelPointer(0);
    auto* right = block.getChannelPointer(1);
    const int numSamples = static_cast<int>(block.getNumSamples());
    
    // Get smoothed width (block-constant is fine for width)
    float currentWidth = smoothedWidth.getNextValue();
    smoothedWidth.skip(numSamples - 1);
    
    // Simplified width processing: scale side channel based on width parameter
    // This avoids the incorrect band-splitting approach that can cause artifacts
    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        float r = right[i];
        
        // M/S conversion
        float mid = (l + r) * 0.5f;
        float side = (l - r) * 0.5f;
        
        // Apply width: scale side channel (0.0 = mono, 2.0 = full width)
        side *= currentWidth;
        
        // Back to L/R
        left[i] = mid + side;
        right[i] = mid - side;
    }
}

void ChorusDSP::process(const juce::dsp::AudioBlock<float>& block)
{
    if (block.getNumSamples() == 0)
        return;
    
    juce::ScopedNoDenormals noDenormals;
    
    juce::dsp::AudioBlock<float> nonConstBlock = block;
    auto context = juce::dsp::ProcessContextReplacing<float>(nonConstBlock);
    
    hpf.process(context);
    // applyRuntimeTuning() removed from audio path - contains heap allocation (IIR coeffs).
    // Called from prepare() and processor timer on message thread only.
    ChorusDSPProcess::processPreEmphasis(*this, nonConstBlock);
    ChorusDSPProcess::processPreChorusSaturation(*this, nonConstBlock);
    ChorusDSPProcess::processChorus(*this, nonConstBlock);
    lpf.process(context);
    if (nonConstBlock.getNumChannels() >= 2)
        processWidth(nonConstBlock);
    compressor.process(context);
}

void ChorusDSP::setRate(float rateHz_)
{
    rateHz = juce::jlimit(0.01f, 10.0f, rateHz_);
    smoothedRate.setTargetValue(rateHz);
}

void ChorusDSP::setDepth(float depth_)
{
    depth = juce::jlimit(0.0f, 1.0f, depth_);
    // Rate limiter in process() will gradually move currentDepthTarget towards this value
    // This prevents rapid changes that overwhelm the exponential smoother
}

void ChorusDSP::setOffset(float offsetDegrees_)
{
    offsetDegrees = juce::jlimit(0.0f, 180.0f, offsetDegrees_);
    smoothedOffset.setTargetValue(offsetDegrees);
}

void ChorusDSP::setWidth(float width_)
{
    width = juce::jlimit(0.0f, 2.0f, width_);
    smoothedWidth.setTargetValue(width);
}

void ChorusDSP::setColor(float color_)
{
    color = juce::jlimit(0.0f, 1.0f, color_);
    // Map to engine-specific range before smoothing
    float mappedColor = mapColorToEngineRange(color);
    smoothedColor.setTargetValue(mappedColor);
}

void ChorusDSP::setEngineColor(int colorIndex)
{
    colorIndex = juce::jlimit(0, 4, colorIndex);
    if (currentColorIndex != colorIndex)
    {
        currentColorIndex = colorIndex;
        // Keep depth transitions smooth across engine switches.
        currentDepthTarget = mapDepthToEngineRange(depth);
        smoothedColor.setTargetValue(mapColorToEngineRange(color));
        switchCore(currentColorIndex, currentQualityHQ);
    }
}

void ChorusDSP::setQualityEnabled(bool enabled)
{
    if (currentQualityHQ != enabled)
    {
        currentQualityHQ = enabled;
        switchCore(currentColorIndex, currentQualityHQ);
    }
}

void ChorusDSP::setMix(float mix_)
{
    mix = juce::jlimit(0.0f, 1.0f, mix_);
    smoothedMix.setTargetValue(mix);
}

void ChorusDSP::switchCore(int colorIndex, bool hq)
{
    choroboros::CoreId resolvedCoreId = choroboros::CoreId::lagrange3;
    ChorusCore* newCore = resolveCorePointer(colorIndex, hq, &resolvedCoreId);
    if (newCore == nullptr)
        return;

    if (newCore == currentCore)
    {
        currentCoreId = resolvedCoreId;
        pendingCoreId = resolvedCoreId;
        pendingCore = nullptr;
        coreSwitchTargetCrossfadeSamples = 0;
        coreSwitchWarmupSamplesRemaining = 0;
        coreSwitchWarmupTotalSamples = 0;
        coreSwitchOldParamsSnapshotValid = false;
        return;
    }
    if (newCore == pendingCore)
    {
        pendingCoreId = resolvedCoreId;
        return;
    }

    const auto slotForCore = [this](const ChorusCore* corePtr) -> int
    {
        if (corePtr == nullptr)
            return -1;

        for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
        {
            for (int mode = 0; mode < choroboros::kEngineModeCount; ++mode)
            {
                const bool slotHq = mode == 1;
                const int slotIndex = getCoreVariantIndex(engine, slotHq);
                if (coreVariants[static_cast<std::size_t>(slotIndex)].get() == corePtr)
                    return getCoreVariantIndex(engine, slotHq);

                for (std::size_t coreIndex = 0; coreIndex < kNumAssignableCores; ++coreIndex)
                {
                    if (modularCorePool[static_cast<std::size_t>(engine)]
                                       [static_cast<std::size_t>(mode)]
                                       [coreIndex]
                            .get() == corePtr)
                    {
                        return slotIndex;
                    }
                }
            }
        }
        return -1;
    };

    const int currentSlotIndex = slotForCore(currentCore);
    const int requestedSlotIndex = getCoreVariantIndex(colorIndex, hq);
    const bool engineFamilySwitch = (currentSlotIndex >= 0) && ((currentSlotIndex / 2) != (requestedSlotIndex / 2));
    const bool qualityToggleOnly = (currentSlotIndex >= 0) && ((currentSlotIndex / 2) == (requestedSlotIndex / 2))
                                   && ((currentSlotIndex % 2) != (requestedSlotIndex % 2));

    const float rateJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(smoothedRate.getTargetValue() - smoothedRate.getCurrentValue()) / 20.0f);
    const float depthJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(currentDepthTarget - smoothedDepthValue));
    const float offsetJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(smoothedOffset.getTargetValue() - smoothedOffset.getCurrentValue()) / 180.0f);
    const float mixJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(smoothedMix.getTargetValue() - smoothedMix.getCurrentValue()));
    const float colorJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(smoothedColor.getTargetValue() - smoothedColor.getCurrentValue()));
    const float centreDelayJumpNorm = juce::jlimit(0.0f, 1.0f,
        std::abs(smoothedCentreDelay.getTargetValue() - smoothedCentreDelay.getCurrentValue()) / 20.0f);

    float switchSeverity =
        0.26f * rateJumpNorm
        + 0.22f * depthJumpNorm
        + 0.17f * offsetJumpNorm
        + 0.12f * mixJumpNorm
        + 0.11f * colorJumpNorm
        + 0.12f * centreDelayJumpNorm;
    if (engineFamilySwitch)
        switchSeverity = juce::jmax(switchSeverity, 0.55f);
    else if (qualityToggleOnly)
        switchSeverity = juce::jmax(switchSeverity, 0.40f);
    switchSeverity = juce::jlimit(0.0f, 1.0f, switchSeverity);

    // Warm up the target core silently before audible crossfade to avoid residual-state zippering
    // when revisiting a previously used engine.
    coreSwitchOldParamsSnapshotValid = true;
    coreSwitchOldRateHz = smoothedRate.getCurrentValue();
    coreSwitchOldDepth = smoothedDepthValue;
    coreSwitchOldCentreDelayMs = smoothedCentreDelay.getCurrentValue();
    coreSwitchOldColor = smoothedColor.getCurrentValue();
    coreSwitchOldOffsetDegrees = lfoPhaseOffset;
    coreSwitchOldBasePhaseRad = lastBaseLfoPhaseRad;
    coreSwitchOldLfoAmplitude = lastLfoAmplitude;
    pendingCore = newCore;
    pendingCoreId = resolvedCoreId;

    if (spec.sampleRate > 0.0)
    {
        const float warmupMs = juce::jmap(switchSeverity, 22.0f, 95.0f);
        const float crossfadeMs = juce::jmap(switchSeverity, 45.0f, 170.0f);
        coreSwitchWarmupTotalSamples = juce::jmax(1, static_cast<int>(std::round(spec.sampleRate * warmupMs * 0.001f)));
        coreSwitchTargetCrossfadeSamples = juce::jmax(1, static_cast<int>(std::round(spec.sampleRate * crossfadeMs * 0.001f)));
        coreSwitchWarmupSamplesRemaining = coreSwitchWarmupTotalSamples;
    }
    else
    {
        currentCore = pendingCore;
        currentCoreId = pendingCoreId;
        pendingCore = nullptr;
        coreSwitchCrossfadeTotalSamples = 0;
        coreSwitchCrossfadeSamplesRemaining = 0;
        coreSwitchTargetCrossfadeSamples = 0;
        coreSwitchCrossfadeActive = false;
        coreSwitchWarmupTotalSamples = 0;
        coreSwitchWarmupSamplesRemaining = 0;
        coreSwitchOldParamsSnapshotValid = false;
        previousCore = nullptr;
    }
}

void ChorusDSP::setModularCoreModeEnabled(bool enabled)
{
    if (modularCoreModeEnabled == enabled)
        return;

    modularCoreModeEnabled = enabled;
    switchCore(currentColorIndex, currentQualityHQ);
}

void ChorusDSP::setCoreAssignments(const choroboros::CoreAssignmentTable& assignments)
{
    coreAssignments = assignments;
    if (modularCoreModeEnabled)
        switchCore(currentColorIndex, currentQualityHQ);
}

bool ChorusDSP::setCoreAssignment(int colorIndex, bool hqEnabled, choroboros::CoreId coreId)
{
    const int safeEngine = juce::jlimit(0, 4, colorIndex);
    const std::size_t coreIndex = static_cast<std::size_t>(coreId);
    const choroboros::CoreId safeCoreId = coreIndex < choroboros::coreIdCount()
        ? coreId
        : legacyCoreIdForSlot(safeEngine, hqEnabled);

    const bool duplicate = choroboros::assignmentIsDuplicate(coreAssignments, safeEngine, hqEnabled, safeCoreId);
    coreAssignments.set(safeEngine, hqEnabled, safeCoreId);

    if (modularCoreModeEnabled && safeEngine == currentColorIndex && hqEnabled == currentQualityHQ)
        switchCore(currentColorIndex, currentQualityHQ);

    return duplicate;
}

std::vector<choroboros::SlotAssignment> ChorusDSP::getDuplicateAssignmentWarnings() const
{
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

choroboros::CoreId ChorusDSP::getAssignedCoreId(int colorIndex, bool hqEnabled) const
{
    return coreAssignments.get(colorIndex, hqEnabled);
}

choroboros::CoreId ChorusDSP::getResolvedCoreId(int colorIndex, bool hqEnabled) const
{
    if (!modularCoreModeEnabled)
        return legacyCoreIdForSlot(colorIndex, hqEnabled);
    return coreAssignments.get(colorIndex, hqEnabled);
}

choroboros::CoreId ChorusDSP::getCurrentResolvedCoreId() const
{
    if (currentCore != nullptr)
        return currentCoreId;
    return getResolvedCoreId(currentColorIndex, currentQualityHQ);
}

const choroboros::CorePackageDescriptor& ChorusDSP::getCurrentCoreDescriptor() const
{
    return choroboros::descriptorForCore(getCurrentResolvedCoreId());
}

const std::array<choroboros::CorePackageDescriptor, choroboros::coreIdCount()>& ChorusDSP::getCorePackageDescriptors()
{
    return choroboros::kCorePackageDescriptors;
}

const choroboros::CorePackageDescriptor& ChorusDSP::getCorePackageDescriptor(choroboros::CoreId coreId)
{
    return choroboros::descriptorForCore(coreId);
}

ChorusCore* ChorusDSP::resolveCorePointer(int colorIndex, bool hqEnabled, choroboros::CoreId* outCoreId)
{
    const int safeEngine = juce::jlimit(0, 4, colorIndex);
    const int safeMode = hqEnabled ? 1 : 0;

    if (modularCoreModeEnabled)
    {
        const choroboros::CoreId assigned = coreAssignments.get(safeEngine, hqEnabled);
        std::size_t coreIndex = static_cast<std::size_t>(assigned);
        if (coreIndex >= kNumAssignableCores)
            coreIndex = static_cast<std::size_t>(legacyCoreIdForSlot(safeEngine, hqEnabled));

        if (outCoreId != nullptr)
            *outCoreId = static_cast<choroboros::CoreId>(coreIndex);

        auto* core = modularCorePool[static_cast<std::size_t>(safeEngine)]
                                     [static_cast<std::size_t>(safeMode)]
                                     [coreIndex]
                         .get();
        if (core != nullptr)
            return core;
    }

    const choroboros::CoreId legacyId = legacyCoreIdForSlot(safeEngine, hqEnabled);
    if (outCoreId != nullptr)
        *outCoreId = legacyId;
    return coreVariants[static_cast<size_t>(getCoreVariantIndex(safeEngine, hqEnabled))].get();
}

const choroboros::CorePackageDescriptor& ChorusDSP::descriptorForResolvedCore() const
{
    return choroboros::descriptorForCore(getCurrentResolvedCoreId());
}
