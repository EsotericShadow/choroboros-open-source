/*
 * Choroboros - Regression harness for DSP and state persistence
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#include "Plugin/PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <iostream>
#include <cstdlib>

static int g_failCount = 0;

#define REGRESS_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::cerr << "FAIL: " << msg << " (at " << __FILE__ << ":" << __LINE__ << ")\n"; \
        ++g_failCount; \
    } \
} while(0)

static bool isFinite(float x) { return std::isfinite(x); }
static bool hasNaNOrInf(const juce::AudioBuffer<float>& buf)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* p = buf.getReadPointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            if (!isFinite(p[i])) return true;
    }
    return false;
}

static void testProcessBlockSizes()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buf(2, 4096);
    juce::MidiBuffer midi;

    for (int blockSize : {1, 64, 128, 256, 512, 1024, 2048, 4096})
    {
        buf.setSize(2, blockSize, false, false, true);
        buf.clear();
        proc.processBlock(buf, midi);
        if (hasNaNOrInf(buf)) { REGRESS_ASSERT(false, "processBlock blockSize produced NaN/Inf"); }
    }
}

static void testEngineHQTorture()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    buf.clear();

    auto* rateParam = proc.getParameters()[0];
    auto* engineParam = proc.getParameters()[5];
    auto* hqParam = proc.getParameters()[6];

    const int cycles = 50;
    for (int i = 0; i < cycles; ++i)
    {
        if (engineParam) engineParam->setValueNotifyingHost(static_cast<float>((i % 5)) / 4.0f);
        if (hqParam) hqParam->setValueNotifyingHost((i % 2) ? 1.0f : 0.0f);
        if (rateParam) rateParam->setValueNotifyingHost(0.1f + 0.8f * i / static_cast<float>(cycles));
        proc.processBlock(buf, midi);
        if (hasNaNOrInf(buf)) { REGRESS_ASSERT(false, "Engine/HQ torture produced NaN/Inf"); }
    }
}

static void testStateRoundTrip()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    juce::MemoryBlock state1;
    proc.getStateInformation(state1);
    REGRESS_ASSERT(state1.getSize() > 0, "getStateInformation returned empty");

    auto* rateParam = proc.getParameters()[0];
    if (rateParam) rateParam->setValueNotifyingHost(0.75f);

    juce::MemoryBlock state2;
    proc.getStateInformation(state2);

    proc.setStateInformation(state2.getData(), static_cast<int>(state2.getSize()));
    juce::MemoryBlock state3;
    proc.getStateInformation(state3);

    REGRESS_ASSERT(state2.getSize() == state3.getSize(),
        "State round-trip size mismatch");
}

static void testMaxBlockChannels()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(48000.0, 4096);

    juce::AudioBuffer<float> buf(2, 4096);
    juce::MidiBuffer midi;
    buf.clear();
    proc.processBlock(buf, midi);
    REGRESS_ASSERT(!hasNaNOrInf(buf), "Max block 2ch produced NaN/Inf");
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    juce::ScopedJuceInitialiser_GUI init;

    std::cout << "Choroboros Regression Harness\n";
    std::cout << "----------------------------\n";

    testProcessBlockSizes();
    testEngineHQTorture();
    testStateRoundTrip();
    testMaxBlockChannels();

    std::cout << "----------------------------\n";
    if (g_failCount == 0)
        std::cout << "PASS: All regression tests passed.\n";
    else
        std::cerr << "FAIL: " << g_failCount << " assertion(s) failed.\n";

    return g_failCount > 0 ? 1 : 0;
}
