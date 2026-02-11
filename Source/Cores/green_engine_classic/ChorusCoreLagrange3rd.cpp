#include "ChorusCoreLagrange3rd.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <fstream>
#include <chrono>

ChorusCoreLagrange3rd::ChorusCoreLagrange3rd()
{
}

void ChorusCoreLagrange3rd::prepare(const juce::dsp::ProcessSpec& processSpec)
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
    
    delayLine.setMaximumDelayInSamples(maxDelaySamples);
    delayLine.prepare(spec);
}

void ChorusCoreLagrange3rd::reset()
{
    delayLine.reset();
}

float ChorusCoreLagrange3rd::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreLagrange3rd::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;
    
    // Access LFO buffers from ChorusDSP (friend class)
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
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            float delaySamp = centreDelaySamples + depthSamples * channelLfo[i];
            delaySamp = juce::jlimit(guardSamples, maxDelaySamples, delaySamp);
            
            const float in = inputSamples[i];
            delayLine.pushSample(ch, in);
            
            const float out = delayLine.popSample(ch, delaySamp, true);
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
            logFile << "{\"location\":\"ChorusCoreLagrange3rd::processDelay\",\"message\":\"Processing audio\",\"data\":{\"core\":\"Green_Normal_Lagrange3rd\",\"delaySamples\":" << sampleDelay << ",\"outputSample\":" << sampleOut << ",\"interpolation\":\"Lagrange3rd\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
