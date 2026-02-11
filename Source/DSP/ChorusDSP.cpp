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
#include <cmath>
#include <fstream>
#include <chrono>

ChorusDSP::ChorusDSP()
{
    // #region agent log
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::ChorusDSP\",\"message\":\"ChorusDSP constructor called\",\"data\":{\"action\":\"initializing\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    // Start with Green Normal (Lagrange3rd)
    currentColorIndex = 0;
    currentQualityHQ = false;
    currentCore = new ChorusCoreLagrange3rd();
    
    // #region agent log
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::ChorusDSP\",\"message\":\"Initial core created\",\"data\":{\"core\":\"Green_Normal_Lagrange3rd\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}

ChorusDSP::~ChorusDSP()
{
    delete currentCore;
}

void ChorusDSP::prepare(const juce::dsp::ProcessSpec& processSpec)
{
    spec = processSpec;
    
    // #region agent log
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::prepare\",\"message\":\"ChorusDSP prepare called\",\"data\":{\"sampleRate\":" << spec.sampleRate << ",\"numChannels\":" << spec.numChannels << ",\"maxBlockSize\":" << spec.maximumBlockSize << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    // Prepare the current core
    if (currentCore)
        currentCore->prepare(spec);
    
    ChorusDSPPrepare::prepareLFOs(*this, spec);
    ChorusDSPPrepare::prepareBuffers(*this, spec);
    ChorusDSPPrepare::prepareFilters(*this, spec);
    
    // Initialize parameter smoothers
    // CRITICAL: Smooth ALL delay-related parameters to prevent read pointer discontinuities
    smoothedRate.reset(spec.sampleRate, 0.02);  // 20ms for rate
    
    // Two-stage exponential smoothing for depth - ultra-smooth for rapid 0-100% changes
    // Stage 1: Fast rate limiter (prevents sudden jumps)
    // Stage 2: Slow exponential smoothing (150ms for very smooth catch-up)
    const float depthSmoothingTimeMs = 150.0f;  // Increased from 100ms for ultra-smooth catch-up
    depthSmoothingCoeff = std::exp(-1.0f / (depthSmoothingTimeMs * 0.001f * spec.sampleRate));
    // Map depth to engine-specific range
    smoothedDepthValue = mapDepthToEngineRange(depth);
    
    // Rate limiter for depth - prevents rapid changes that cause crackling
    // Maximum change: 0.25 per second (full 0-1 range in 4 seconds, reduced from 0.3/sec)
    // Slower rate limit ensures smoother catch-up during rapid 0-100% changes
    depthRateLimitPerSample = depthRateLimit / spec.sampleRate;
    currentDepthTarget = mapDepthToEngineRange(depth);
    
    // Centre delay smoothing - increased to match depth smoothing time
    // CRITICAL: JUCE's setCentreDelay() has NO internal smoothing, so we must smooth it ourselves
    smoothedCentreDelay.reset(spec.sampleRate, 0.15);  // 150ms for centre delay (matches depth smoothing)
    smoothedColor.reset(spec.sampleRate, 0.02);  // 20ms for color
    smoothedWidth.reset(spec.sampleRate, 0.02);  // 20ms for width
    
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
    // Reset the current core
    if (currentCore)
        currentCore->reset();
    
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
    
    // Reset smoothed depth and rate-limited target to current value (mapped to engine range)
    smoothedDepthValue = mapDepthToEngineRange(depth);
    currentDepthTarget = mapDepthToEngineRange(depth);
}

float ChorusDSP::calculateCentreDelay(float depthValue)
{
    // Rule B: centreDelayMs = 8.0 + 10.0 * depthN
    return 8.0f + 10.0f * depthValue;
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
            
        default:
            return normalizedColor;
    }
}

float ChorusDSP::mapRateToEngineRange(float normalizedRate) const
{
    // Map normalized rate (0-1) to engine-specific ranges
    // Currently all engines use the same rate range (0.01-10.0 Hz)
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
    float drive = 1.0f + 3.0f * colorValue;  // Increased from 2.0 to 3.0 for more effect
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
    // #region agent log
    static int processCounter = 0;
    static bool firstProcess = true;
    if (firstProcess)
    {
        firstProcess = false;
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::process\",\"message\":\"First audio process call\",\"data\":{\"blockSize\":" << block.getNumSamples() << ",\"numChannels\":" << block.getNumChannels() << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    if (block.getNumSamples() == 0)
        return;
    
    juce::ScopedNoDenormals noDenormals;
    
    juce::dsp::AudioBlock<float> nonConstBlock = block;
    auto context = juce::dsp::ProcessContextReplacing<float>(nonConstBlock);
    
    hpf.process(context);
    ChorusDSPProcess::processPreEmphasis(*this, nonConstBlock);
    ChorusDSPProcess::processSaturation(*this, nonConstBlock);
    ChorusDSPProcess::processChorus(*this, nonConstBlock);
    
    if (nonConstBlock.getNumChannels() >= 2)
        processWidth(nonConstBlock);
    
    compressor.process(context);
}

void ChorusDSP::setRate(float rateHz_)
{
    rateHz = juce::jlimit(0.01f, 10.0f, rateHz_);
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
    colorIndex = juce::jlimit(0, 3, colorIndex);
    if (currentColorIndex != colorIndex)
    {
        currentColorIndex = colorIndex;
        // Remap depth when switching engines (Purple has compressed range 0-0.45)
        float mappedDepth = mapDepthToEngineRange(depth);
        currentDepthTarget = mappedDepth;
        smoothedDepthValue = mappedDepth;
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
    // Create new core based on color and quality
    ChorusCore* newCore = nullptr;
    const char* coreName = nullptr;
    
    if (colorIndex == 0) // Green
    {
        if (hq)
        {
            newCore = new ChorusCoreLagrange5th();
            coreName = "Green_HQ_Lagrange5th";
        }
        else
        {
            newCore = new ChorusCoreLagrange3rd();
            coreName = "Green_Normal_Lagrange3rd";
        }
    }
    else if (colorIndex == 1) // Blue
    {
        if (hq)
        {
            newCore = new ChorusCoreThiran();
            coreName = "Blue_HQ_Thiran";
        }
        else
        {
            newCore = new ChorusCoreCubic();
            coreName = "Blue_Normal_Cubic";
        }
    }
    else if (colorIndex == 2) // Red
    {
        if (hq)
        {
            newCore = new ChorusCoreTape();
            coreName = "Red_HQ_Tape";
        }
        else
        {
            newCore = new ChorusCoreBBD();
            coreName = "Red_Normal_BBD";
        }
    }
    else if (colorIndex == 3) // Purple
    {
        if (hq)
        {
            newCore = new ChorusCoreOrbit();
            coreName = "Purple_HQ_Orbit";
        }
        else
        {
            newCore = new ChorusCorePhaseWarped();
            coreName = "Purple_Normal_PhaseWarped";
        }
    }
    
    // #region agent log
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::switchCore\",\"message\":\"Core switched\",\"data\":{\"colorIndex\":" << colorIndex << ",\"hq\":" << (hq ? "true" : "false") << ",\"coreName\":\"" << coreName << "\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
    
    // Prepare the new core with current spec
    if (spec.sampleRate > 0.0)
        newCore->prepare(spec);
    
    // Swap cores (seamless - no clicks)
    delete currentCore;
    currentCore = newCore;
    
    // #region agent log
    {
        std::ofstream logFile("/Users/main/Desktop/green_chorus/.cursor/debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"location\":\"ChorusDSP::switchCore\",\"message\":\"Core swap completed\",\"data\":{\"coreName\":\"" << coreName << "\",\"currentCorePtr\":" << (currentCore != nullptr ? "valid" : "null") << ",\"sampleRate\":" << spec.sampleRate << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            logFile.close();
        }
    }
    // #endregion
}
