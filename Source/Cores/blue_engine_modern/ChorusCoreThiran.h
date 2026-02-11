#pragma once

#include "../ChorusCore.h"
#include <array>
#include <vector>

// Windowed-sinc polyphase FIR fractional delay chorus core
// Blue HQ mode (replaces Thiran allpass - cleaner for time-varying delay)
class ChorusCoreThiran : public ChorusCore
{
public:
    ChorusCoreThiran();
    ~ChorusCoreThiran() override = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;
    
    float getGuardSamples() const override { return 16.0f; } // Need guard for FIR taps
    float getMaxDelaySamples() const override;
    
private:
    // Windowed-sinc polyphase FIR fractional delay
    struct SincFD
    {
        static constexpr int PHASES = 1024;  // HQ: 1024 phases for smooth interpolation
        static constexpr int TAPS = 32;      // HQ: 32 taps for high quality
        static constexpr int HALF = TAPS / 2;
        
        std::array<std::array<float, TAPS>, PHASES> table{};
        bool tableBuilt = false;
        
        static inline float sinc(float x)
        {
            if (std::abs(x) < 1e-8f) return 1.0f;
            const float pix = juce::MathConstants<float>::pi * x;
            return std::sin(pix) / pix;
        }
        
        // Blackman window (better than Hann for HQ)
        static inline float window(int n, int taps)
        {
            const float a = 2.0f * juce::MathConstants<float>::pi * static_cast<float>(n) / static_cast<float>(taps - 1);
            return 0.42f - 0.5f * std::cos(a) + 0.08f * std::cos(2.0f * a);
        }
        
        void buildTable()
        {
            for (int p = 0; p < PHASES; ++p)
            {
                const float frac = static_cast<float>(p) / static_cast<float>(PHASES - 1); // 0..1
                float sum = 0.0f;
                
                for (int k = 0; k < TAPS; ++k)
                {
                    // Center taps around 0, with fractional shift
                    const float x = static_cast<float>(k - (HALF - 1)) - frac;
                    float h = sinc(x) * window(k, TAPS);
                    table[static_cast<size_t>(p)][static_cast<size_t>(k)] = h;
                    sum += h;
                }
                
                // Normalize DC gain to 1.0
                const float inv = (sum != 0.0f) ? (1.0f / sum) : 1.0f;
                for (int k = 0; k < TAPS; ++k)
                    table[static_cast<size_t>(p)][static_cast<size_t>(k)] *= inv;
            }
            tableBuilt = true;
        }
        
        // Read from ring buffer at fractional position
        float read(const float* buf, int bufMask, int writePos, float delaySamples) const
        {
            // Calculate read position (behind write head)
            float readPos = static_cast<float>(writePos) - delaySamples;
            
            // Wrap to positive range (same as cubic core)
            while (readPos < 0.0f)
                readPos += static_cast<float>(bufMask + 1);
            
            const int iCenter = static_cast<int>(std::floor(readPos));
            const float frac = readPos - static_cast<float>(iCenter); // Fractional part in [0,1)
            
            // CRITICAL FIX: Interpolate between phases to prevent phase jitter (zipper)
            // Use floor + linear interpolation instead of lround
            float phaseF = frac * static_cast<float>(PHASES - 1);
            int p0 = static_cast<int>(phaseF);
            float t = phaseF - static_cast<float>(p0); // Fractional part for interpolation
            int p1 = juce::jmin(p0 + 1, PHASES - 1);
            
            const auto& h0 = table[static_cast<size_t>(p0)];
            const auto& h1 = table[static_cast<size_t>(p1)];
            
            float y = 0.0f;
            
            // CRITICAL FIX: Correct tap indexing - use iCenter + tapOffset (not minus)
            // This reads backwards in time (older samples) correctly aligned with the kernel
            for (int k = 0; k < TAPS; ++k)
            {
                // Interpolate between neighboring phase tables
                float hk = h0[static_cast<size_t>(k)] + t * (h1[static_cast<size_t>(k)] - h0[static_cast<size_t>(k)]);
                
                // Tap offset: center taps around iCenter
                const int tapOffset = k - (HALF - 1);
                
                // CRITICAL: Use iCenter + tapOffset (not minus) to match kernel orientation
                const int idx = (iCenter + tapOffset) & bufMask;
                
                y += buf[static_cast<size_t>(idx)] * hk;
            }
            return y;
        }
    };
    
    std::vector<SincFD> sincFilters; // One per channel
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    std::vector<float> smoothedDelays; // Per-channel smoothed delay values
    std::vector<bool> delayInitialized;
    int bufferSize = 0;
    int bufferMask = 0;
    juce::dsp::ProcessSpec spec;
    int maxDelaySamples = 0;
    
    float readSinc(int channel, float delaySamples) const;
};
