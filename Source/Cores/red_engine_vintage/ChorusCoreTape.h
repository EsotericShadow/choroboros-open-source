#pragma once

#include "../ChorusCore.h"
#include <vector>

// Tape chorus with resampling-based modulation (no moving read head)
// Red HQ mode - uses fixed delay + resampler instead of time-varying delay tap
class ChorusCoreTape : public ChorusCore
{
public:
    ChorusCoreTape();
    ~ChorusCoreTape() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 2.0f; }
    float getMaxDelaySamples() const override;
    
private:
    // Fixed delay buffer (no moving read head)
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    std::vector<int> readPositions; // Fixed read position (static delay)
    int bufferSize = 0;
    int bufferMask = 0;
    
    // Resampler state per channel
    struct ResamplerState
    {
        float readPhase = 0.0f;    // Continuous read position in delay buffer (samples)
        float ratio = 1.0f;         // Current resample ratio (1.0 = no pitch change)
        float smoothedRatio = 1.0f; // Smoothed ratio to prevent clicks
    };
    std::vector<ResamplerState> resamplers;
    
    // Tape modulation: modulates resample ratio, not delay position
    struct TapeModState
    {
        float wowPhase = 0.0f;
        float flutterPhase = 0.0f;
        
        float wowFreq = 0.35f;        // Hz (very slow)
        float flutterFreq = 6.0f;     // Hz (classic tape flutter)
        
        float wowDepth = 0.0f;        // Speed modulation depth (dimensionless)
        float flutterDepth = 0.0f;    // Speed modulation depth (dimensionless)
    };
    std::vector<TapeModState> tapeMod;
    
    // Tape tone/saturation (per-channel to prevent cross-channel contamination)
    std::vector<juce::dsp::IIR::Filter<float>> toneLP;
    std::vector<juce::dsp::IIR::Coefficients<float>::Ptr> toneLPCoeffs;
    float lastColor = -1.0f;
    float smoothedToneCutoff = 12000.0f;
    float lastSetToneCutoff = 12000.0f;
    
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    int fixedDelaySamples = 0; // Fixed delay length (static, no modulation)
    
    // Resampling-based modulation: controls resample ratio, not delay position
    float processTapeMod(TapeModState& state, double sampleRate, float wowDepthScale, float flutterDepthScale);
    
    // Hermite 4-point resampler (clean, efficient, stable under modulation)
    float resampleHermite(const float* buf, int bufMask, int readPos, float phase) const;
    
    // Tape saturation (soft clip)
    float tapeSaturate(float sample, float drive);
};
