#include "ChorusCoreTape.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <chrono>

int tapeProcessCallCount = 0;

ChorusCoreTape::ChorusCoreTape()
{
}

void ChorusCoreTape::prepare(const juce::dsp::ProcessSpec& processSpec)
{
    spec = processSpec;
    
    // #region agent log - reset process counter
    {
        static int prepareCount = 0;
        prepareCount++;
        tapeProcessCallCount = 0;
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusCoreTape::prepare\",\"message\":\"Tape core prepared (resampling-based)\",\"data\":{\"prepareCount\":" << prepareCount << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    // Calculate maximum delay needed
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 4;
    
    maxDelaySamples = static_cast<int>(std::ceil(
        maxCentreDelayMs * spec.sampleRate / 1000.0)) + guardMarginSamples;
    
    // Fixed delay length (static, no modulation - modulation is via resampling)
    fixedDelaySamples = static_cast<int>(std::ceil(20.0f * spec.sampleRate / 1000.0f)); // 20ms fixed delay
    
    // Allocate delay buffers
    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 4)
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;
    
    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    readPositions.resize(static_cast<size_t>(spec.numChannels));
    resamplers.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        readPositions[ch] = 0; // Will be set to fixed delay offset
        resamplers[ch].readPhase = 0.0f; // Start at beginning of delay
        resamplers[ch].ratio = 1.0f;
        resamplers[ch].smoothedRatio = 1.0f;
    }
    
    // Initialize tape modulation (deterministic oscillators)
    tapeMod.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < tapeMod.size(); ++ch)
    {
        auto& s = tapeMod[ch];
        
        // Subtle stereo decorrelation
        s.wowFreq     = 0.35f + 0.05f * static_cast<float>(ch);
        s.flutterFreq = 6.0f + 0.2f * static_cast<float>(ch);
        
        // Vintage tape speed modulation: very subtle
        // These control resample ratio, not delay position
        s.wowDepth     = 0.0015f;  // ~0.15% speed variation
        s.flutterDepth = 0.0003f;  // ~0.03% speed variation
    }
    
    // Tape tone filter (per-channel)
    toneLP.resize(static_cast<size_t>(spec.numChannels));
    toneLPCoeffs.resize(static_cast<size_t>(spec.numChannels));
    
    for (size_t ch = 0; ch < static_cast<size_t>(spec.numChannels); ++ch)
    {
        toneLPCoeffs[ch] = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 12000.0f, 0.707f);
        toneLP[ch].coefficients = toneLPCoeffs[ch];
        toneLP[ch].prepare(spec);
    }
}

void ChorusCoreTape::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(writePositions.begin(), writePositions.end(), 0);
    
    for (size_t ch = 0; ch < readPositions.size(); ++ch)
    {
        readPositions[ch] = fixedDelaySamples; // Set fixed read position
    }
    
    for (auto& resampler : resamplers)
    {
        resampler.readPhase = 0.0f;
        resampler.ratio = 1.0f;
        resampler.smoothedRatio = 1.0f;
    }
    
    for (auto& mod : tapeMod)
    {
        mod.wowPhase = 0.0f;
        mod.flutterPhase = 0.0f;
    }
    
    lastColor = -1.0f;
    smoothedToneCutoff = 12000.0f;
    lastSetToneCutoff = 12000.0f;
    
    for (auto& filter : toneLP)
    {
        filter.reset();
    }
}

float ChorusCoreTape::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreTape::processTapeMod(TapeModState& s, double sampleRate, float wowDepthScale, float flutterDepthScale)
{
    // Advance phases
    s.wowPhase     += s.wowFreq / static_cast<float>(sampleRate);
    s.flutterPhase += s.flutterFreq / static_cast<float>(sampleRate);
    
    if (s.wowPhase >= 1.0f)     s.wowPhase -= 1.0f;
    if (s.flutterPhase >= 1.0f) s.flutterPhase -= 1.0f;
    
    // Continuous oscillators (deterministic, no noise)
    float wow     = std::sin(juce::MathConstants<float>::twoPi * s.wowPhase);
    float flutter = std::sin(juce::MathConstants<float>::twoPi * s.flutterPhase);
    
    // CRITICAL: Modulate resample RATIO, not delay position
    // Ratio = 1.0 means no pitch change, 1.001 = 0.1% faster (higher pitch)
    // Depth parameter scales the modulation amount
    float targetRatio = 1.0f + wow * s.wowDepth * wowDepthScale + flutter * s.flutterDepth * flutterDepthScale;
    
    // Clamp ratio to safe range (very small variations)
    targetRatio = juce::jlimit(0.995f, 1.005f, targetRatio); // ±0.5% max
    
    return targetRatio;
}

float ChorusCoreTape::tapeSaturate(float sample, float drive)
{
    // Soft clip using tanh
    float saturated = std::tanh(sample * drive);
    return saturated / drive; // Normalize
}

float ChorusCoreTape::resampleHermite(const float* buf, int bufMask, int readPos, float phase) const
{
    // Hermite 4-point interpolation (clean, efficient, stable under modulation)
    // phase is in [0, 1) - fractional position between samples
    
    // Get 4 samples: p[-1], p[0], p[1], p[2]
    int i0 = readPos;
    int im1 = (i0 - 1) & bufMask;
    int ip1 = (i0 + 1) & bufMask;
    int ip2 = (i0 + 2) & bufMask;
    
    float p_m1 = buf[static_cast<size_t>(im1)];
    float p_0  = buf[static_cast<size_t>(i0)];
    float p_1  = buf[static_cast<size_t>(ip1)];
    float p_2  = buf[static_cast<size_t>(ip2)];
    
    // Hermite interpolation weights
    float t = phase;
    float t2 = t * t;
    float t3 = t2 * t;
    
    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;
    
    // Tangents (finite differences)
    float m0 = 0.5f * (p_1 - p_m1);
    float m1 = 0.5f * (p_2 - p_0);
    
    return h00 * p_0 + h10 * m0 + h01 * p_1 + h11 * m1;
}

void ChorusCoreTape::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    
    const float guardSamples = getGuardSamples();
    const float maxDelaySamples = getMaxDelaySamples();
    
    // Fixed delay (no modulation - modulation is via resampling)
    // Centre delay parameter sets the fixed delay length
    int targetFixedDelay = static_cast<int>(std::round(currentCentreDelayMs * spec.sampleRate / 1000.0f));
    targetFixedDelay = juce::jlimit(static_cast<int>(guardSamples), static_cast<int>(maxDelaySamples), targetFixedDelay);
    
    // Smooth fixed delay changes (user parameter changes)
    constexpr float fixedDelaySmoothingCoeff = 0.999f; // ~20ms @ 48k
    fixedDelaySamples = static_cast<int>(fixedDelaySmoothingCoeff * static_cast<float>(fixedDelaySamples) + 
                                        (1.0f - fixedDelaySmoothingCoeff) * static_cast<float>(targetFixedDelay));
    
    // Get current depth to scale modulation (depth = 0.0 to 1.0)
    float currentDepth = dsp.smoothedDepthValue;
    
    // Get current color for tape tone
    float currentColor = dsp.smoothedColor.getCurrentValue();
    float tapeDrive = 1.0f + 0.3f * currentColor;
    float targetToneCutoff = 12000.0f + 4000.0f * (1.0f - currentColor);
    
    lastColor = currentColor;
    
    // Access LFO buffers for chorus modulation (in addition to wow/flutter)
    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;
    
    // #region agent log
    static int logCounter = 0;
    tapeProcessCallCount++;
    bool shouldLog = (++logCounter % 100 == 0) || (tapeProcessCallCount <= 10);
    float sampleRatio = 0.0f;
    float sampleOut = 0.0f;
    // #endregion
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        
        auto& mod = tapeMod[static_cast<size_t>(ch)];
        auto& resampler = resamplers[static_cast<size_t>(ch)];
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        
        // Scale wow/flutter depth by depth parameter
        float scaledWowDepth = mod.wowDepth * currentDepth;
        float scaledFlutterDepth = mod.flutterDepth * currentDepth;
        
        // CRITICAL: For chorus effect, we need LFO modulation too
        // LFO already scaled by depth*0.5, so use it directly
        constexpr float lfoModulationScale = 0.002f; // ±0.2% pitch modulation for chorus
        
        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float in = inputSamples[i];
            
            // CRITICAL: Modulate resample ratio (LFO + wow/flutter)
            // LFO provides main chorus modulation, wow/flutter add tape character
            float lfoMod = channelLfo[i] * lfoModulationScale; // LFO already scaled by depth
            float tapeModRatio = processTapeMod(mod, spec.sampleRate, scaledWowDepth, scaledFlutterDepth);
            
            // Combine: LFO modulation + tape wow/flutter
            float targetRatio = 1.0f + lfoMod + (tapeModRatio - 1.0f);
            
            // Clamp ratio to safe range
            targetRatio = juce::jlimit(0.99f, 1.01f, targetRatio); // ±1% max for chorus
            
            // Smooth ratio changes to prevent clicks
            constexpr float ratioSmoothingCoeff = 0.998f; // ~5ms @ 48k
            resampler.smoothedRatio = ratioSmoothingCoeff * resampler.smoothedRatio + 
                                     (1.0f - ratioSmoothingCoeff) * targetRatio;
            resampler.ratio = resampler.smoothedRatio;
            
            // CRITICAL FIX: Read BEFORE write (correct delay line operation)
            // Calculate read position: fixed delay behind write head, minus accumulated phase offset
            // readPhase accumulates the pitch modulation offset (positive = read slower, negative = read faster)
            float baseReadPos = static_cast<float>(writePos) - static_cast<float>(fixedDelaySamples);
            while (baseReadPos < 0.0f) baseReadPos += static_cast<float>(bufferSize);
            
            // Subtract accumulated phase offset (when ratio > 1.0, we consume more samples, so offset increases)
            float totalReadPos = baseReadPos - resampler.readPhase;
            while (totalReadPos < 0.0f) totalReadPos += static_cast<float>(bufferSize);
            while (totalReadPos >= static_cast<float>(bufferSize)) totalReadPos -= static_cast<float>(bufferSize);
            
            // Get integer and fractional parts for interpolation
            int readIdx = static_cast<int>(std::floor(totalReadPos)) & bufferMask;
            float phaseFrac = totalReadPos - std::floor(totalReadPos);
            
            // Resample using Hermite interpolation
            float wet = resampleHermite(buffer.data(), bufferMask, readIdx, phaseFrac);
            
            // Apply tape tone (subtle high-frequency rolloff)
            wet = toneLP[static_cast<size_t>(ch)].processSample(wet);
            
            outputSamples[i] = wet;
            
            // Saturate BEFORE delay (how tape actually behaves)
            float saturated = tapeSaturate(in, tapeDrive);
            
            // Write saturated input to delay buffer (AFTER reading)
            buffer[static_cast<size_t>(writePos)] = saturated;
            writePos = (writePos + 1) & bufferMask;
            
            // Advance read phase by (ratio - 1.0) to accumulate pitch modulation offset
            // When ratio = 1.0, phase doesn't change (no pitch shift)
            // When ratio > 1.0, phase increases (we're reading faster, consuming more samples)
            // When ratio < 1.0, phase decreases (we're reading slower, consuming fewer samples)
            resampler.readPhase += (resampler.ratio - 1.0f);
            
            // Wrap read phase to stay within reasonable bounds (±bufferSize)
            // This prevents unbounded accumulation
            while (resampler.readPhase >= static_cast<float>(bufferSize))
                resampler.readPhase -= static_cast<float>(bufferSize);
            while (resampler.readPhase <= -static_cast<float>(bufferSize))
                resampler.readPhase += static_cast<float>(bufferSize);
            
            // #region agent log
            if (shouldLog && ch == 0 && i == 0)
            {
                sampleRatio = resampler.ratio;
                sampleOut = wet;
            }
            // #endregion
        }
    }
    
    // #region agent log
    if (shouldLog)
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusCoreTape::processDelay\",\"message\":\"Processing audio\",\"data\":{\"core\":\"Red_HQ_Tape_Resampler\",\"resampleRatio\":" << sampleRatio << ",\"outputSample\":" << sampleOut << ",\"type\":\"ResamplingBased\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
