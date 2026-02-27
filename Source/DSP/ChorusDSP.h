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

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <memory>

// Forward declarations
class ChorusCore;
class ChoroborosAudioProcessor;

// Main DSP class for chorus effect
// Acts as a controller that owns and switches between different chorus cores
class ChorusDSP
{
public:
    struct RuntimeTuning
    {
        std::atomic<float> rateSmoothingMs { 20.0f };
        std::atomic<float> depthSmoothingMs { 150.0f };
        std::atomic<float> depthRateLimit { 0.25f };
        std::atomic<float> centreDelaySmoothingMs { 150.0f };
        std::atomic<float> colorSmoothingMs { 20.0f };
        std::atomic<float> widthSmoothingMs { 20.0f };
        std::atomic<float> centreDelayBaseMs { 8.0f };
        std::atomic<float> centreDelayScale { 10.0f };

        std::atomic<float> hpfCutoffHz { 30.0f };
        std::atomic<float> hpfQ { 0.707f };
        std::atomic<float> lpfCutoffHz { 20000.0f };
        std::atomic<float> lpfQ { 0.707f };
        std::atomic<float> preEmphasisFreqHz { 3000.0f };
        std::atomic<float> preEmphasisQ { 0.707f };
        std::atomic<float> preEmphasisGain { 1.2f };
        std::atomic<float> preEmphasisLevelSmoothing { 0.95f };
        std::atomic<float> preEmphasisQuietThreshold { 0.125f };
        std::atomic<float> preEmphasisMaxAmount { 0.5f };
        std::atomic<float> compressorAttackMs { 50.0f };
        std::atomic<float> compressorReleaseMs { 200.0f };
        std::atomic<float> compressorThresholdDb { -6.0f };
        std::atomic<float> compressorRatio { 4.0f };
        std::atomic<float> saturationDriveScale { 3.0f };

        std::atomic<float> bbdDelaySmoothingMs { 20.0f };
        std::atomic<float> bbdDelayMinMs { 8.0f };
        std::atomic<float> bbdDelayMaxMs { 100.0f };
        std::atomic<float> bbdCentreBaseMs { 16.0f };
        std::atomic<float> bbdCentreScale { 2.0f };
        std::atomic<float> bbdDepthMs { 12.0f };
        std::atomic<float> bbdClockSmoothingMs { 20.0f };
        std::atomic<float> bbdFilterSmoothingMs { 10.0f };
        std::atomic<float> bbdFilterCutoffMinHz { 1200.0f };
        std::atomic<float> bbdFilterCutoffMaxHz { 9000.0f };
        std::atomic<float> bbdFilterCutoffScale { 0.45f };
        std::atomic<float> bbdClockMinHz { 2000.0f };
        std::atomic<float> bbdClockMaxRatio { 0.9f };
        std::atomic<float> bbdStages { 1024.0f };
        std::atomic<float> bbdFilterMaxRatio { 0.22f };

        std::atomic<float> tapeDelaySmoothingMs { 180.0f };
        std::atomic<float> tapeCentreBaseMs { 16.0f };
        std::atomic<float> tapeCentreScale { 2.0f };
        std::atomic<float> tapeToneMaxHz { 16000.0f };
        std::atomic<float> tapeToneMinHz { 12000.0f };
        std::atomic<float> tapeToneSmoothingCoeff { 0.08f };
        std::atomic<float> tapeDriveScale { 0.35f };
        std::atomic<float> tapeLfoRatioScale { 0.05f };
        std::atomic<float> tapeLfoModSmoothingCoeff { 0.0015f };
        std::atomic<float> tapeRatioSmoothingCoeff { 0.004f };
        std::atomic<float> tapePhaseDamping { 1.0f };
        std::atomic<float> tapeWowFreqBase { 0.33f };
        std::atomic<float> tapeWowFreqSpread { 0.03f };
        std::atomic<float> tapeFlutterFreqBase { 5.8f };
        std::atomic<float> tapeFlutterFreqSpread { 0.2f };
        std::atomic<float> tapeWowDepthBase { 0.0022f };
        std::atomic<float> tapeWowDepthSpread { 0.0002f };
        std::atomic<float> tapeFlutterDepthBase { 0.0011f };
        std::atomic<float> tapeFlutterDepthSpread { 0.0001f };
        std::atomic<float> tapeRatioMin { 0.96f };
        std::atomic<float> tapeRatioMax { 1.04f };
        std::atomic<float> tapeWetGain { 1.15f };
        std::atomic<float> tapeHermiteTension { 0.75f };
    };

    ChorusDSP();
    ~ChorusDSP();
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::AudioBlock<float>& block);
    
    void setRate(float rateHz);
    void setDepth(float depth); // 0.0 to 1.0
    void setOffset(float offsetDegrees); // 0 to 180
    void setWidth(float width); // 0.0 to 2.0
    void setColor(float color); // 0.0 to 1.0 (engine-specific character control)
    void setEngineColor(int colorIndex); // 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    void setQualityEnabled(bool enabled); // false=Normal, true=HQ
    void setMix(float mix); // 0.0 to 1.0 (dry/wet mix)

    RuntimeTuning& getRuntimeTuning() { return runtimeTuning; }
    const RuntimeTuning& getRuntimeTuning() const { return runtimeTuning; }
    
    // Make members accessible to helper classes and cores
    friend class ChorusDSPPrepare;
    friend class ChorusDSPProcess;
    friend class ChorusCore;
    friend class ChorusCoreLagrange3rd;
    friend class ChorusCoreLagrange5th;
    friend class ChorusCoreCubic;
    friend class ChorusCoreThiran;
    friend class ChorusCoreBBD;
    friend class ChorusCoreTape;
    friend class ChorusCorePhaseWarped;
    friend class ChorusCoreOrbit;
    friend class ChorusCoreLinear;
    friend class ChorusCoreLinearEnsemble;
    friend class ChoroborosAudioProcessor;

private:
    static constexpr int kNumEngineVariants = 10;
    static int getCoreVariantIndex(int colorIndex, bool hq) { return juce::jlimit(0, 4, colorIndex) * 2 + (hq ? 1 : 0); }

    struct RuntimeTuningSnapshot
    {
        float rateSmoothingMs = 20.0f;
        float depthSmoothingMs = 150.0f;
        float depthRateLimit = 0.25f;
        float centreDelaySmoothingMs = 150.0f;
        float colorSmoothingMs = 20.0f;
        float widthSmoothingMs = 20.0f;
        float centreDelayBaseMs = 8.0f;
        float centreDelayScale = 10.0f;

        float hpfCutoffHz = 30.0f;
        float hpfQ = 0.707f;
        float lpfCutoffHz = 20000.0f;
        float lpfQ = 0.707f;
        float preEmphasisFreqHz = 3000.0f;
        float preEmphasisQ = 0.707f;
        float preEmphasisGain = 1.2f;
        float preEmphasisLevelSmoothing = 0.95f;
        float preEmphasisQuietThreshold = 0.125f;
        float preEmphasisMaxAmount = 0.5f;
        float compressorAttackMs = 50.0f;
        float compressorReleaseMs = 200.0f;
        float compressorThresholdDb = -6.0f;
        float compressorRatio = 4.0f;
        float saturationDriveScale = 3.0f;

        float bbdDelaySmoothingMs = 20.0f;
        float bbdDelayMinMs = 8.0f;
        float bbdDelayMaxMs = 100.0f;
        float bbdCentreBaseMs = 16.0f;
        float bbdCentreScale = 2.0f;
        float bbdDepthMs = 12.0f;
        float bbdClockSmoothingMs = 20.0f;
        float bbdFilterSmoothingMs = 10.0f;
        float bbdFilterCutoffMinHz = 1200.0f;
        float bbdFilterCutoffMaxHz = 9000.0f;
        float bbdFilterCutoffScale = 0.45f;
        float bbdClockMinHz = 2000.0f;
        float bbdClockMaxRatio = 0.9f;
        float bbdStages = 1024.0f;
        float bbdFilterMaxRatio = 0.22f;

        float tapeDelaySmoothingMs = 180.0f;
        float tapeCentreBaseMs = 16.0f;
        float tapeCentreScale = 2.0f;
        float tapeToneMaxHz = 16000.0f;
        float tapeToneMinHz = 12000.0f;
        float tapeToneSmoothingCoeff = 0.08f;
        float tapeDriveScale = 0.35f;
        float tapeLfoRatioScale = 0.05f;
        float tapeLfoModSmoothingCoeff = 0.0015f;
        float tapeRatioSmoothingCoeff = 0.004f;
        float tapePhaseDamping = 1.0f;
        float tapeWowFreqBase = 0.33f;
        float tapeWowFreqSpread = 0.03f;
        float tapeFlutterFreqBase = 5.8f;
        float tapeFlutterFreqSpread = 0.2f;
        float tapeWowDepthBase = 0.0022f;
        float tapeWowDepthSpread = 0.0002f;
        float tapeFlutterDepthBase = 0.0011f;
        float tapeFlutterDepthSpread = 0.0001f;
        float tapeRatioMin = 0.96f;
        float tapeRatioMax = 1.04f;
        float tapeWetGain = 1.15f;
        float tapeHermiteTension = 0.75f;
    };

    void applyRuntimeTuning();

    juce::dsp::ProcessSpec spec;
    
    // Store maximum block size for buffer allocation
    int maxBlockSize = 2048;
    
    // Current chorus core (swappable at runtime)
    ChorusCore* currentCore = nullptr;
    ChorusCore* previousCore = nullptr;
    ChorusCore* pendingCore = nullptr;
    std::array<std::unique_ptr<ChorusCore>, kNumEngineVariants> coreVariants;
    
    // Engine selection state
    int currentColorIndex = 0; // 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    bool currentQualityHQ = false; // false=Normal, true=HQ
    bool coreSwitchCrossfadeActive = false;
    int coreSwitchCrossfadeSamplesRemaining = 0;
    int coreSwitchCrossfadeTotalSamples = 0;
    int coreSwitchWarmupSamplesRemaining = 0;
    int coreSwitchWarmupTotalSamples = 0;
    
    // Create and switch to a new core based on color and quality
    void switchCore(int colorIndex, bool hq);
    
    // Oscillators for LFO generation
    juce::dsp::Oscillator<float> lfo;  // Sine LFO for left channel
    juce::dsp::Oscillator<float> lfoCos;  // Cosine LFO for stereo phase offset
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> oscVolume;  // LFO amplitude smoothing
    juce::dsp::DryWetMixer<float> dryWet;  // Dry/wet mixing
    
    // Phase offset for stereo width (0-180° offset between L/R channels)
    float lfoPhaseOffset = 90.0f;  // Default 90° for classic stereo chorus
    
    // Pre-allocated buffers (CRITICAL: no heap allocation in process())
    juce::AudioBuffer<float> lfoBuffer;  // LFO buffer
    juce::AudioBuffer<float> cosBuffer;  // Cosine buffer for stereo
    juce::AudioBuffer<float> delaySamplesBuffer;  // Delay samples buffer
    juce::AudioBuffer<float> preEmphOriginalBuffer;  // Original signal storage for pre-emphasis
    juce::AudioBuffer<float> coreCrossfadeBufferA;  // Wet path buffer for active core
    juce::AudioBuffer<float> coreCrossfadeBufferB;  // Wet path buffer for previous core
    
    // Additional processing stages
    juce::dsp::IIR::Filter<float> hpf;  // High pass filter
    juce::dsp::IIR::Coefficients<float>::Ptr hpfCoeffs;
    juce::dsp::IIR::Filter<float> lpf;  // Low pass filter (tame highs, e.g. BBD aliasing)
    juce::dsp::IIR::Coefficients<float>::Ptr lpfCoeffs;
    juce::dsp::IIR::Filter<float> preEmphasis;  // Pre-emphasis filter
    juce::dsp::IIR::Coefficients<float>::Ptr preEmphasisCoeffs;
    float inputLevel = 0.0f;  // Input level tracking
    
    // Width processing filters
    juce::dsp::IIR::Filter<float> widthMidFilter1;
    juce::dsp::IIR::Filter<float> widthMidFilter2;
    juce::dsp::IIR::Filter<float> widthSideFilter1;
    juce::dsp::IIR::Filter<float> widthSideFilter2;
    juce::dsp::IIR::Coefficients<float>::Ptr widthMidCoeffs1;
    juce::dsp::IIR::Coefficients<float>::Ptr widthMidCoeffs2;
    
    juce::dsp::Compressor<float> compressor;  // Compression
    
    // Parameter smoothing using JUCE's official SmoothedValue
    // CRITICAL: All delay-related parameters must be smoothed to prevent read pointer jumps
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedRate;  // Rate smoothing
    // Use exponential smoothing (one-pole) for depth - more responsive to rapid changes
    // This prevents crackling when knob is turned fast
    float smoothedDepthValue = 0.5f;  // Depth value
    float depthSmoothingCoeff = 0.0f;  // One-pole filter coefficient
    
    // Rate limiter for depth - prevents rapid changes that overwhelm the smoother
    // Maximum change rate: 0.25 per second (full range in 4 seconds, reduced for ultra-smooth catch-up)
    // This ensures smooth transitions even during rapid 0-100% changes
    float depthRateLimit = 0.25f;  // Maximum change per second
    float depthRateLimitPerSample = 0.0f;  // Calculated per-sample limit
    float currentDepthTarget = 0.5f;  // Current rate-limited target
    
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCentreDelay;  // Centre delay smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedColor;  // Color smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedWidth;  // Width smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOffset;  // Offset smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;  // Mix smoothing
    
    // Target parameters
    float rateHz = 0.5f;  // Rate - Default: 0.5 Hz
    float depth = 0.5f;  // Depth - Default: 50%
    float offsetDegrees = 90.0f;  // Offset - Default: 90°
    float width = 1.0f;  // Width - Default: 100%
    float color = 0.5f;  // Color - Default: 50%
    float mix = 0.5f;  // Mix - Default: 50%
    
    // Helper functions
    float applySaturation(float sample, float colorValue);  // Saturation
    void processWidth(juce::dsp::AudioBlock<float>& block);  // Width processing
    float calculateCentreDelay(float depthValue);  // Centre delay calculation
    
    // Engine-specific parameter mapping
    // Maps normalized parameter (0-1) to engine-specific range
    float mapColorToEngineRange(float normalizedColor) const;  // Maps color based on current engine
    float mapRateToEngineRange(float normalizedRate) const;   // Maps rate based on current engine
    float mapDepthToEngineRange(float normalizedDepth) const; // Maps depth based on current engine

    RuntimeTuning runtimeTuning;
    RuntimeTuningSnapshot runtimeTuningSnapshot;
    RuntimeTuningSnapshot lastAppliedTuningSnapshot;
    bool runtimeTuningApplied = false;
};
