#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

// Forward declaration (ChorusCore.h will forward declare ChorusDSP, avoiding circular dependency)
class ChorusCore;

// Main DSP class for chorus effect
// Acts as a controller that owns and switches between different chorus cores
class ChorusDSP
{
public:
    ChorusDSP();
    ~ChorusDSP();
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::AudioBlock<float>& block);
    
    void setRate(float rateHz);
    void setDepth(float depth); // 0.0 to 1.0
    void setOffset(float offsetDegrees); // 0 to 180
    void setWidth(float width); // 0.0 to 2.0
    void setColor(float color); // 0.0 to 1.0 (tone/saturation parameter)
    void setEngineColor(int colorIndex); // 0=Green, 1=Blue, 2=Red, 3=Purple
    void setQualityEnabled(bool enabled); // false=Normal, true=HQ
    void setMix(float mix); // 0.0 to 1.0 (dry/wet mix)
    
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
    
private:
    juce::dsp::ProcessSpec spec;
    
    // Store maximum block size for buffer allocation
    int maxBlockSize = 2048;
    
    // Current chorus core (swappable at runtime)
    ChorusCore* currentCore = nullptr;
    
    // Engine selection state
    int currentColorIndex = 0; // 0=Green, 1=Blue, 2=Red, 3=Purple
    bool currentQualityHQ = false; // false=Normal, true=HQ
    
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
    
    // Additional processing stages
    juce::dsp::IIR::Filter<float> hpf;  // High pass filter
    juce::dsp::IIR::Coefficients<float>::Ptr hpfCoeffs;
    
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
    
    // Target parameters
    float rateHz = 0.5f;  // Rate - Default: 0.5 Hz
    float depth = 0.5f;  // Depth - Default: 50%
    float offsetDegrees = 90.0f;  // Offset - Default: 90°
    float width = 1.0f;  // Width - Default: 100%
    float color = 0.5f;  // Color - Default: 50%
    
    // Helper functions
    float applySaturation(float sample, float colorValue);  // Saturation
    void processWidth(juce::dsp::AudioBlock<float>& block);  // Width processing
    float calculateCentreDelay(float depthValue);  // Centre delay calculation
    
    // Engine-specific parameter mapping
    // Maps normalized parameter (0-1) to engine-specific range
    float mapColorToEngineRange(float normalizedColor) const;  // Maps color based on current engine
    float mapRateToEngineRange(float normalizedRate) const;   // Maps rate based on current engine
    float mapDepthToEngineRange(float normalizedDepth) const; // Maps depth based on current engine
};
