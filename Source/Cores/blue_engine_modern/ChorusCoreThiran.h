#pragma once

#include "../ChorusCore.h"
#include "../InterpolationUtils.h"
#include <array>
#include <vector>

class ChorusCoreThiran : public ChorusCore
{
public:
    ChorusCoreThiran();
    ~ChorusCoreThiran() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec, ChorusDSP* dsp = nullptr) override;
    void reset() override;
    void processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs) override;

    // Match cubic-style moving-read requirements
    float getGuardSamples() const override { return 2.0f; }
    float getMaxDelaySamples() const override;

    // Kept for compatibility with any existing tooling/tests that reference these.
    static constexpr int thiranCoefficientCount = 6;
    static constexpr int thiranAllpassOrder = 5;
    static void computeCoefficientsForTests(float D, std::array<float, thiranCoefficientCount>& out);
    static float processAllpassForTests(const std::array<float, thiranCoefficientCount>& coeffs,
                                        std::array<float, thiranAllpassOrder>& state,
                                        float input);

private:
    static constexpr int ORDER = 5;

    struct ChannelState
    {
        float smoothedDelay = 0.0f;
        bool delayInitialized = false;
        float outputLpState = 0.0f;
        std::array<float, 4> voicePhases {{ 0.0f, 1.7f, 3.4f, 5.1f }};

        void resetState()
        {
            smoothedDelay = 0.0f;
            delayInitialized = false;
            outputLpState = 0.0f;
            voicePhases = {{ 0.0f, 1.7f, 3.4f, 5.1f }};
        }
    };

    // Retained only so old test surfaces still compile.
    static void computeCoefficients(float D, std::array<float, ORDER + 1>& a);

    float readCubic(int channel, float delaySamples) const
    {
        const auto& buf = delayBuffers[static_cast<size_t>(channel)];
        const int writePos = writePositions[static_cast<size_t>(channel)];
        float readPos = static_cast<float>(writePos) - delaySamples;
        while (readPos < 0.0f)
            readPos += static_cast<float>(bufferSize);
        return readCubicInterp(buf.data(), bufferMask, readPos);
    }

    std::vector<ChannelState> channels;
    std::vector<std::vector<float>> delayBuffers;
    std::vector<int> writePositions;
    int bufferSize = 0;
    int bufferMask = 0;
    juce::dsp::ProcessSpec spec {};
    int maxDelaySamples = 0;

    float outputLpAlpha = 0.0f;
    float delaySmoothingCoeff = 0.0f;

    std::array<float, 2> smoothedCentreDelay {{ 0.0f, 0.0f }};
    std::array<bool, 2> centreDelayInitialized {{ false, false }};
    float centreDelaySmoothAlpha = 0.0f;

    // HQ multi-voice tuning. The live "thiran" slot is currently a denser
    // cubic chorus, so the support voices need genuinely different motion and
    // wider time separation than NQ, not a tight correlated tap cluster.
    float voice2StaticOffsetSamples = 28.0f;
    float voice3StaticOffsetSamples = -24.0f;
    float voice4StaticOffsetSamples = 63.0f;
    float voice5StaticOffsetSamples = -74.0f;
    float voice2ModDepthScale = 0.070f;
    float voice3ModDepthScale = 0.080f;
    float voice4ModDepthScale = 0.055f;
    float voice5ModDepthScale = 0.060f;
    float outerStereoOffsetSamples = 9.0f;

    std::array<float, 5> voiceGain {{ 0.50f, 0.17f, 0.13f, 0.11f, 0.09f }};
};
