#include "ChorusCoreThiran.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>

int thiranProcessCallCount = 0;

ChorusCoreThiran::ChorusCoreThiran()
{
}

void ChorusCoreThiran::prepare(const juce::dsp::ProcessSpec& processSpec)
{
    spec = processSpec;
    
    // #region agent log - reset process counter
    {
        static int prepareCount = 0;
        prepareCount++;
        thiranProcessCallCount = 0;
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusCoreThiran::prepare\",\"message\":\"Windowed-sinc FIR core prepared\",\"data\":{\"prepareCount\":" << prepareCount << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    // Calculate maximum delay needed
    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 16; // Need guard for FIR taps
    
    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;
    
    // Round up to next power of 2
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + SincFD::TAPS)
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;
    
    // Allocate per-channel structures
    sincFilters.resize(static_cast<size_t>(spec.numChannels));
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    smoothedDelays.resize(static_cast<size_t>(spec.numChannels), 0.0f);
    delayInitialized.resize(static_cast<size_t>(spec.numChannels), false);
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        sincFilters[ch].buildTable(); // Build polyphase table
        smoothedDelays[ch] = 0.0f;
        delayInitialized[ch] = false;
    }
}

void ChorusCoreThiran::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    std::fill(smoothedDelays.begin(), smoothedDelays.end(), 0.0f);
    std::fill(delayInitialized.begin(), delayInitialized.end(), false);
}

float ChorusCoreThiran::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreThiran::readSinc(int channel, float delaySamples) const
{
    const auto& buffer = delayBuffers[static_cast<size_t>(channel)];
    const int wp = writePositions[static_cast<size_t>(channel)];
    const auto& sinc = sincFilters[static_cast<size_t>(channel)];
    
    return sinc.read(buffer.data(), bufferMask, wp, delaySamples);
}

void ChorusCoreThiran::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;
    
    // Access LFO buffers from ChorusDSP
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;
    
    // #region agent log
    static int logCounter = 0;
    thiranProcessCallCount++;
    bool shouldLog = (++logCounter % 100 == 0) || (thiranProcessCallCount <= 10); // Log first 10 calls + every 100th
    float sampleDelay = 0.0f;
    float sampleOut = 0.0f;
    // #endregion
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        float& dSmooth = smoothedDelays[static_cast<size_t>(ch)];
        
        // Initialize delay smoothing
        if (!delayInitialized[static_cast<size_t>(ch)])
        {
            float initialDelay = centreDelaySamples + depthSamples * channelLfo[0];
            initialDelay = juce::jlimit(guardSamples, maxDelaySamples, initialDelay);
            dSmooth = initialDelay;
            delayInitialized[static_cast<size_t>(ch)] = true;
        }
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];
            
            // Calculate target delay from LFO BEFORE writing
            // This ensures we read from the correct position relative to current write
            float targetDelay = centreDelaySamples + depthSamples * channelLfo[i];
            targetDelay = juce::jlimit(guardSamples, maxDelaySamples, targetDelay);
            
            // Smooth delay to prevent artifacts (fast one-pole, similar to tape core)
            constexpr float delaySmoothingCoeff = 0.998f; // ~5ms @ 48k
            dSmooth = delaySmoothingCoeff * dSmooth + (1.0f - delaySmoothingCoeff) * targetDelay;
            
            // Read with windowed-sinc polyphase FIR (read BEFORE writing)
            // This ensures we read from samples that were written earlier
            const float out = readSinc(ch, dSmooth);
            
            // Write to delay buffer AFTER reading
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            
            outputSamples[i] = out;
            
            // #region agent log
            if (shouldLog && ch == 0 && i == 0)
            {
                sampleDelay = dSmooth;
                sampleOut = out;
            }
            // #endregion
        }
    }
    
    // #region agent log
    if (shouldLog)
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusCoreThiran::processDelay\",\"message\":\"Processing audio\",\"data\":{\"core\":\"Blue_HQ_WindowedSinc\",\"delaySamples\":" << sampleDelay << ",\"outputSample\":" << sampleOut << ",\"interpolation\":\"WindowedSincFIR\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
