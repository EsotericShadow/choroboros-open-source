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

ChorusDSP::ChorusDSP()
{
    // Pre-create all engine/HQ core variants to avoid heap allocation on the audio thread.
    coreVariants[getCoreVariantIndex(0, false)] = std::make_unique<ChorusCoreLagrange3rd>();
    coreVariants[getCoreVariantIndex(0, true)]  = std::make_unique<ChorusCoreLagrange5th>();
    coreVariants[getCoreVariantIndex(1, false)] = std::make_unique<ChorusCoreCubic>();
    coreVariants[getCoreVariantIndex(1, true)]  = std::make_unique<ChorusCoreThiran>();
    coreVariants[getCoreVariantIndex(2, false)] = std::make_unique<ChorusCoreBBD>();
    coreVariants[getCoreVariantIndex(2, true)]  = std::make_unique<ChorusCoreTape>();
    coreVariants[getCoreVariantIndex(3, false)] = std::make_unique<ChorusCorePhaseWarped>();
    coreVariants[getCoreVariantIndex(3, true)]  = std::make_unique<ChorusCoreOrbit>();
    coreVariants[getCoreVariantIndex(4, false)] = std::make_unique<ChorusCoreLinear>();
    coreVariants[getCoreVariantIndex(4, true)]  = std::make_unique<ChorusCoreLinearEnsemble>();

    // Start with Green Normal (Lagrange3rd)
    currentColorIndex = 0;
    currentQualityHQ = false;
    currentCore = coreVariants[getCoreVariantIndex(currentColorIndex, currentQualityHQ)].get();
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
            core->prepare(spec);
    
    ChorusDSPPrepare::prepareLFOs(*this, spec);
    ChorusDSPPrepare::prepareBuffers(*this, spec);
    ChorusDSPPrepare::prepareFilters(*this, spec);
    maxBlockSize = static_cast<int>(spec.maximumBlockSize);
    coreCrossfadeBufferA.setSize(static_cast<int>(spec.numChannels), maxBlockSize, false, false, true);
    coreCrossfadeBufferB.setSize(static_cast<int>(spec.numChannels), maxBlockSize, false, false, true);
    
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
    smoothedWidth.setCurrentAndTargetValue(width);
    
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
    
    lfo.reset();
    lfoCos.reset();
    // Keep oscVolume instant (depth already has heavy smoothing + rate limit)
    oscVolume.reset(spec.sampleRate, 0.0);
    oscVolume.setCurrentAndTargetValue(depth * 0.5f);  // oscVolumeMultiplier = 0.5
    dryWet.reset();
    
    hpf.reset();
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
    previousCore = nullptr;
    
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
        case 0: // Green - Saturation: 0% = no saturation, 100% = full saturation
            // Direct mapping: 0-1 stays 0-1 (used in applySaturation)
            return normalizedColor;
            
        case 1: // Blue - Saturation: 0% = no saturation, 100% = full saturation
            // Direct mapping: 0-1 stays 0-1 (used in applySaturation)
            return normalizedColor;
            
        case 2: // Red - Tape tone: 0% = bright (16kHz), 100% = dark (12kHz)
            // Direct mapping: 0-1 stays 0-1 (used for tape drive and tone cutoff)
            return normalizedColor;
            
        case 3: // Purple - Warp/Orbit: 0% = minimal effect, 100% = maximum effect
            // Direct mapping: 0-1 stays 0-1 (used for warp amount or orbit eccentricity)
            return normalizedColor;

        case 4: // Black - Linear / Linear Ensemble
            return normalizedColor;
            
        default:
            return normalizedColor;
    }
}

float ChorusDSP::mapRateToEngineRange(float normalizedRate) const
{
    // Map normalized rate (0-1) to engine-specific ranges
    // Currently all engines use the same rate range (0.01-20.0 Hz)
    // This can be customized per engine if needed
    return normalizedRate;
}

float ChorusDSP::mapDepthToEngineRange(float normalizedDepth) const
{
    // Map normalized depth (0-1) to engine-specific ranges
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
    // Soft clip using tanh
    // At color = 0%, no saturation (drive = 1.0, output = input)
    // At color = 100%, maximum saturation (drive = 3.0)
    // Make it more audible by using stronger drive curve
    float drive = 1.0f + runtimeTuningSnapshot.saturationDriveScale * colorValue;
    float saturated = std::tanh(sample * drive);
    // Normalize to maintain level (tanh compresses, so we scale back)
    return saturated / drive;
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
    ChorusDSPProcess::processSaturation(*this, nonConstBlock);
    ChorusDSPProcess::processChorus(*this, nonConstBlock);
    
    if (nonConstBlock.getNumChannels() >= 2)
        processWidth(nonConstBlock);
    
    compressor.process(context);
}

void ChorusDSP::setRate(float rateHz_)
{
    rateHz = juce::jlimit(0.01f, 20.0f, rateHz_);
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
    // Use offset to control phase difference between L/R channels
    // This creates stereo width in the chorus effect itself
    lfoPhaseOffset = offsetDegrees;  // Actually set the phase offset
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
    float mix = juce::jlimit(0.0f, 1.0f, mix_);
    dryWet.setWetMixProportion(mix);
}

void ChorusDSP::switchCore(int colorIndex, bool hq)
{
    const int variantIndex = getCoreVariantIndex(colorIndex, hq);
    ChorusCore* newCore = coreVariants[variantIndex].get();
    if (newCore == nullptr || newCore == currentCore)
        return;

    // Cores are pre-prepared in prepare(); only reset state on switch.
    if (spec.sampleRate > 0.0)
        newCore->reset();

    // Swap cores with a short fade to eliminate click/pop transients.
    previousCore = currentCore;
    currentCore = newCore;
    if (spec.sampleRate > 0.0)
    {
        const float crossfadeMs = juce::jlimit(5.0f, 100.0f, runtimeTuning.coreSwitchCrossfadeMs.load());
        coreSwitchCrossfadeTotalSamples = juce::jmax(1, static_cast<int>(std::round(spec.sampleRate * crossfadeMs * 0.001f)));
        coreSwitchCrossfadeSamplesRemaining = coreSwitchCrossfadeTotalSamples;
        coreSwitchCrossfadeActive = (previousCore != nullptr);
    }
    else
    {
        coreSwitchCrossfadeTotalSamples = 0;
        coreSwitchCrossfadeSamplesRemaining = 0;
        coreSwitchCrossfadeActive = false;
        previousCore = nullptr;
    }
}
