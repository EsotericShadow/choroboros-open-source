#include "ChorusCoreBBD.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>

ChorusCoreBBD::ChorusCoreBBD()
{
}

void ChorusCoreBBD::prepare(const juce::dsp::ProcessSpec& processSpec)
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
    
    // Allocate channels
    channels.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& chan = channels[ch];
        // Fix #1: Allocate full BBD_STAGES (1024) to match clock formula
        chan.stages.assign(BBD_STAGES, 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldOutput = 0.0f;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        
        // Initialize one-pole filter states
        chan.inputLPState = 0.0f;
        chan.outputLPState = 0.0f;
        
        // Initialize smoothed delay (20-50ms ramp for smooth knob tracking)
        chan.smoothedDelayMs.reset(spec.sampleRate, 0.03f); // 30ms ramp
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f); // Default 20ms
    }
}

void ChorusCoreBBD::reset()
{
    for (auto& chan : channels)
    {
        std::fill(chan.stages.begin(), chan.stages.end(), 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldOutput = 0.0f;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        chan.inputLPState = 0.0f;
        chan.outputLPState = 0.0f;
        chan.smoothedClockFreq = 5000.0f;
        chan.smoothedFilterCutoff = 5000.0f;
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f);
    }
}

float ChorusCoreBBD::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreBBD::processBBDChannel(int channel, float input, float clockFreq)
{
    auto& chan = channels[static_cast<size_t>(channel)];
    
    // Smooth clock frequency changes to prevent crackling
    constexpr float clockSmoothingCoeff = 0.999f; // ~20ms @ 48kHz
    chan.smoothedClockFreq = clockSmoothingCoeff * chan.smoothedClockFreq + (1.0f - clockSmoothingCoeff) * clockFreq;
    const float smoothClockFreq = chan.smoothedClockFreq;
    
    // Calculate filter cutoff from clock frequency
    // Clamp cutoff to musical range (800Hz - 6kHz)
    float targetCutoffRaw = smoothClockFreq * 0.4f;
    float targetCutoff = juce::jlimit(800.0f, 6000.0f, targetCutoffRaw);
    constexpr float filterSmoothingCoeff = 0.998f; // ~10ms @ 48kHz
    chan.smoothedFilterCutoff = filterSmoothingCoeff * chan.smoothedFilterCutoff + (1.0f - filterSmoothingCoeff) * targetCutoff;
    
    // CRITICAL FIX: Use one-pole lowpass (no coefficient swapping, click-free)
    // One-pole: y[n] = g * y[n-1] + (1-g) * x[n], where g = exp(-2π * cutoff / sampleRate)
    const float g = std::exp(-2.0f * juce::MathConstants<float>::pi * chan.smoothedFilterCutoff / static_cast<float>(spec.sampleRate));
    
    // Input filter (anti-aliasing before BBD)
    chan.inputLPState = g * chan.inputLPState + (1.0f - g) * input;
    float filteredInput = chan.inputLPState;
    
    // Fix #1: Use consistent stage count - tap at BBD_STAGES/2 (512)
    const int delayStages = BBD_STAGES / 2;
    
    chan.clockPhase += static_cast<double>(smoothClockFreq) / static_cast<double>(spec.sampleRate);
    
    // Fix #2: Time interpolation - update held outputs on clock ticks
    // Handle multiple ticks per sample if clock > sample rate
    while (chan.clockPhase >= 1.0)
    {
        chan.clockPhase -= 1.0;
        
        // Shift register: write input to current head
        chan.stages[static_cast<size_t>(chan.head)] = filteredInput;
        chan.head = (chan.head + 1) % static_cast<int>(chan.stages.size());
        
        // Read from position offset by delayStages (BBD delay = N/2 stages)
        int readPos = (chan.head - delayStages + static_cast<int>(chan.stages.size())) % static_cast<int>(chan.stages.size());
        
        // Update held outputs for time interpolation
        chan.heldPrev = chan.heldNext;
        chan.heldNext = chan.stages[static_cast<size_t>(readPos)];
    }
    
    // Interpolate between held outputs over time (not across stages)
    // This removes bitcrush artifacts by smoothing the sample-and-hold staircase
    float t = static_cast<float>(chan.clockPhase);  // 0..1 phase within current tick
    float held = chan.heldPrev + t * (chan.heldNext - chan.heldPrev);
    
    // Output filter (reconstruction after BBD) - reuse same g from input filter
    chan.outputLPState = g * chan.outputLPState + (1.0f - g) * held;
    
    return chan.outputLPState;
}

void ChorusCoreBBD::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    // CRITICAL FIX: LFO is already scaled by depth*0.5 in ChorusDSP
    // So channelLfo values are in range [-depth*0.5, +depth*0.5]
    // Don't multiply by 0.5 again - use full maximumDelayModulation range
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelayMs = currentCentreDelayMs;
    float depthMs = maximumDelayModulation; // LFO already has depth scaling, don't multiply again
    
    // Access LFO buffers from ChorusDSP
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;
    
    // #region agent log
    static int logCounter = 0;
    static bool firstProcess = true;
    bool shouldLog = (++logCounter % 1000 == 0) || firstProcess; // Log first process immediately
    if (firstProcess) firstProcess = false;
    float sampleClockFreq = 0.0f;
    float sampleOut = 0.0f;
    // #endregion
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        
        auto& chan = channels[static_cast<size_t>(ch)];
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            // Calculate target delay time in milliseconds
            float targetDelayMs = centreDelayMs + depthMs * channelLfo[i];
            
            // CRITICAL FIX: Remove hard 50ms clamp - match UI range (up to 100ms)
            // BBD can handle longer delays, just need appropriate clock frequency
            targetDelayMs = juce::jlimit(2.0f, 100.0f, targetDelayMs); // Match UI range
            
            // CRITICAL FIX: Smooth delay with SmoothedValue (control-rate style)
            // This gives "analog knob" behavior and kills zipper
            chan.smoothedDelayMs.setTargetValue(targetDelayMs);
            float delayMs = chan.smoothedDelayMs.getNextValue();
            
            // Convert smoothed delay time to BBD clock frequency
            // T_delay = N / (2 * f_clk) => f_clk = N / (2 * T_delay)
            float delaySeconds = delayMs * 0.001f;
            float clockFreq = static_cast<float>(BBD_STAGES) / (2.0f * delaySeconds);
            
            // Clamp clock frequency to reasonable range
            // For BBD chorus, typical clock range is 2kHz - 50kHz
            // Upper limit: allow up to 0.9 * sampleRate to handle short delays
            // Lower limit: 2kHz minimum for stability
            float maxClockFreq = static_cast<float>(spec.sampleRate) * 0.9f;
            clockFreq = juce::jlimit(2000.0f, maxClockFreq, clockFreq);
            
            const float in = inputSamples[i];
            const float out = processBBDChannel(ch, in, clockFreq);
            outputSamples[i] = out;
            
            // #region agent log
            if (shouldLog && ch == 0 && i == 0)
            {
                sampleClockFreq = clockFreq;
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
            logFile << "{\"location\":\"ChorusCoreBBD::processDelay\",\"message\":\"Processing audio\",\"data\":{\"core\":\"Red_Normal_BBD\",\"clockFreqHz\":" << sampleClockFreq << ",\"outputSample\":" << sampleOut << ",\"type\":\"BBD\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
