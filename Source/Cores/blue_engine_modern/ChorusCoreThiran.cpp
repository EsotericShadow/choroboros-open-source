#include "ChorusCoreThiran.h"
#include "../../DSP/ChorusDSP.h"
#include <algorithm>
#include <cmath>

namespace
{
float wrapPhasePositive(float phase)
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    while (phase >= twoPi)
        phase -= twoPi;
    while (phase < 0.0f)
        phase += twoPi;

    return phase;
}

float thiranProcessAllpassOneStep(const std::array<float, ChorusCoreThiran::thiranCoefficientCount>& a,
                                  std::array<float, ChorusCoreThiran::thiranAllpassOrder>& s,
                                  const float input)
{
    constexpr int N = ChorusCoreThiran::thiranAllpassOrder;
    static_assert(ChorusCoreThiran::thiranCoefficientCount == N + 1, "Thiran coeff count");

    const float output = a[static_cast<size_t>(N)] * input + s[0];

    for (int k = 0; k < N - 1; ++k)
    {
        s[static_cast<size_t>(k)] = a[static_cast<size_t>(N - 1 - k)] * input
                                 - a[static_cast<size_t>(k + 1)] * output
                                 + s[static_cast<size_t>(k + 1)];
    }

    s[static_cast<size_t>(N - 1)] = a[0] * input - a[static_cast<size_t>(N)] * output;
    return output;
}
} // namespace

ChorusCoreThiran::ChorusCoreThiran() = default;

void ChorusCoreThiran::computeCoefficients(float D, std::array<float, ORDER + 1>& a)
{
    // Retained for compatibility with existing tests/tooling.
    const float dMin = static_cast<float>(ORDER) - 0.5f + 0.01f;
    const float dMax = static_cast<float>(ORDER) + 0.5f - 0.01f;
    const float Df = juce::jlimit(dMin, dMax, D);
    const int N = ORDER;

    a[0] = 1.0f;

    for (int k = 1; k <= N; ++k)
    {
        double binom = 1.0;
        for (int j = 1; j <= k; ++j)
            binom *= static_cast<double>(N - k + j) / static_cast<double>(j);

        double prod = 1.0;
        for (int n = 0; n <= N; ++n)
        {
            const double num = static_cast<double>(Df) - static_cast<double>(N) + static_cast<double>(n);
            const double den = static_cast<double>(Df) - static_cast<double>(N) + static_cast<double>(k) + static_cast<double>(n);

            if (std::abs(den) < 1.0e-12)
            {
                prod = 0.0;
                break;
            }

            prod *= num / den;
        }

        const double sign = (k & 1) ? -1.0 : 1.0;
        a[static_cast<size_t>(k)] = static_cast<float>(sign * binom * prod);
    }
}

void ChorusCoreThiran::computeCoefficientsForTests(float D, std::array<float, thiranCoefficientCount>& out)
{
    static_assert(ORDER + 1 == thiranCoefficientCount, "coefficient array size");
    std::array<float, ORDER + 1> tmp {};
    computeCoefficients(D, tmp);
    out = tmp;
}

float ChorusCoreThiran::processAllpassForTests(const std::array<float, thiranCoefficientCount>& coeffs,
                                               std::array<float, thiranAllpassOrder>& state,
                                               const float input)
{
    return thiranProcessAllpassOneStep(coeffs, state, input);
}

void ChorusCoreThiran::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP*)
{
    spec = processSpec;

    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 4;

    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0f)) + guardMarginSamples;

    bufferSize = 1;
    while (bufferSize < maxDelaySamples + 4)
        bufferSize <<= 1;
    bufferMask = bufferSize - 1;

    delayBuffers.resize(static_cast<size_t>(spec.numChannels));
    writePositions.resize(static_cast<size_t>(spec.numChannels));
    channels.resize(static_cast<size_t>(spec.numChannels));

    for (size_t ch = 0; ch < delayBuffers.size(); ++ch)
    {
        delayBuffers[ch].assign(static_cast<size_t>(bufferSize), 0.0f);
        writePositions[ch] = 0;
        channels[ch].resetState();
        auto& phases = channels[ch].voicePhases;
        for (size_t voiceIndex = 0; voiceIndex < phases.size(); ++voiceIndex)
        {
            const float phaseSeed = 0.61f * static_cast<float>(voiceIndex)
                                  + 0.43f * static_cast<float>(ch)
                                  + 0.29f * static_cast<float>(voiceIndex * voiceIndex);
            phases[voiceIndex] = wrapPhasePositive(phaseSeed);
        }
    }

    // Similar center smoothing to cubic NQ so the family feels related.
    centreDelaySmoothAlpha = 1.0f - std::exp(-1.0f / (0.003f * static_cast<float>(spec.sampleRate)));

    // Small extra slew smoothing for HQ polish.
    {
        constexpr float delaySmoothingTauMs = 6.0f;
        delaySmoothingCoeff = std::exp(-1.0f / (delaySmoothingTauMs * 0.001f * static_cast<float>(spec.sampleRate)));
    }

    {
        constexpr float outputLpCutoffHz = 8200.0f;
        const float w = 2.0f * juce::MathConstants<float>::pi * outputLpCutoffHz
                      / static_cast<float>(spec.sampleRate);
        outputLpAlpha = std::exp(-w);
    }

    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

void ChorusCoreThiran::reset()
{
    for (auto& buffer : delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);

    std::fill(writePositions.begin(), writePositions.end(), 0);

    for (auto& ch : channels)
        ch.resetState();

    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& phases = channels[ch].voicePhases;
        for (size_t voiceIndex = 0; voiceIndex < phases.size(); ++voiceIndex)
        {
            const float phaseSeed = 0.61f * static_cast<float>(voiceIndex)
                                  + 0.43f * static_cast<float>(ch)
                                  + 0.29f * static_cast<float>(voiceIndex * voiceIndex);
            phases[voiceIndex] = wrapPhasePositive(phaseSeed);
        }
    }

    smoothedCentreDelay.fill(0.0f);
    centreDelayInitialized.fill(false);
}

float ChorusCoreThiran::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

void ChorusCoreThiran::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());

    const int p = dsp.runtimeTuningSnapshot.thiranReductionProbe;
    const int reductionProbe = (p >= 1 && p <= 4) ? p : 0;
    const bool snapInnerCentreProbe = (p == 13);

    const float guardSamples = getGuardSamples();
    const float maxDelay = getMaxDelaySamples();

    constexpr float maximumDelayModulation = 20.0f;
    const float* const cdPerMs = dsp.getCentreDelayMsPerSample(blockNumSamples);
    const float centreDelaySamplesBlock = currentCentreDelayMs * static_cast<float>(spec.sampleRate) / 1000.0f;
    const float depthSamples = maximumDelayModulation * static_cast<float>(spec.sampleRate) / 1000.0f;
    const float baseRateHz = juce::jlimit(0.01f, 20.0f, dsp.smoothedRate.getCurrentValue());
    const float phaseScale = juce::MathConstants<float>::twoPi / static_cast<float>(spec.sampleRate);
    const std::array<float, 4> voiceRateMultipliers {{ 0.71f, 1.09f, 0.47f, 1.61f }};

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    const bool pushThiranTelem = (dsp.getCurrentResolvedCoreId() == choroboros::CoreId::thiran);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;
        auto& buffer = delayBuffers[static_cast<size_t>(ch)];
        int& writePos = writePositions[static_cast<size_t>(ch)];
        auto& tc = channels[static_cast<size_t>(ch)];
        const auto chIdx = static_cast<size_t>(ch);
        const float stereoSign = (ch == 0) ? 1.0f : -1.0f;

        if (!centreDelayInitialized[chIdx])
        {
            smoothedCentreDelay[chIdx] = centreDelaySamplesBlock;
            centreDelayInitialized[chIdx] = true;
        }

        if (!tc.delayInitialized)
        {
            float initialDelay = centreDelaySamplesBlock + depthSamples * channelLfo[0];
            initialDelay = juce::jlimit(guardSamples, maxDelay, initialDelay);
            tc.smoothedDelay = initialDelay;
            tc.delayInitialized = true;
        }

        for (int i = 0; i < blockNumSamples; ++i)
        {
            const float centreMsThis = cdPerMs != nullptr ? cdPerMs[i] : currentCentreDelayMs;
            const float centreDelaySamplesThis = centreMsThis * static_cast<float>(spec.sampleRate) / 1000.0f;

            if (snapInnerCentreProbe)
                smoothedCentreDelay[chIdx] = centreDelaySamplesThis;
            else
                smoothedCentreDelay[chIdx] += centreDelaySmoothAlpha * (centreDelaySamplesThis - smoothedCentreDelay[chIdx]);

            float targetDelay = smoothedCentreDelay[chIdx] + depthSamples * channelLfo[i];
            targetDelay = juce::jlimit(guardSamples, maxDelay, targetDelay);

            if (reductionProbe == 2)
                tc.smoothedDelay = targetDelay;
            else
                tc.smoothedDelay = delaySmoothingCoeff * tc.smoothedDelay + (1.0f - delaySmoothingCoeff) * targetDelay;

            const float in = inputSamples[i];

            // Write input first, matching the cubic core's basic operating style.
            buffer[static_cast<size_t>(writePos)] = in;
            writePos = (writePos + 1) & bufferMask;

            const float d = tc.smoothedDelay;
            const float lfoA = channelLfo[i];
            const float lfoB = (numChannels >= 2)
                ? ((ch == 0) ? lfoRight[i] : lfoLeft[i])
                : -lfoA;
            const float outerStereoBias = outerStereoOffsetSamples * stereoSign;
            const float anchorA = depthSamples * lfoA;
            const float anchorB = depthSamples * lfoB;

            std::array<float, 4> supportVoiceMods {};
            for (size_t voiceIndex = 0; voiceIndex < supportVoiceMods.size(); ++voiceIndex)
            {
                const float phase = tc.voicePhases[voiceIndex];
                const float sinePrimary = std::sin(phase);
                const float sineSecondary = std::sin(phase * (voiceIndex >= 2 ? 2.0f : 1.0f)
                                                     + 0.37f * static_cast<float>(voiceIndex + 1));
                supportVoiceMods[voiceIndex] = 0.82f * sinePrimary + 0.18f * sineSecondary;

                const float phaseInc = phaseScale * baseRateHz * voiceRateMultipliers[voiceIndex];
                tc.voicePhases[voiceIndex] = wrapPhasePositive(phase + phaseInc);
            }

            // Voice 1: main stable chorus read
            const float v1 = readCubic(ch, d);

            const float d2 = juce::jlimit(guardSamples, maxDelay,
                d + voice2StaticOffsetSamples
                  + voice2ModDepthScale * depthSamples * supportVoiceMods[0]
                  + 0.025f * anchorA);

            const float d3 = juce::jlimit(guardSamples, maxDelay,
                d + voice3StaticOffsetSamples
                  + voice3ModDepthScale * depthSamples * supportVoiceMods[1]
                  - 0.020f * anchorB);

            const float d4 = juce::jlimit(guardSamples, maxDelay,
                d + voice4StaticOffsetSamples + outerStereoBias
                  + voice4ModDepthScale * depthSamples * supportVoiceMods[2]);

            const float d5 = juce::jlimit(guardSamples, maxDelay,
                d + voice5StaticOffsetSamples - outerStereoBias
                  + voice5ModDepthScale * depthSamples * supportVoiceMods[3]);

            const float v2 = readCubic(ch, d2);
            const float v3 = readCubic(ch, d3);
            const float v4 = readCubic(ch, d4);
            const float v5 = readCubic(ch, d5);

            const float wet = (reductionProbe == 3)
                ? v1
                : (voiceGain[0] * v1
                   + voiceGain[1] * v2
                   + voiceGain[2] * v3
                   + voiceGain[3] * v4
                   + voiceGain[4] * v5);

            const float out = (reductionProbe == 1)
                ? wet
                : ((1.0f - outputLpAlpha) * wet + outputLpAlpha * tc.outputLpState);

            tc.outputLpState = out;
            outputSamples[i] = out;

            if (pushThiranTelem && ch == 0)
                dsp.pushThiranTelemetrySample(0.0f, (reductionProbe == 3) ? 0.0f : 1.0f);
        }
    }
}
