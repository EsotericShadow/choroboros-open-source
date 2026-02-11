#include "ChorusDSPProcess.h"
#include "ChorusDSP.h"
#include "../Cores/ChorusCore.h"

void ChorusDSPProcess::processPreEmphasis(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
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
    
    chorusDSP.inputLevel = 0.95f * chorusDSP.inputLevel + 0.05f * rmsLevel;
    
    const float quietThreshold = 0.125f;
    float preEmphAmount = 0.0f;
    if (chorusDSP.inputLevel < quietThreshold)
        preEmphAmount = (quietThreshold - chorusDSP.inputLevel) / quietThreshold * 0.5f;
    
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

void ChorusDSPProcess::processSaturation(ChorusDSP& chorusDSP, juce::dsp::AudioBlock<float>& block)
{
    const int numSamples = static_cast<int>(block.getNumSamples());
    float currentColor = chorusDSP.smoothedColor.getNextValue();
    chorusDSP.smoothedColor.skip(numSamples - 1);
    
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
    
    float centreDelayMs = chorusDSP.calculateCentreDelay(currentDepth);
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
        
        float phaseOffsetRad = chorusDSP.lfoPhaseOffset * juce::MathConstants<float>::pi / 180.0f;
        float cosOffset = std::cos(phaseOffsetRad);
        float sinOffset = std::sin(phaseOffsetRad);
        
        auto* lfoLeft = chorusDSP.lfoBuffer.getWritePointer(0);
        auto* cosSamples = chorusDSP.cosBuffer.getWritePointer(0);
        
        for (int i = 0; i < blockNumSamples; ++i)
            cosSamples[i] = lfoLeft[i] * cosOffset + cosSamples[i] * sinOffset;
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
    processChorusDelay(chorusDSP, block, currentCentreDelayMs);
    chorusDSP.dryWet.mixWetSamples(block);
}
