#include "ChorusCoreLagrange5th.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>

ChorusCoreLagrange5th::ChorusCoreLagrange5th()
{
}

void ChorusCoreLagrange5th::prepare(const juce::dsp::ProcessSpec& processSpec)
{
    spec = processSpec;
    
    // Calculate maximum delay needed
    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 4;
    
    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;
    
    // Round up to next power of 2
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 6) // +6 for 5th order
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;
    
    // Allocate buffers for each channel
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
    }
}

void ChorusCoreLagrange5th::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
}

float ChorusCoreLagrange5th::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreLagrange5th::readLagrange5th(int channel, float delaySamples) const
{
    const auto& buf = delayBuffers[static_cast<size_t>(channel)];
    const int writePos = writePositions[static_cast<size_t>(channel)];
    
    // Calculate read position
    float readPos = static_cast<float>(writePos) - delaySamples;
    while (readPos < 0.0f)
        readPos += static_cast<float>(bufferSize);
    
    // Get integer and fractional parts
    int i0 = static_cast<int>(readPos);
    float u = readPos - static_cast<float>(i0); // Fractional part
    
    // Get 6 samples for 5th order Lagrange: x[-2], x[-1], x[0], x[1], x[2], x[3]
    int indices[6];
    indices[0] = (i0 - 2) & bufferMask;
    indices[1] = (i0 - 1) & bufferMask;
    indices[2] = (i0 + 0) & bufferMask;
    indices[3] = (i0 + 1) & bufferMask;
    indices[4] = (i0 + 2) & bufferMask;
    indices[5] = (i0 + 3) & bufferMask;
    
    float samples[6];
    for (int i = 0; i < 6; ++i)
        samples[i] = buf[static_cast<size_t>(indices[i])];
    
    // Lagrange 5th order interpolation weights
    // Using standard Lagrange polynomial formula
    float result = 0.0f;
    for (int i = 0; i < 6; ++i)
    {
        float weight = 1.0f;
        float xi = static_cast<float>(i - 2); // Position relative to center
        for (int j = 0; j < 6; ++j)
        {
            if (i != j)
            {
                float xj = static_cast<float>(j - 2);
                weight *= (u - xj) / (xi - xj);
            }
        }
        result += weight * samples[i];
    }
    
    return result;
}

void ChorusCoreLagrange5th::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
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
    static bool firstProcess = true;
    bool shouldLog = (++logCounter % 1000 == 0) || firstProcess; // Log first process immediately
    if (firstProcess) firstProcess = false;
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
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            float delaySamp = centreDelaySamples + depthSamples * channelLfo[i];
            delaySamp = juce::jlimit(guardSamples, maxDelaySamples, delaySamp);
            
            const float in = inputSamples[i];
            
            // Write to buffer
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            
            // Read with Lagrange 5th order
            const float out = readLagrange5th(ch, delaySamp);
            outputSamples[i] = out;
            
            // #region agent log
            if (shouldLog && ch == 0 && i == 0)
            {
                sampleDelay = delaySamp;
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
            logFile << "{\"location\":\"ChorusCoreLagrange5th::processDelay\",\"message\":\"Processing audio\",\"data\":{\"core\":\"Green_HQ_Lagrange5th\",\"delaySamples\":" << sampleDelay << ",\"outputSample\":" << sampleOut << ",\"interpolation\":\"Lagrange5th\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
