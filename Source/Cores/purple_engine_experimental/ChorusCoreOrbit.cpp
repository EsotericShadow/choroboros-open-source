#include "ChorusCoreOrbit.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>

ChorusCoreOrbit::ChorusCoreOrbit()
{
}

void ChorusCoreOrbit::prepare(const juce::dsp::ProcessSpec& processSpec)
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
    
    // Round up to next power of 2 for efficient masking
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 4) // +4 for cubic interpolation
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;
    
    // Allocate buffers for each channel
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    orbitStates.resize(static_cast<size_t>(spec.numChannels));
    delaySmoothers1.resize(static_cast<size_t>(spec.numChannels));
    delaySmoothers2.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        
        auto& state = orbitStates[ch];
        state.phase = 0.0f;
        state.theta = 0.0f;
        state.theta2 = 0.0f;
        state.smoothedDelay1 = 0.0f;
        state.smoothedDelay2 = 0.0f;
        state.initialized = false;
        
        // 20ms smoothing for delay (snappy but safe) - one per tap
        delaySmoothers1[ch].reset(spec.sampleRate, 0.02f);
        delaySmoothers1[ch].setCurrentAndTargetValue(0.0f);
        delaySmoothers2[ch].reset(spec.sampleRate, 0.02f);
        delaySmoothers2[ch].setCurrentAndTargetValue(0.0f);
    }
}

void ChorusCoreOrbit::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    
    for (size_t ch = 0; ch < orbitStates.size(); ++ch)
    {
        auto& state = orbitStates[ch];
        state.phase = 0.0f;
        state.theta = 0.0f;
        state.theta2 = 0.0f;
        state.smoothedDelay1 = 0.0f;
        state.smoothedDelay2 = 0.0f;
        state.initialized = false;
        delaySmoothers1[ch].setCurrentAndTargetValue(0.0f);
        delaySmoothers2[ch].setCurrentAndTargetValue(0.0f);
    }
}

float ChorusCoreOrbit::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreOrbit::computeOrbitModulation(float phase, float theta, float eccentricity) const
{
    // Convert phases to radians
    const float phi = phase * juce::MathConstants<float>::twoPi;
    const float thetaRad = theta * juce::MathConstants<float>::twoPi;
    
    // 2D oscillator: x = sin(φ), y = (1-e)*cos(φ) (elliptical)
    const float x = std::sin(phi);
    const float y = (1.0f - eccentricity) * std::cos(phi);
    
    // Project onto rotating axis: u = x*cos(θ) + y*sin(θ)
    const float u = x * std::cos(thetaRad) + y * std::sin(thetaRad);
    
    return u;
}

float ChorusCoreOrbit::readCubic(int channel, float delaySamples) const
{
    const auto& buf = delayBuffers[static_cast<size_t>(channel)];
    const int writePos = writePositions[static_cast<size_t>(channel)];
    
    // Calculate read position (behind write head)
    float readPos = static_cast<float>(writePos) - delaySamples;
    
    // Wrap to positive range
    while (readPos < 0.0f)
        readPos += static_cast<float>(bufferSize);
    
    // Get integer and fractional parts
    int i1 = static_cast<int>(readPos);
    float u = readPos - static_cast<float>(i1); // Fractional part in [0,1)
    
    // Get indices for 4-point cubic (p_{-1}, p_0, p_{+1}, p_{+2})
    int im1 = (i1 - 1) & bufferMask;
    int i0  = (i1 + 0) & bufferMask;
    int ip1 = (i1 + 1) & bufferMask;
    int ip2 = (i1 + 2) & bufferMask;
    
    // Get samples
    float pm1 = buf[static_cast<size_t>(im1)];
    float p0  = buf[static_cast<size_t>(i0)];
    float p1  = buf[static_cast<size_t>(ip1)];
    float p2  = buf[static_cast<size_t>(ip2)];
    
    // Catmull-Rom cubic weights
    float u2 = u * u;
    float u3 = u2 * u;
    
    float w_m1 = 0.5f * (-u3 + 2.0f * u2 - u);
    float w_0  = 0.5f * ( 3.0f * u3 - 5.0f * u2 + 2.0f);
    float w_1  = 0.5f * (-3.0f * u3 + 4.0f * u2 + u);
    float w_2  = 0.5f * ( u3 - u2);
    
    return w_m1 * pm1 + w_0 * p0 + w_1 * p1 + w_2 * p2;
}

void ChorusCoreOrbit::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    constexpr float maximumDelayModulation = 20.0f;
    float centreDelaySamples = currentCentreDelayMs * spec.sampleRate / 1000.0f;
    float depthSamples = maximumDelayModulation * spec.sampleRate / 1000.0f;
    
    // Get smoothed rate and color (computed once per block)
    float currentRate = dsp.smoothedRate.getCurrentValue();
    float currentColor = dsp.smoothedColor.getCurrentValue();
    
    // Phase increment per sample (fast, rate-controlled)
    float phaseInc = currentRate / spec.sampleRate;
    
    // Orbit parameters from color:
    // e (eccentricity): 0.0 → 0.6 (0 = circle, 0.6 = very elliptical)
    // thetaRate: 0.01 → 0.1 Hz (ultra slow rotation)
    const float eccentricity = 0.6f * currentColor;
    const float thetaRate = 0.01f + 0.09f * currentColor; // 0.01 to 0.1 Hz
    const float thetaInc = thetaRate / spec.sampleRate;
    
    // Second tap parameters (slightly different for ensemble effect)
    const float thetaRate2 = thetaRate * 1.3f; // 30% faster rotation
    const float thetaInc2 = thetaRate2 / spec.sampleRate;
    const float eccentricity2 = eccentricity * 0.8f; // Slightly less elliptical
    
    // Dual tap mix ratios (60/40 for ensemble density)
    constexpr float mix1 = 0.6f;
    constexpr float mix2 = 0.4f;
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        auto& state = orbitStates[static_cast<size_t>(ch)];
        auto& delaySmoother1 = delaySmoothers1[static_cast<size_t>(ch)];
        auto& delaySmoother2 = delaySmoothers2[static_cast<size_t>(ch)];
        
        // Slight stereo decorrelation: offset theta for right channel
        float thetaOffset = (ch == 1) ? 0.25f : 0.0f; // 90° offset for right channel
        
        // Initialize delays on first sample if needed
        if (!state.initialized)
        {
            float mod1 = computeOrbitModulation(state.phase, state.theta + thetaOffset, eccentricity);
            float mod2 = computeOrbitModulation(state.phase, state.theta2 + thetaOffset, eccentricity2);
            
            float initialDelay1 = centreDelaySamples + depthSamples * mod1;
            float initialDelay2 = centreDelaySamples + depthSamples * mod2;
            initialDelay1 = juce::jlimit(guardSamples, maxDelaySamples, initialDelay1);
            initialDelay2 = juce::jlimit(guardSamples, maxDelaySamples, initialDelay2);
            
            state.smoothedDelay1 = initialDelay1;
            state.smoothedDelay2 = initialDelay2;
            delaySmoother1.setCurrentAndTargetValue(initialDelay1);
            delaySmoother2.setCurrentAndTargetValue(initialDelay2);
            state.initialized = true;
        }
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];
            
            // Write to buffer (once per sample, shared by both taps)
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;
            
            // Advance phases
            state.phase += phaseInc;
            if (state.phase >= 1.0f)
                state.phase -= 1.0f;
            
            state.theta += thetaInc;
            if (state.theta >= 1.0f)
                state.theta -= 1.0f;
            
            state.theta2 += thetaInc2;
            if (state.theta2 >= 1.0f)
                state.theta2 -= 1.0f;
            
            // Compute orbit modulation for both taps
            float mod1 = computeOrbitModulation(state.phase, state.theta + thetaOffset, eccentricity);
            float mod2 = computeOrbitModulation(state.phase, state.theta2 + thetaOffset, eccentricity2);
            
            // Calculate target delays
            float targetDelay1 = centreDelaySamples + depthSamples * mod1;
            float targetDelay2 = centreDelaySamples + depthSamples * mod2;
            targetDelay1 = juce::jlimit(guardSamples, maxDelaySamples, targetDelay1);
            targetDelay2 = juce::jlimit(guardSamples, maxDelaySamples, targetDelay2);
            
            // Smooth delays (20ms ramp)
            delaySmoother1.setTargetValue(targetDelay1);
            delaySmoother2.setTargetValue(targetDelay2);
            float delaySamp1 = delaySmoother1.getNextValue();
            float delaySamp2 = delaySmoother2.getNextValue();
            
            // Read both taps with cubic interpolation
            float wet1 = readCubic(ch, delaySamp1);
            float wet2 = readCubic(ch, delaySamp2);
            
            // Mix dual taps for ensemble density
            outputSamples[i] = mix1 * wet1 + mix2 * wet2;
        }
    }
}
