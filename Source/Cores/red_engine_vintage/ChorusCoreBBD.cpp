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
 * but WITHOUT ANY IMPLIED WARRANTY of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ChorusCoreBBD.h"
#include "../../DSP/ChorusDSP.h"
#include <cmath>
#include <algorithm>

ChorusCoreBBD::ChorusCoreBBD()
{
}

void ChorusCoreBBD::prepare(const juce::dsp::ProcessSpec& processSpec, ChorusDSP* dsp)
{
    spec = processSpec;

    constexpr float maximumDelayModulation = 20.0f;
    constexpr float oscVolumeMultiplier = 0.5f;
    constexpr float maxDepth = 1.0f;
    constexpr float maxCentreDelayMs = 100.0f;
    constexpr int guardMarginSamples = 4;

    maxDelaySamples = static_cast<int>(std::ceil(
        (maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
        * spec.sampleRate / 1000.0)) + guardMarginSamples;

    channels.resize(static_cast<size_t>(spec.numChannels));

    // Compute 5th-order Butterworth cutoff: 0.5 * minClock (jpcima style)
    // minClock = stages / (2 * maxDelaySec) = worst-case for longest delay
    float maxDelaySec = 0.1f;
    int stages = 1024;
    if (dsp != nullptr)
    {
        maxDelaySec = juce::jmax(0.001f, dsp->getRuntimeTuning().bbdDelayMaxMs.load() * 0.001f);
        stages = juce::jlimit(256, 2048, static_cast<int>(dsp->getRuntimeTuning().bbdStages.load()));
    }
    const float minClockHz = static_cast<float>(stages) / (2.0f * maxDelaySec);
    const float cutoffHz = juce::jlimit(500.0f, static_cast<float>(spec.sampleRate) * 0.4f, 0.5f * minClockHz);

    filterCoeffs = choroboros::designBBD5thOrderButterworth(cutoffHz, static_cast<float>(spec.sampleRate));

    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& chan = channels[ch];
        chan.stages.assign(static_cast<size_t>(BBD_STAGES_MAX), 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        chan.prevFilteredInput = 0.0f;
        chan.inputFilter.setCoeffs(filterCoeffs);
        chan.outputFilter.setCoeffs(filterCoeffs);
        chan.smoothedDelayMs.reset(spec.sampleRate, 0.02f);
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f);
        chan.smoothedFilterCutoffHz = cutoffHz;
        chan.lastDesignedFilterCutoffHz = cutoffHz;
    }
}

void ChorusCoreBBD::reset()
{
    for (auto& chan : channels)
    {
        std::fill(chan.stages.begin(), chan.stages.end(), 0.0f);
        chan.head = 0;
        chan.clockPhase = 0.0;
        chan.heldPrev = 0.0f;
        chan.heldNext = 0.0f;
        chan.prevFilteredInput = 0.0f;
        chan.inputFilter.reset();
        chan.outputFilter.reset();
        chan.smoothedClockFreq = 5000.0f;
        chan.smoothedDelayMs.setCurrentAndTargetValue(20.0f);
        chan.lastDesignedFilterCutoffHz = -1.0f;
    }
}

float ChorusCoreBBD::getMaxDelaySamples() const
{
    return static_cast<float>(maxDelaySamples) - getGuardSamples();
}

float ChorusCoreBBD::processBBDChannel(int channel, float input, float clockFreq, float clockSmoothCoeff,
                                       int effectiveStages)
{
    auto& chan = channels[static_cast<size_t>(channel)];

    chan.smoothedClockFreq = clockSmoothCoeff * chan.smoothedClockFreq + (1.0f - clockSmoothCoeff) * clockFreq;
    const float smoothClockFreq = chan.smoothedClockFreq;

    // 5th-order Butterworth anti-aliasing
    const float filteredInput = chan.inputFilter.processSample(input);

    const int delayStages = effectiveStages / 2;

    const double clockPhaseInc = static_cast<double>(smoothClockFreq) / static_cast<double>(spec.sampleRate);
    chan.clockPhase += clockPhaseInc;

    while (chan.clockPhase >= 1.0)
    {
        // Input interpolation (Raffel): delta*cur + (1-delta)*prev
        // delta = fraction of sample when tick occurred
        const int tickCount = static_cast<int>(chan.clockPhase);
        const float delta = static_cast<float>((static_cast<double>(tickCount) - (chan.clockPhase - clockPhaseInc)) / clockPhaseInc);
        const float toWrite = delta * filteredInput + (1.0f - delta) * chan.prevFilteredInput;

        chan.clockPhase -= 1.0;

        chan.stages[static_cast<size_t>(chan.head)] = toWrite;
        chan.head = (chan.head + 1) % effectiveStages;

        int readPos = (chan.head - delayStages + effectiveStages) % effectiveStages;
        chan.heldPrev = chan.heldNext;
        chan.heldNext = chan.stages[static_cast<size_t>(readPos)];

        chan.prevFilteredInput = filteredInput;
    }

    // When we don't tick, prevFilteredInput still updates for next tick's interpolation
    chan.prevFilteredInput = filteredInput;

    const float t = static_cast<float>(chan.clockPhase);
    const float held = chan.heldPrev + t * (chan.heldNext - chan.heldPrev);

    return chan.outputFilter.processSample(held);
}

void ChorusCoreBBD::processDelay(ChorusDSP& dsp, juce::dsp::AudioBlock<float>& block, float currentCentreDelayMs)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int blockNumSamples = static_cast<int>(block.getNumSamples());
    const auto& tuning = dsp.runtimeTuningSnapshot;

    const float remappedCentreDelayMs = tuning.bbdCentreBaseMs + (currentCentreDelayMs - 8.0f) * tuning.bbdCentreScale;
    const float depthMs = tuning.bbdDepthMs;

    const float delaySmoothingMs = juce::jmax(0.0f, tuning.bbdDelaySmoothingMs);
    if (delaySmoothingMs != lastDelaySmoothingMs)
    {
        const float rampSeconds = delaySmoothingMs * 0.001f;
        for (auto& chan : channels)
        {
            const float current = chan.smoothedDelayMs.getCurrentValue();
            chan.smoothedDelayMs.reset(spec.sampleRate, rampSeconds);
            chan.smoothedDelayMs.setCurrentAndTargetValue(current);
        }
        lastDelaySmoothingMs = delaySmoothingMs;
    }

    const float clockSmoothMs = juce::jmax(0.001f, tuning.bbdClockSmoothingMs);
    const float clockSmoothCoeff = std::exp(-1.0f / (clockSmoothMs * 0.001f * static_cast<float>(spec.sampleRate)));
    const float clockMinHz = juce::jmax(20.0f, tuning.bbdClockMinHz);
    const float clockMaxRatio = juce::jlimit(0.05f, 1.0f, tuning.bbdClockMaxRatio);
    const int effectiveStages = juce::jlimit(256, 2048, static_cast<int>(tuning.bbdStages));

    auto* lfoLeft = dsp.lfoBuffer.getReadPointer(0);
    auto* lfoRight = (numChannels >= 2) ? dsp.cosBuffer.getReadPointer(0) : lfoLeft;

    const float fs = static_cast<float>(spec.sampleRate);
    const float nyquistSafeClock = 0.45f * fs;
    const float ratioClockLimit = fs * clockMaxRatio;
    const float maxClockFreq = juce::jmax(clockMinHz + 1.0f, juce::jmin(nyquistSafeClock, ratioClockLimit));

    const float filterMinHz = juce::jmax(20.0f, tuning.bbdFilterCutoffMinHz);
    const float filterMaxHz = juce::jmax(filterMinHz, tuning.bbdFilterCutoffMaxHz);
    const float filterMaxRatio = juce::jlimit(0.1f, 0.5f, tuning.bbdFilterMaxRatio);
    const float filterMaxByRatioHz = fs * filterMaxRatio;
    const float effectiveFilterMaxHz = juce::jmax(filterMinHz, juce::jmin(filterMaxHz, filterMaxByRatioHz));
    const float filterScale = juce::jmax(0.0f, tuning.bbdFilterCutoffScale);
    const float centreDelayMsForFilter = juce::jlimit(tuning.bbdDelayMinMs, tuning.bbdDelayMaxMs, remappedCentreDelayMs);
    const float centreDelaySecForFilter = juce::jmax(0.001f, centreDelayMsForFilter * 0.001f);
    float approxClockHz = static_cast<float>(effectiveStages) / (2.0f * centreDelaySecForFilter);
    approxClockHz = juce::jmin(approxClockHz, fs);
    approxClockHz = juce::jlimit(clockMinHz, maxClockFreq, approxClockHz);
    const float targetFilterCutoffHz = juce::jlimit(filterMinHz, effectiveFilterMaxHz, filterScale * approxClockHz);

    const float filterSmoothMs = juce::jmax(0.0f, tuning.bbdFilterSmoothingMs);
    const float blockSeconds = static_cast<float>(blockNumSamples) / fs;
    const float filterBlockCoeff = (filterSmoothMs > 0.0f)
        ? std::exp(-blockSeconds / (filterSmoothMs * 0.001f))
        : 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* inputSamples = block.getChannelPointer(ch);
        auto* outputSamples = block.getChannelPointer(ch);
        const float* channelLfo = (ch == 0) ? lfoLeft : lfoRight;

        auto& chan = channels[static_cast<size_t>(ch)];

        if (chan.smoothedFilterCutoffHz <= 0.0f || !std::isfinite(chan.smoothedFilterCutoffHz))
            chan.smoothedFilterCutoffHz = targetFilterCutoffHz;

        if (filterSmoothMs > 0.0f)
            chan.smoothedFilterCutoffHz = filterBlockCoeff * chan.smoothedFilterCutoffHz + (1.0f - filterBlockCoeff) * targetFilterCutoffHz;
        else
            chan.smoothedFilterCutoffHz = targetFilterCutoffHz;

        if (chan.lastDesignedFilterCutoffHz < 0.0f
            || std::abs(chan.smoothedFilterCutoffHz - chan.lastDesignedFilterCutoffHz) >= 1.0f)
        {
            filterCoeffs = choroboros::designBBD5thOrderButterworth(chan.smoothedFilterCutoffHz, fs);
            chan.inputFilter.setCoeffs(filterCoeffs);
            chan.outputFilter.setCoeffs(filterCoeffs);
            chan.lastDesignedFilterCutoffHz = chan.smoothedFilterCutoffHz;
        }

        for (int i = 0; i < blockNumSamples; ++i)
        {
            float targetDelayMs = remappedCentreDelayMs + depthMs * channelLfo[i];
            targetDelayMs = juce::jlimit(tuning.bbdDelayMinMs, tuning.bbdDelayMaxMs, targetDelayMs);

            chan.smoothedDelayMs.setTargetValue(targetDelayMs);
            float delayMs = chan.smoothedDelayMs.getNextValue();

            float delaySeconds = delayMs * 0.001f;
            float clockFreq = static_cast<float>(effectiveStages) / (2.0f * delaySeconds);

            // Cap clock at sample rate (jpcima)
            clockFreq = juce::jmin(clockFreq, fs);

            clockFreq = juce::jlimit(clockMinHz, maxClockFreq, clockFreq);

            const float in = inputSamples[i];
            const float out = processBBDChannel(ch, in, clockFreq, clockSmoothCoeff, effectiveStages);
            outputSamples[i] = out;
        }
    }
}
