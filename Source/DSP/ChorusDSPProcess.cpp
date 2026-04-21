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

#include "ChorusDSPProcess.h"
#include "ChorusDSP.h"
#include "../Cores/ChorusCore.h"
#include <cmath>
#include <utility>

#if CHOROBOROS_PERFETTO_ENABLED
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif

void ChorusDSPProcess::processPreEmphasis(ChorusDSP& chorusDSP,
                                          juce::dsp::AudioBlock<float>& block,
                                          juce::dsp::IIR::Filter<float>& filter,
                                          float& inputLevelState)
{
    if (block.getNumSamples() == 0 || block.getNumChannels() == 0)
        return;

    if (chorusDSP.isModularCoreModeEnabled())
    {
        if (chorusDSP.getCurrentCoreDescriptor().skipPreEmphasis)
            return;
    }
    else
    {
        // Red NQ: skip pre-emphasis - it boosts highs before BBD, which aliase and cause downsampled drone
        if (chorusDSP.currentColorIndex == 2 && !chorusDSP.currentQualityHQ)
            return;
    }

    float rmsLevel = 0.0f;
    const int blockSize = static_cast<int>(block.getNumSamples());
    for (int ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        float sumSq = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            sumSq += data[i] * data[i];
        rmsLevel += std::sqrt(sumSq / blockSize);
    }
    rmsLevel /= block.getNumChannels();
    
    const auto& tuning = chorusDSP.runtimeTuningSnapshot;
    const float levelSmoothing = juce::jlimit(0.0f, 1.0f, tuning.preEmphasisLevelSmoothing);
    inputLevelState = levelSmoothing * inputLevelState + (1.0f - levelSmoothing) * rmsLevel;
    
    const float quietThreshold = tuning.preEmphasisQuietThreshold;
    float preEmphAmount = 0.0f;
    if (quietThreshold > 0.0f && inputLevelState < quietThreshold)
        preEmphAmount = (quietThreshold - inputLevelState) / quietThreshold * tuning.preEmphasisMaxAmount;
    
    if (preEmphAmount > 0.0f)
    {
        jassert(blockSize <= chorusDSP.maxBlockSize);
        for (int ch = 0; ch < block.getNumChannels(); ++ch)
            chorusDSP.preEmphOriginalBuffer.copyFrom(ch, 0, block.getChannelPointer(ch), blockSize);
        
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        filter.process(context);
        
        for (int ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* filtered = block.getChannelPointer(ch);
            auto* original = chorusDSP.preEmphOriginalBuffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
                filtered[i] = original[i] + preEmphAmount * (filtered[i] - original[i]);
        }
    }
}

void ChorusDSPProcess::processPreChorusSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
    // No pre-chorus saturation: adds harmonics that aliase in BBD (Red NQ drone).
    // Saturation is applied post-chorus for the engines that use Color as drive.
    (void) chorusDSP;
    (void) block;
}

void ChorusDSPProcess::processWetCharacter(ChorusDSP& chorusDSP,
                                           juce::dsp::AudioBlock<float>& block,
                                           const choroboros::CorePackageDescriptor& descriptor,
                                           int engineIndex,
                                           ChorusDSP::WetCharacterState& state)
{
    // Color drives wet-only character macros:
    // Green => Bloom, Blue => Focus, others => handled elsewhere.
    const int numSamples = static_cast<int>(block.getNumSamples());
    const float currentColor = juce::jlimit(0.0f, 1.0f, chorusDSP.colorBlockValue);
    const float* colorPerSample = chorusDSP.getColorPerSample(numSamples);
    if (chorusDSP.isModularCoreModeEnabled())
    {
        if (descriptor.bloomWetCharacter)
        {
            chorusDSP.processGreenBloomWet(block, currentColor, state, colorPerSample);
            return;
        }

        if (descriptor.focusWetCharacter)
        {
            chorusDSP.processBlueFocusWet(block, currentColor, state, colorPerSample);
            return;
        }

        return;
    }

    if (engineIndex == 0)
    {
        chorusDSP.processGreenBloomWet(block, currentColor, state, colorPerSample);
        return;
    }

    if (engineIndex == 1)
    {
        chorusDSP.processBlueFocusWet(block, currentColor, state, colorPerSample);
        return;
    }
}

void ChorusDSPProcess::processPostChorusSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block, const choroboros::CorePackageDescriptor& descriptor, int engineIndex)
{
    bool usesPostSaturation = descriptor.postChorusSaturation;

    if (!usesPostSaturation)
        return;

    const int numSamples = static_cast<int>(block.getNumSamples());
    const float currentColor = juce::jlimit(0.0f, 1.0f, chorusDSP.colorBlockValue);
    const float* colorPerSample = chorusDSP.getColorPerSample(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float colorValue = juce::jlimit(0.0f, 1.0f, colorPerSample != nullptr ? colorPerSample[i] : currentColor);
        for (int ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            data[i] = chorusDSP.applySaturation(data[i], colorValue);
        }
    }
}

void ChorusDSPProcess::processOutputPeakCatch(ChorusDSP& chorusDSP,
                                               juce::dsp::AudioBlock<float>& block)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());
    if (numChannels <= 0 || numSamples <= 0)
        return;

    if (static_cast<size_t>(numChannels) > chorusDSP.wetCompressors.size())
    {
        jassertfalse;
        return;
    }

    // Post-sum peak catcher: transparent limiter that only tames chorus peaks
    // in the final dry+wet mix. Dry signal stays untouched at normal levels.
    using Comp = ChorusDSP::WetCompressorState;
    constexpr float kneeHalfDb = Comp::kneeDb * 0.5f;
    constexpr float threshLow = Comp::thresholdDb - kneeHalfDb;
    constexpr float threshHigh = Comp::thresholdDb + kneeHalfDb;
    const float threshLowLin = std::pow(10.0f, threshLow / 20.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        auto& comp = chorusDSP.wetCompressors[static_cast<size_t>(ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = data[i];

            const float inputAbs = std::abs(sample);
            if (inputAbs > comp.envelope)
                comp.envelope = comp.attackCoeff * comp.envelope
                    + (1.0f - comp.attackCoeff) * inputAbs;
            else
                comp.envelope = comp.releaseCoeff * comp.envelope
                    + (1.0f - comp.releaseCoeff) * inputAbs;

            if (comp.envelope > threshLowLin)
            {
                const float envDb = (comp.envelope > 1.0e-10f)
                    ? 20.0f * std::log10(comp.envelope)
                    : -200.0f;

                float gainReductionDb = 0.0f;
                if (envDb > threshHigh)
                {
                    gainReductionDb =
                        (envDb - Comp::thresholdDb) * (1.0f - 1.0f / Comp::ratio);
                }
                else if (envDb > threshLow)
                {
                    const float x = envDb - threshLow;
                    gainReductionDb =
                        (1.0f - 1.0f / Comp::ratio) * x * x / (2.0f * Comp::kneeDb);
                }

                if (gainReductionDb > 0.0f)
                {
                    const float gainLin = std::pow(10.0f, -gainReductionDb / 20.0f);
                    sample *= gainLin;
                }
            }

            data[i] = sample;
        }
    }
}

void ChorusDSPProcess::processOutputTrim(ChorusDSP& chorusDSP,
                                         juce::dsp::AudioBlock<float>& block)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());
    if (numChannels <= 0 || numSamples <= 0)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float trimGain = chorusDSP.smoothedOutputTrimGain.getNextValue();
            data[i] *= trimGain;
        }
        chorusDSP.smoothedOutputTrimGain.skip(numSamples - 1);
    }
}

void ChorusDSPProcess::processChorusParameters(ChorusDSP& chorusDSP, int blockNumSamples, float& currentDepth, float& currentRate, float& currentCentreDelayMs)
{
    // Smooth the raw 0-1 depth first so engine switches do not inject a step
    // into the smoother just because the engine-specific mapping changed.
    const float rawDepth = chorusDSP.depth;
    auto* rawDepthPerSample = chorusDSP.rawDepthPerSampleBuffer.getWritePointer(0);
    auto* colorPerSample = chorusDSP.colorPerSampleBuffer.getWritePointer(0);
    auto* centreDelayPerSample = chorusDSP.centreDelayPerSampleMsBuffer.getWritePointer(0);
    auto* modulationDepthPerSample = chorusDSP.modulationDepthPerSampleBuffer.getWritePointer(0);

    const bool modular = chorusDSP.isModularCoreModeEnabled();
    const auto& descriptor = chorusDSP.getCurrentCoreDescriptor();
    float mappedDepth = chorusDSP.mapDepthToEngineRange(chorusDSP.smoothedDepthValue);
    float mappedColor = chorusDSP.mapColorToEngineRange(chorusDSP.smoothedColor.getCurrentValue());

    currentRate = chorusDSP.smoothedRate.getNextValue();
    chorusDSP.smoothedRate.skip(blockNumSamples - 1);

    // Smooth wet mix to avoid zipper/pops during engine/profile transitions.
    const float currentMix = chorusDSP.smoothedMix.getNextValue();
    chorusDSP.smoothedMix.skip(blockNumSamples - 1);
    chorusDSP.dryWet.setWetMixProportion(currentMix);

    const auto& tuning = chorusDSP.runtimeTuningSnapshot;
    for (int i = 0; i < blockNumSamples; ++i)
    {
        const float targetDiff = rawDepth - chorusDSP.currentDepthTarget;
        const float maxChange = chorusDSP.depthRateLimitPerSample;
        if (std::abs(targetDiff) > maxChange)
            chorusDSP.currentDepthTarget += (targetDiff > 0.0f ? maxChange : -maxChange);
        else
            chorusDSP.currentDepthTarget = rawDepth;

        chorusDSP.smoothedDepthValue =
            chorusDSP.depthSmoothingCoeff * chorusDSP.smoothedDepthValue
            + (1.0f - chorusDSP.depthSmoothingCoeff) * chorusDSP.currentDepthTarget;
        rawDepthPerSample[i] = chorusDSP.smoothedDepthValue;

        mappedDepth = chorusDSP.mapDepthToEngineRange(chorusDSP.smoothedDepthValue);
        mappedColor = chorusDSP.mapColorToEngineRange(chorusDSP.smoothedColor.getNextValue());
        colorPerSample[i] = mappedColor;

        float depthForDelay = mappedDepth;
        if ((modular && descriptor.bloomDepthScale) || (!modular && chorusDSP.currentColorIndex == 0))
        {
            const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
            const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, mappedColor), bloomExp);
            depthForDelay *= (1.0f + juce::jmax(0.0f, tuning.greenBloomDepthScale) * bloom);
        }
        modulationDepthPerSample[i] = depthForDelay;

        float centreDelayMs = chorusDSP.calculateCentreDelay(depthForDelay);
        if ((modular && descriptor.bloomCentreOffset) || (!modular && chorusDSP.currentColorIndex == 0))
        {
            const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
            const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, mappedColor), bloomExp);
            centreDelayMs += juce::jmax(0.0f, tuning.greenBloomCentreOffsetMs) * bloom;
        }

        chorusDSP.smoothedCentreDelay.setTargetValue(centreDelayMs);
        centreDelayPerSample[i] = chorusDSP.smoothedCentreDelay.getNextValue();
        currentCentreDelayMs = centreDelayPerSample[i];
    }

    chorusDSP.activeRawDepthPerSample = rawDepthPerSample;
    chorusDSP.activeColorPerSample = colorPerSample;
    chorusDSP.activeCentreDelayMsPerSample = centreDelayPerSample;
    chorusDSP.activeModulationDepthPerSample = modulationDepthPerSample;
    chorusDSP.colorBlockValue = mappedColor;
    currentDepth = modulationDepthPerSample[blockNumSamples - 1];
}

void ChorusDSPProcess::processChorusLFO(ChorusDSP& chorusDSP, int blockNumSamples, int numChannels, float currentRate, float currentDepth)
{
    chorusDSP.lfo.setFrequency(currentRate);
    chorusDSP.lfoCos.setFrequency(currentRate);
    const float* modulationDepthPerSample = chorusDSP.getModulationDepthPerSample(blockNumSamples);
    chorusDSP.oscVolume.setCurrentAndTargetValue(currentDepth * 0.5f);
    
    auto lfoBlock = juce::dsp::AudioBlock<float>(chorusDSP.lfoBuffer.getArrayOfWritePointers(), 1, blockNumSamples);
    auto lfoContext = juce::dsp::ProcessContextReplacing<float>(lfoBlock);
    lfoBlock.clear();
    chorusDSP.lfo.process(lfoContext);
    
    if (numChannels >= 2)
    {
        auto cosBlock = juce::dsp::AudioBlock<float>(chorusDSP.cosBuffer.getArrayOfWritePointers(), 1, blockNumSamples);
        auto cosContext = juce::dsp::ProcessContextReplacing<float>(cosBlock);
        cosBlock.clear();
        chorusDSP.lfoCos.process(cosContext);
        auto* lfoLeft = chorusDSP.lfoBuffer.getWritePointer(0);
        auto* cosSamples = chorusDSP.cosBuffer.getWritePointer(0);
        float lastRawSin = 0.0f;
        float lastRawCos = 1.0f;

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float modDepth = modulationDepthPerSample != nullptr ? modulationDepthPerSample[i] : currentDepth;
            const float amp = juce::jlimit(0.0f, 1.0f, modDepth) * 0.5f;
            lfoLeft[i] *= amp;
            cosSamples[i] *= amp;

            lastRawSin = lfoLeft[i];
            lastRawCos = cosSamples[i];
        }

        if (blockNumSamples > 0)
        {
            const float amplitude = std::sqrt(lastRawSin * lastRawSin + lastRawCos * lastRawCos);
            if (amplitude > 1.0e-9f)
                chorusDSP.lastBaseLfoPhaseRad = std::atan2(lastRawSin, lastRawCos);
            chorusDSP.lastLfoAmplitude = amplitude;
        }
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            // Consume offset smoother per-sample so phase offset transitions are truly continuous,
            // eliminating residual block-step zippering on sensitive engines (e.g. Black NQ).
            const float phaseOffsetDeg = chorusDSP.smoothedOffset.getNextValue();
            const float phaseOffsetRad = phaseOffsetDeg * juce::MathConstants<float>::pi / 180.0f;
            const float cosOffset = std::cos(phaseOffsetRad);
            const float sinOffset = std::sin(phaseOffsetRad);
            cosSamples[i] = lfoLeft[i] * cosOffset + cosSamples[i] * sinOffset;
            chorusDSP.lfoPhaseOffset = phaseOffsetDeg;
        }
    }
    else if (blockNumSamples > 0)
    {
        auto* lfoLeft = chorusDSP.lfoBuffer.getWritePointer(0);
        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float modDepth = modulationDepthPerSample != nullptr ? modulationDepthPerSample[i] : currentDepth;
            const float amp = juce::jlimit(0.0f, 1.0f, modDepth) * 0.5f;
            lfoLeft[i] *= amp;
        }
        const float currentOffset = chorusDSP.smoothedOffset.getNextValue();
        chorusDSP.smoothedOffset.skip(blockNumSamples - 1);
        chorusDSP.lfoPhaseOffset = currentOffset;
        const float lastSin = chorusDSP.lfoBuffer.getSample(0, blockNumSamples - 1);
        chorusDSP.lastBaseLfoPhaseRad = (lastSin >= 0.0f)
            ? juce::MathConstants<float>::halfPi
            : -juce::MathConstants<float>::halfPi;
        chorusDSP.lastLfoAmplitude = std::abs(lastSin);
    }
}

void ChorusDSPProcess::processChorusDelay(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    // Delegate to the current core
    if (chorusDSP.currentCore)
        chorusDSP.currentCore->processDelay(chorusDSP, block, currentCentreDelayMs);
}

void ChorusDSPProcess::processChorus(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
#if CHOROBOROS_PERFETTO_ENABLED
    TRACE_DSP();
#endif

    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    jassert(blockNumSamples <= chorusDSP.maxBlockSize);

    struct SelectionState
    {
        int colorIndex = 0;
        bool qualityHQ = false;
        bool modularMode = false;
        choroboros::CoreId coreId = choroboros::CoreId::lagrange3;
    };

    const auto depthForSelection = [&chorusDSP](const SelectionState& selection, float rawDepth) -> float
    {
        if (selection.modularMode)
        {
            if (choroboros::descriptorForCore(selection.coreId).depthCompression)
                return rawDepth * 0.45f;
            return rawDepth;
        }

        if (selection.colorIndex == 3)
            return rawDepth * 0.45f;

        return rawDepth;
    };

    const auto centreDelayForSelection = [&chorusDSP, &depthForSelection](const SelectionState& selection,
                                                                          float rawDepth,
                                                                          float colorValue) -> float
    {
        const auto& tuning = chorusDSP.runtimeTuningSnapshot;
        float mappedDepth = depthForSelection(selection, rawDepth);

        const bool bloomDepth =
            selection.modularMode
                ? choroboros::descriptorForCore(selection.coreId).bloomDepthScale
                : (selection.colorIndex == 0);

        if (bloomDepth)
        {
            const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
            const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, colorValue), bloomExp);
            mappedDepth *= (1.0f + juce::jmax(0.0f, tuning.greenBloomDepthScale) * bloom);
        }

        float centreDelayMs = chorusDSP.calculateCentreDelay(mappedDepth);

        const bool bloomOffset =
            selection.modularMode
                ? choroboros::descriptorForCore(selection.coreId).bloomCentreOffset
                : (selection.colorIndex == 0);

        if (bloomOffset)
        {
            const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
            const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, colorValue), bloomExp);
            centreDelayMs += juce::jmax(0.0f, tuning.greenBloomCentreOffsetMs) * bloom;
        }

        return centreDelayMs;
    };

    const auto withSelectionContext = [&chorusDSP](const SelectionState& selection,
                                                   float scopedRate,
                                                   float scopedColor,
                                                   auto&& fn)
    {
        const int savedColorIndex = chorusDSP.currentColorIndex;
        const bool savedQualityHQ = chorusDSP.currentQualityHQ;
        const bool savedModularMode = chorusDSP.modularCoreModeEnabled;
        const auto savedCoreId = chorusDSP.currentCoreId;
        const float savedRateCurrent = chorusDSP.smoothedRate.getCurrentValue();
        const float savedRateTarget = chorusDSP.smoothedRate.getTargetValue();
        const float savedColorCurrent = chorusDSP.smoothedColor.getCurrentValue();
        const float savedColorTarget = chorusDSP.smoothedColor.getTargetValue();
        const float savedColorBlock = chorusDSP.colorBlockValue;
        const float* savedColourPerSample = chorusDSP.activeColorPerSample;
        const float* savedCentreDelayPerSample = chorusDSP.activeCentreDelayMsPerSample;

        chorusDSP.currentColorIndex = selection.colorIndex;
        chorusDSP.currentQualityHQ = selection.qualityHQ;
        chorusDSP.modularCoreModeEnabled = selection.modularMode;
        chorusDSP.currentCoreId = selection.coreId;
        chorusDSP.smoothedRate.setCurrentAndTargetValue(scopedRate);
        chorusDSP.smoothedColor.setCurrentAndTargetValue(scopedColor);
        chorusDSP.colorBlockValue = scopedColor;
        chorusDSP.activeColorPerSample = nullptr;
        chorusDSP.activeCentreDelayMsPerSample = nullptr;

        fn();

        chorusDSP.currentColorIndex = savedColorIndex;
        chorusDSP.currentQualityHQ = savedQualityHQ;
        chorusDSP.modularCoreModeEnabled = savedModularMode;
        chorusDSP.currentCoreId = savedCoreId;
        chorusDSP.smoothedRate.setCurrentAndTargetValue(savedRateCurrent);
        chorusDSP.smoothedRate.setTargetValue(savedRateTarget);
        chorusDSP.smoothedColor.setCurrentAndTargetValue(savedColorCurrent);
        chorusDSP.smoothedColor.setTargetValue(savedColorTarget);
        chorusDSP.colorBlockValue = savedColorBlock;
        chorusDSP.activeColorPerSample = savedColourPerSample;
        chorusDSP.activeCentreDelayMsPerSample = savedCentreDelayPerSample;
    };

    const auto resetWetCharacterState = [](ChorusDSP::WetCharacterState& state)
    {
        std::fill(state.greenWetLPState.begin(), state.greenWetLPState.end(), 0.0f);
        std::fill(state.blueWetHPState.begin(), state.blueWetHPState.end(), 0.0f);
        std::fill(state.blueWetLPState.begin(), state.blueWetLPState.end(), 0.0f);
        for (auto& biquad : state.bluePresenceState)
            biquad = {};
        state.bluePresenceB0 = 1.0f;
        state.bluePresenceB1 = 0.0f;
        state.bluePresenceB2 = 0.0f;
        state.bluePresenceA1 = 0.0f;
        state.bluePresenceA2 = 0.0f;
        state.bluePresenceCachedFreqHz = -1.0f;
        state.bluePresenceCachedQ = -1.0f;
        state.bluePresenceCachedGainDb = -1000.0f;
    };
    
    float currentDepth, currentRate, currentCentreDelayMs;
    processChorusParameters(chorusDSP, blockNumSamples, currentDepth, currentRate, currentCentreDelayMs);
    processChorusLFO(chorusDSP, blockNumSamples, numChannels, currentRate, currentDepth);

    const SelectionState pendingSelection
    {
        chorusDSP.pendingColorIndex,
        chorusDSP.pendingQualityHQ,
        chorusDSP.pendingModularCoreModeEnabled,
        chorusDSP.pendingCoreId
    };
    const SelectionState previousSelection
    {
        chorusDSP.coreSwitchOldColorIndex,
        chorusDSP.coreSwitchOldQualityHQ,
        chorusDSP.coreSwitchOldModularCoreModeEnabled,
        chorusDSP.previousCoreId
    };
    const auto& pendingDesc = chorusDSP.getCorePackageDescriptor(chorusDSP.pendingCoreId);
    
    chorusDSP.dryWet.pushDrySamples(block);

    bool pendingCoreReady = false;
    if (chorusDSP.pendingCore != nullptr)
    {
        jassert(blockNumSamples <= chorusDSP.maxBlockSize);
        jassert(numChannels <= static_cast<int>(chorusDSP.coreCrossfadeBufferA.getNumChannels()));

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* src = block.getChannelPointer(ch);
            chorusDSP.coreCrossfadeBufferA.copyFrom(ch, 0, src, blockNumSamples);
        }

        auto wetPending = juce::dsp::AudioBlock<float>(chorusDSP.coreCrossfadeBufferA.getArrayOfWritePointers(),
                                                       static_cast<size_t>(numChannels),
                                                       static_cast<size_t>(blockNumSamples));
        const float pendingCentreDelayMs = centreDelayForSelection(pendingSelection,
                                                                   chorusDSP.smoothedDepthValue,
                                                                   chorusDSP.colorBlockValue);
        withSelectionContext(pendingSelection, currentRate, chorusDSP.colorBlockValue, [&]
        {
            processPreEmphasis(chorusDSP,
                               wetPending,
                               chorusDSP.pendingPreEmphasis,
                               chorusDSP.pendingInputLevel);
            chorusDSP.pendingCore->processDelay(chorusDSP, wetPending, pendingCentreDelayMs);
            processWetCharacter(chorusDSP,
                                wetPending,
                                pendingDesc,
                                chorusDSP.pendingColorIndex,
                                chorusDSP.pendingWetCharacterState);
            processPostChorusSaturation(chorusDSP,
                                        wetPending,
                                        pendingDesc,
                                        chorusDSP.pendingColorIndex);
        });

        if (chorusDSP.coreSwitchWarmupSamplesRemaining > 0)
            chorusDSP.coreSwitchWarmupSamplesRemaining = juce::jmax(0, chorusDSP.coreSwitchWarmupSamplesRemaining - blockNumSamples);

        pendingCoreReady = (chorusDSP.coreSwitchWarmupSamplesRemaining <= 0);
    }

    bool applyWetCharacterShared = false;
    bool applySaturationShared = false;
    const choroboros::CorePackageDescriptor* sharedDesc = nullptr;
    int sharedColorIndex = chorusDSP.currentColorIndex;

    if (chorusDSP.coreSwitchCrossfadeActive && chorusDSP.previousCore != nullptr)
    {
        jassert(blockNumSamples <= chorusDSP.maxBlockSize);
        jassert(numChannels <= static_cast<int>(chorusDSP.coreCrossfadeBufferA.getNumChannels()));
        jassert(numChannels <= static_cast<int>(chorusDSP.coreCrossfadeBufferB.getNumChannels()));

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* src = block.getChannelPointer(ch);
            chorusDSP.coreCrossfadeBufferA.copyFrom(ch, 0, src, blockNumSamples);
            chorusDSP.coreCrossfadeBufferB.copyFrom(ch, 0, src, blockNumSamples);
        }

        auto wetCurrent = juce::dsp::AudioBlock<float>(chorusDSP.coreCrossfadeBufferA.getArrayOfWritePointers(),
                                                       static_cast<size_t>(numChannels),
                                                       static_cast<size_t>(blockNumSamples));
        auto wetPrevious = juce::dsp::AudioBlock<float>(chorusDSP.coreCrossfadeBufferB.getArrayOfWritePointers(),
                                                        static_cast<size_t>(numChannels),
                                                        static_cast<size_t>(blockNumSamples));

        if (chorusDSP.currentCore != nullptr)
        {
            processPreEmphasis(chorusDSP,
                               wetCurrent,
                               chorusDSP.preEmphasis,
                               chorusDSP.inputLevel);
            chorusDSP.currentCore->processDelay(chorusDSP, wetCurrent, currentCentreDelayMs);
        }

        if (chorusDSP.coreSwitchOldParamsSnapshotValid)
        {
            auto* oldLfo = chorusDSP.lfoBuffer.getWritePointer(0);
            auto* oldCos = chorusDSP.cosBuffer.getWritePointer(0);
            const float oldRate = juce::jmax(0.0f, chorusDSP.coreSwitchOldRateHz);
            const float phaseInc = juce::MathConstants<float>::twoPi * oldRate
                                   / static_cast<float>(juce::jmax(1.0, chorusDSP.spec.sampleRate));
            float phase = chorusDSP.coreSwitchOldBasePhaseRad;
            const float amp = chorusDSP.coreSwitchOldLfoAmplitude;
            const float oldOffsetRad = chorusDSP.coreSwitchOldOffsetDegrees
                                       * juce::MathConstants<float>::pi / 180.0f;
            for (int i = 0; i < blockNumSamples; ++i)
            {
                phase += phaseInc;
                if (phase >= juce::MathConstants<float>::twoPi)
                    phase -= juce::MathConstants<float>::twoPi;
                else if (phase < 0.0f)
                    phase += juce::MathConstants<float>::twoPi;

                oldLfo[i] = amp * std::sin(phase);
                oldCos[i] = amp * std::sin(phase + oldOffsetRad);
            }
            chorusDSP.coreSwitchOldBasePhaseRad = phase;

            withSelectionContext(previousSelection, chorusDSP.coreSwitchOldRateHz, chorusDSP.coreSwitchOldColor, [&]
            {
                processPreEmphasis(chorusDSP,
                                   wetPrevious,
                                   chorusDSP.previousPreEmphasis,
                                   chorusDSP.previousInputLevel);
                chorusDSP.previousCore->processDelay(chorusDSP, wetPrevious, chorusDSP.coreSwitchOldCentreDelayMs);
            });
        }
        else
        {
            withSelectionContext(previousSelection, currentRate, chorusDSP.colorBlockValue, [&]
            {
                processPreEmphasis(chorusDSP,
                                   wetPrevious,
                                   chorusDSP.previousPreEmphasis,
                                   chorusDSP.previousInputLevel);
                chorusDSP.previousCore->processDelay(chorusDSP, wetPrevious, currentCentreDelayMs);
            });
        }

        sharedDesc = &chorusDSP.getCurrentCoreDescriptor();
        const auto& prevDesc = chorusDSP.getCorePackageDescriptor(chorusDSP.previousCoreId);

        // During an active core handoff, do not share post-core character stages across
        // the mixed branches. Even when the logical macro is "the same", the internal
        // state of the old and new branches differs, and applying one shared stage to
        // the summed signal can reintroduce a click right at the switch boundary.
        applyWetCharacterShared = false;
        applySaturationShared = false;

        if (!applyWetCharacterShared) {
            withSelectionContext(previousSelection, chorusDSP.coreSwitchOldRateHz, chorusDSP.coreSwitchOldColor, [&]
            {
                processWetCharacter(chorusDSP,
                                    wetPrevious,
                                    prevDesc,
                                    chorusDSP.coreSwitchOldColorIndex,
                                    chorusDSP.previousWetCharacterState);
            });
            processWetCharacter(chorusDSP,
                                wetCurrent,
                                *sharedDesc,
                                chorusDSP.currentColorIndex,
                                chorusDSP.wetCharacterState);
        }
        if (!applySaturationShared) {
            withSelectionContext(previousSelection, chorusDSP.coreSwitchOldRateHz, chorusDSP.coreSwitchOldColor, [&]
            {
                processPostChorusSaturation(chorusDSP, wetPrevious, prevDesc, chorusDSP.coreSwitchOldColorIndex);
            });
            processPostChorusSaturation(chorusDSP, wetCurrent, *sharedDesc, chorusDSP.currentColorIndex);
        }

        const int totalSamples = juce::jmax(1, chorusDSP.coreSwitchCrossfadeTotalSamples);
        int remaining = chorusDSP.coreSwitchCrossfadeSamplesRemaining;

        const float currentCoreTrim = (chorusDSP.currentCore != nullptr)
            ? chorusDSP.currentCore->getOutputTrim() : 1.0f;
        const float previousCoreTrim = (chorusDSP.previousCore != nullptr)
            ? chorusDSP.previousCore->getOutputTrim() : 1.0f;

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float progress = 1.0f - (static_cast<float>(juce::jmax(remaining, 0)) / static_cast<float>(totalSamples));
            // Bias the transition toward the old core during early samples so stale-state transients
            // in the new core stay masked while its delay memory settles.
            constexpr float crossfadeCurveExp = 1.8f;
            const float shapedProgress = std::pow(progress, crossfadeCurveExp);
            const float newGain = std::sin(shapedProgress * juce::MathConstants<float>::halfPi);
            const float oldGain = std::cos(shapedProgress * juce::MathConstants<float>::halfPi);
            // Gentle mid-crossfade duck: sin²(π·progress) is exactly 1.0 at
            // both boundaries (no level discontinuity) and dips ~0.5 dB at the
            // midpoint where two uncorrelated cores sum loudest.
            const float envelope = std::sin(progress * juce::MathConstants<float>::pi);
            const float duckGain = 1.0f - 0.06f * envelope * envelope;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float wetNew = chorusDSP.coreCrossfadeBufferA.getSample(ch, i)
                    * currentCoreTrim;
                const float wetOld = chorusDSP.coreCrossfadeBufferB.getSample(ch, i)
                    * previousCoreTrim;
                block.setSample(ch, i, (wetOld * oldGain + wetNew * newGain) * duckGain);
            }
            remaining = juce::jmax(remaining - 1, 0);
        }

        chorusDSP.coreSwitchCrossfadeSamplesRemaining = remaining;
        if (remaining <= 0)
        {
            chorusDSP.coreSwitchCrossfadeActive = false;
            chorusDSP.previousCore = nullptr;
            chorusDSP.coreSwitchCrossfadeSamplesRemaining = 0;
            chorusDSP.coreSwitchCrossfadeTotalSamples = 0;
            chorusDSP.coreSwitchTargetCrossfadeSamples = 0;
            chorusDSP.coreSwitchOldParamsSnapshotValid = false;
        }
    }
    else
    {
        processPreEmphasis(chorusDSP,
                           block,
                           chorusDSP.preEmphasis,
                           chorusDSP.inputLevel);
        processChorusDelay(chorusDSP, block, currentCentreDelayMs);
        sharedDesc = &chorusDSP.getCorePackageDescriptor(chorusDSP.currentCoreId);
        sharedColorIndex = chorusDSP.currentColorIndex;
        applyWetCharacterShared = true;
        applySaturationShared = true;
    }

    const bool promotePendingCore =
        !chorusDSP.coreSwitchCrossfadeActive && pendingCoreReady && chorusDSP.pendingCore != nullptr;

    // Apply wet-character (Green/Blue) and Red NQ saturation before dry/wet mix.
    if (applyWetCharacterShared && sharedDesc != nullptr)
        processWetCharacter(chorusDSP,
                            block,
                            *sharedDesc,
                            sharedColorIndex,
                            chorusDSP.wetCharacterState);
        
    if (applySaturationShared && sharedDesc != nullptr)
        processPostChorusSaturation(chorusDSP, block, *sharedDesc, sharedColorIndex);

    chorusDSP.dryWet.mixWetSamples(block);
    processOutputPeakCatch(chorusDSP, block);
    processOutputTrim(chorusDSP, block);

    if (promotePendingCore)
    {
        // Snapshot the outgoing branch at the actual handoff point, not at the
        // original switch request time. Warmup can span many blocks, so using
        // the older phase/centre-delay snapshot makes the "old" branch resume
        // from stale modulation state and creates an audible boundary click
        // when the crossfade begins.
        chorusDSP.coreSwitchOldParamsSnapshotValid = true;
        chorusDSP.coreSwitchOldColorIndex = chorusDSP.currentColorIndex;
        chorusDSP.coreSwitchOldQualityHQ = chorusDSP.currentQualityHQ;
        chorusDSP.coreSwitchOldModularCoreModeEnabled = chorusDSP.modularCoreModeEnabled;
        chorusDSP.coreSwitchOldRateHz = currentRate;
        chorusDSP.coreSwitchOldDepth = chorusDSP.smoothedDepthValue;
        chorusDSP.coreSwitchOldCentreDelayMs = currentCentreDelayMs;
        chorusDSP.coreSwitchOldColor = chorusDSP.colorBlockValue;
        chorusDSP.coreSwitchOldOffsetDegrees = chorusDSP.lfoPhaseOffset;
        chorusDSP.coreSwitchOldBasePhaseRad = chorusDSP.lastBaseLfoPhaseRad;
        chorusDSP.coreSwitchOldLfoAmplitude = chorusDSP.lastLfoAmplitude;

        chorusDSP.previousCore = chorusDSP.currentCore;
        chorusDSP.previousCoreId = chorusDSP.currentCoreId;
        chorusDSP.previousInputLevel = chorusDSP.inputLevel;
        chorusDSP.previousWetCharacterState = chorusDSP.wetCharacterState;
        std::swap(chorusDSP.previousPreEmphasis, chorusDSP.preEmphasis);
        chorusDSP.currentCore = chorusDSP.pendingCore;
        chorusDSP.currentCoreId = chorusDSP.pendingCoreId;
        chorusDSP.currentColorIndex = chorusDSP.pendingColorIndex;
        chorusDSP.currentQualityHQ = chorusDSP.pendingQualityHQ;
        chorusDSP.modularCoreModeEnabled = chorusDSP.pendingModularCoreModeEnabled;
        chorusDSP.inputLevel = chorusDSP.pendingInputLevel;
        std::swap(chorusDSP.preEmphasis, chorusDSP.pendingPreEmphasis);
        chorusDSP.pendingPreEmphasis.reset();
        chorusDSP.pendingCore = nullptr;
        chorusDSP.coreSwitchWarmupSamplesRemaining = 0;
        chorusDSP.coreSwitchWarmupTotalSamples = 0;
        chorusDSP.wetCharacterState = chorusDSP.pendingWetCharacterState;
        resetWetCharacterState(chorusDSP.pendingWetCharacterState);

        if (chorusDSP.previousCore != nullptr && chorusDSP.spec.sampleRate > 0.0)
        {
            const int requestedCrossfade = juce::jmax(1, chorusDSP.coreSwitchTargetCrossfadeSamples);
            chorusDSP.coreSwitchCrossfadeTotalSamples = requestedCrossfade;
            chorusDSP.coreSwitchCrossfadeSamplesRemaining = chorusDSP.coreSwitchCrossfadeTotalSamples;
            chorusDSP.coreSwitchCrossfadeActive = true;
        }
        else
        {
            chorusDSP.previousCore = nullptr;
            chorusDSP.coreSwitchCrossfadeActive = false;
            chorusDSP.coreSwitchCrossfadeSamplesRemaining = 0;
            chorusDSP.coreSwitchCrossfadeTotalSamples = 0;
            chorusDSP.coreSwitchTargetCrossfadeSamples = 0;
            chorusDSP.coreSwitchOldParamsSnapshotValid = false;
        }
    }
}
