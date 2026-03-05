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

void ChorusDSPProcess::processPreEmphasis(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
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
    chorusDSP.inputLevel = levelSmoothing * chorusDSP.inputLevel + (1.0f - levelSmoothing) * rmsLevel;
    
    const float quietThreshold = tuning.preEmphasisQuietThreshold;
    float preEmphAmount = 0.0f;
    if (quietThreshold > 0.0f && chorusDSP.inputLevel < quietThreshold)
        preEmphAmount = (quietThreshold - chorusDSP.inputLevel) / quietThreshold * tuning.preEmphasisMaxAmount;
    
    if (preEmphAmount > 0.0f)
    {
        jassert(blockSize <= chorusDSP.maxBlockSize);
        for (int ch = 0; ch < block.getNumChannels(); ++ch)
            chorusDSP.preEmphOriginalBuffer.copyFrom(ch, 0, block.getChannelPointer(ch), blockSize);
        
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        chorusDSP.preEmphasis.process(context);
        
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

void ChorusDSPProcess::processWetCharacter(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
    // Color drives wet-only character macros:
    // Green => Bloom, Blue => Focus, others => handled elsewhere.
    const float currentColor = juce::jlimit(0.0f, 1.0f, chorusDSP.colorBlockValue);
    if (chorusDSP.isModularCoreModeEnabled())
    {
        const auto& descriptor = chorusDSP.getCurrentCoreDescriptor();
        if (descriptor.bloomWetCharacter)
        {
            chorusDSP.processGreenBloomWet(block, currentColor);
            return;
        }

        if (descriptor.focusWetCharacter)
        {
            chorusDSP.processBlueFocusWet(block, currentColor);
            return;
        }

        return;
    }

    if (chorusDSP.currentColorIndex == 0)
    {
        chorusDSP.processGreenBloomWet(block, currentColor);
        return;
    }

    if (chorusDSP.currentColorIndex == 1)
    {
        chorusDSP.processBlueFocusWet(block, currentColor);
        return;
    }
}

void ChorusDSPProcess::processPostChorusSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
    bool usesPostSaturation = false;
    if (chorusDSP.isModularCoreModeEnabled())
    {
        usesPostSaturation = chorusDSP.getCurrentCoreDescriptor().postChorusSaturation;
    }
    else
    {
        // Red NQ is the only legacy engine where Color is post-chorus saturation.
        const int engine = chorusDSP.currentColorIndex;
        usesPostSaturation = (engine == 2 && !chorusDSP.currentQualityHQ);
    }

    if (!usesPostSaturation)
        return;

    const int numSamples = static_cast<int>(block.getNumSamples());
    const float currentColor = juce::jlimit(0.0f, 1.0f, chorusDSP.colorBlockValue);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            data[i] = chorusDSP.applySaturation(data[i], currentColor);
        }
    }
}

void ChorusDSPProcess::processChorusParameters(ChorusDSP& chorusDSP, int blockNumSamples, float& currentDepth, float& currentRate, float& currentCentreDelayMs)
{
    // Map depth to engine-specific range (Purple uses compressed range)
    float mappedDepth = chorusDSP.mapDepthToEngineRange(chorusDSP.depth);
    
    float targetDiff = mappedDepth - chorusDSP.currentDepthTarget;
    float maxChange = chorusDSP.depthRateLimit * (blockNumSamples / chorusDSP.spec.sampleRate);
    if (std::abs(targetDiff) > maxChange)
        chorusDSP.currentDepthTarget += (targetDiff > 0.0f ? maxChange : -maxChange);
    else
        chorusDSP.currentDepthTarget = mappedDepth;
    
    float aN = std::pow(chorusDSP.depthSmoothingCoeff, static_cast<float>(blockNumSamples));
    currentDepth = aN * chorusDSP.smoothedDepthValue + (1.0f - aN) * chorusDSP.currentDepthTarget;
    chorusDSP.smoothedDepthValue = currentDepth;
    
    currentRate = chorusDSP.smoothedRate.getNextValue();
    chorusDSP.smoothedRate.skip(blockNumSamples - 1);

    // Smooth wet mix to avoid zipper/pops during engine/profile transitions.
    const float currentMix = chorusDSP.smoothedMix.getNextValue();
    chorusDSP.smoothedMix.skip(blockNumSamples - 1);
    chorusDSP.dryWet.setWetMixProportion(currentMix);

    // Advance Color smoothing once per block for all engines and use block-constant value.
    // Some cores (e.g. Red HQ Tape) read smoothedColor directly in processDelay.
    const float currentColor = chorusDSP.smoothedColor.getNextValue();
    chorusDSP.smoothedColor.skip(blockNumSamples - 1);
    chorusDSP.colorBlockValue = currentColor;

    const bool modular = chorusDSP.isModularCoreModeEnabled();
    const auto& descriptor = chorusDSP.getCurrentCoreDescriptor();

    // Bloom behavior can now follow core package semantics in modular mode.
    if ((modular && descriptor.bloomDepthScale) || (!modular && chorusDSP.currentColorIndex == 0))
    {
        const auto& tuning = chorusDSP.runtimeTuningSnapshot;
        const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
        const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, currentColor), bloomExp);
        currentDepth *= (1.0f + juce::jmax(0.0f, tuning.greenBloomDepthScale) * bloom);
    }
    
    float centreDelayMs = chorusDSP.calculateCentreDelay(currentDepth);
    if ((modular && descriptor.bloomCentreOffset) || (!modular && chorusDSP.currentColorIndex == 0))
    {
        const auto& tuning = chorusDSP.runtimeTuningSnapshot;
        const float bloomExp = juce::jmax(0.1f, tuning.greenBloomExponent);
        const float bloom = std::pow(juce::jlimit(0.0f, 1.0f, currentColor), bloomExp);
        centreDelayMs += juce::jmax(0.0f, tuning.greenBloomCentreOffsetMs) * bloom;
    }
    chorusDSP.smoothedCentreDelay.setTargetValue(centreDelayMs);
    currentCentreDelayMs = chorusDSP.smoothedCentreDelay.getNextValue();
    chorusDSP.smoothedCentreDelay.skip(blockNumSamples - 1);
}

void ChorusDSPProcess::processChorusLFO(ChorusDSP& chorusDSP, int blockNumSamples, int numChannels, float currentRate, float currentDepth)
{
    chorusDSP.lfo.setFrequency(currentRate);
    chorusDSP.lfoCos.setFrequency(currentRate);
    chorusDSP.oscVolume.setTargetValue(currentDepth * 0.5f);
    
    auto lfoBlock = juce::dsp::AudioBlock<float>(chorusDSP.lfoBuffer.getArrayOfWritePointers(), 1, blockNumSamples);
    auto lfoContext = juce::dsp::ProcessContextReplacing<float>(lfoBlock);
    lfoBlock.clear();
    chorusDSP.lfo.process(lfoContext);
    lfoBlock.multiplyBy(chorusDSP.oscVolume);
    
    if (numChannels >= 2)
    {
        auto cosBlock = juce::dsp::AudioBlock<float>(chorusDSP.cosBuffer.getArrayOfWritePointers(), 1, blockNumSamples);
        auto cosContext = juce::dsp::ProcessContextReplacing<float>(cosBlock);
        cosBlock.clear();
        chorusDSP.lfoCos.process(cosContext);
        cosBlock.multiplyBy(chorusDSP.oscVolume);

        if (blockNumSamples > 0)
        {
            const float lastSin = chorusDSP.lfoBuffer.getSample(0, blockNumSamples - 1);
            const float lastCos = chorusDSP.cosBuffer.getSample(0, blockNumSamples - 1);
            chorusDSP.lastBaseLfoPhaseRad = std::atan2(lastSin, lastCos);
            chorusDSP.lastLfoAmplitude = std::sqrt(lastSin * lastSin + lastCos * lastCos);
        }

        auto* lfoLeft = chorusDSP.lfoBuffer.getWritePointer(0);
        auto* cosSamples = chorusDSP.cosBuffer.getWritePointer(0);
        
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
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    jassert(blockNumSamples <= chorusDSP.maxBlockSize);
    
    float currentDepth, currentRate, currentCentreDelayMs;
    processChorusParameters(chorusDSP, blockNumSamples, currentDepth, currentRate, currentCentreDelayMs);
    processChorusLFO(chorusDSP, blockNumSamples, numChannels, currentRate, currentDepth);
    
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
        chorusDSP.pendingCore->processDelay(chorusDSP, wetPending, currentCentreDelayMs);

        if (chorusDSP.coreSwitchWarmupSamplesRemaining > 0)
            chorusDSP.coreSwitchWarmupSamplesRemaining = juce::jmax(0, chorusDSP.coreSwitchWarmupSamplesRemaining - blockNumSamples);

        pendingCoreReady = (chorusDSP.coreSwitchWarmupSamplesRemaining <= 0);
    }

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
            chorusDSP.currentCore->processDelay(chorusDSP, wetCurrent, currentCentreDelayMs);

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

            const float savedRateCurrent = chorusDSP.smoothedRate.getCurrentValue();
            const float savedRateTarget = chorusDSP.smoothedRate.getTargetValue();
            const float savedColorCurrent = chorusDSP.smoothedColor.getCurrentValue();
            const float savedColorTarget = chorusDSP.smoothedColor.getTargetValue();
            const float savedColorBlock = chorusDSP.colorBlockValue;

            chorusDSP.smoothedRate.setCurrentAndTargetValue(chorusDSP.coreSwitchOldRateHz);
            chorusDSP.smoothedColor.setCurrentAndTargetValue(chorusDSP.coreSwitchOldColor);
            chorusDSP.colorBlockValue = chorusDSP.coreSwitchOldColor;

            chorusDSP.previousCore->processDelay(chorusDSP, wetPrevious, chorusDSP.coreSwitchOldCentreDelayMs);

            chorusDSP.smoothedRate.setCurrentAndTargetValue(savedRateCurrent);
            chorusDSP.smoothedRate.setTargetValue(savedRateTarget);
            chorusDSP.smoothedColor.setCurrentAndTargetValue(savedColorCurrent);
            chorusDSP.smoothedColor.setTargetValue(savedColorTarget);
            chorusDSP.colorBlockValue = savedColorBlock;
        }
        else
        {
            chorusDSP.previousCore->processDelay(chorusDSP, wetPrevious, currentCentreDelayMs);
        }

        const int totalSamples = juce::jmax(1, chorusDSP.coreSwitchCrossfadeTotalSamples);
        int remaining = chorusDSP.coreSwitchCrossfadeSamplesRemaining;

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float progress = 1.0f - (static_cast<float>(juce::jmax(remaining, 0)) / static_cast<float>(totalSamples));
            // Bias the transition toward the old core during early samples so stale-state transients
            // in the new core stay masked while its delay memory settles.
            constexpr float crossfadeCurveExp = 1.8f;
            const float shapedProgress = std::pow(progress, crossfadeCurveExp);
            const float newGain = std::sin(shapedProgress * juce::MathConstants<float>::halfPi);
            const float oldGain = std::cos(shapedProgress * juce::MathConstants<float>::halfPi);
            // Extra edge de-click treatment: short attenuation at transition start/end
            // suppresses single-sample discontinuities when switching core states.
            constexpr float edgeWindow = 0.08f; // 8% of crossfade length
            const float edgeIn = juce::jlimit(0.0f, 1.0f, progress / edgeWindow);
            const float edgeOut = juce::jlimit(0.0f, 1.0f, (1.0f - progress) / edgeWindow);
            const float edgeBlend = juce::jmin(edgeIn, edgeOut);
            const float edgeDuckGain = 0.82f + 0.18f * edgeBlend;
            const float midDuckGain = 1.0f - 0.08f * std::sin(progress * juce::MathConstants<float>::pi);
            const float duckGain = edgeDuckGain * midDuckGain;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float wetNew = chorusDSP.coreCrossfadeBufferA.getSample(ch, i);
                const float wetOld = chorusDSP.coreCrossfadeBufferB.getSample(ch, i);
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
        processChorusDelay(chorusDSP, block, currentCentreDelayMs);
    }

    if (!chorusDSP.coreSwitchCrossfadeActive && pendingCoreReady && chorusDSP.pendingCore != nullptr)
    {
        chorusDSP.previousCore = chorusDSP.currentCore;
        chorusDSP.currentCore = chorusDSP.pendingCore;
        chorusDSP.currentCoreId = chorusDSP.pendingCoreId;
        chorusDSP.pendingCore = nullptr;
        chorusDSP.coreSwitchWarmupSamplesRemaining = 0;
        chorusDSP.coreSwitchWarmupTotalSamples = 0;

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

    // Apply wet-character (Green/Blue) and Red NQ saturation before dry/wet mix.
    processWetCharacter(chorusDSP, block);
    processPostChorusSaturation(chorusDSP, block);
    chorusDSP.dryWet.mixWetSamples(block);
}
