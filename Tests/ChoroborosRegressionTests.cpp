/*
 * Choroboros - Regression harness for DSP and state persistence
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 */

#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"
#include "UI/DevPanel.h"
#include "UI/DevPanelSupport.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

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

static devpanel::CommandConsolePropertyComponent* findConsoleComponentRecursive(juce::Component& root)
{
    if (auto* console = dynamic_cast<devpanel::CommandConsolePropertyComponent*>(&root))
        return console;

    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        if (auto* child = root.getChildComponent(i))
        {
            if (auto* found = findConsoleComponentRecursive(*child))
                return found;
        }
    }
    return nullptr;
}

static void fillPitchModulatedSine(juce::AudioBuffer<float>& buffer,
                                   double sampleRate,
                                   double& carrierPhase,
                                   double& lfoPhase)
{
    constexpr double twoPi = 6.28318530717958647692;
    constexpr double lfoRateHz = 0.37;
    const int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i)
    {
        const double lfoValue = std::sin(lfoPhase);
        const double carrierHz = 220.0 + (110.0 * lfoValue);
        const double sample = 0.22 * std::sin(carrierPhase);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, i, static_cast<float>(sample));

        carrierPhase += (twoPi * carrierHz) / sampleRate;
        if (carrierPhase > twoPi)
            carrierPhase -= twoPi;

        lfoPhase += (twoPi * lfoRateHz) / sampleRate;
        if (lfoPhase > twoPi)
            lfoPhase -= twoPi;
    }
}

static double percentile(std::vector<double> values, double p)
{
    if (values.empty())
        return 0.0;
    std::sort(values.begin(), values.end());
    const double index = juce::jlimit(0.0, static_cast<double>(values.size() - 1),
                                      (p * 0.01) * static_cast<double>(values.size() - 1));
    const auto lo = static_cast<size_t>(std::floor(index));
    const auto hi = static_cast<size_t>(std::ceil(index));
    const double t = index - static_cast<double>(lo);
    return values[lo] + (values[hi] - values[lo]) * t;
}

static double percentileWarm(const std::vector<double>& values, double p)
{
    if (values.empty())
        return 0.0;
    if (values.size() == 1)
        return values.front();
    std::vector<double> warm(values.begin() + 1, values.end());
    return percentile(std::move(warm), p);
}

static juce::String parseFirstSlugFromListOutput(const juce::String& output)
{
    juce::StringArray lines;
    lines.addLines(output);
    for (const auto& lineRaw : lines)
    {
        const juce::String line = lineRaw.trim();
        if (line.isEmpty())
            continue;
        if (line.startsWithIgnoreCase("parameter slugs for")
            || line.startsWithIgnoreCase("global parameter slugs")
            || line.startsWithIgnoreCase("error:"))
            continue;

        auto slugChunk = line.upToFirstOccurrenceOf("[", false, false).trim();
        if (slugChunk.isEmpty())
            slugChunk = line;

        juce::StringArray tokens;
        tokens.addTokens(slugChunk, " \t", "");
        tokens.trim();
        tokens.removeEmptyStrings();
        if (!tokens.isEmpty())
            return tokens[0];
    }
    return {};
}

static void testConsoleCommandLatencyUnderAudioLoad()
{
    using Clock = std::chrono::steady_clock;

    struct BenchCase
    {
        juce::String label;
        std::function<juce::String()> makeCommand;
        int iterations = 1;
        bool expectSuccess = true;
    };

    struct BenchResult
    {
        std::vector<double> samplesUs;
        int errorCount = 0;
        juce::String firstError;
    };

    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(48000.0, 256);

    ChoroborosPluginEditor editor(proc);
    editor.setBounds(0, 0, 1028, 525);
    editor.resized();

    DevPanel devPanel(editor, proc);
    devPanel.setBounds(0, 0, 1280, 760);
    devPanel.resized();

    auto* console = findConsoleComponentRecursive(devPanel);
    REGRESS_ASSERT(console != nullptr, "DevPanel console component not found");
    if (console == nullptr)
        return;

    auto globalsResult = console->submitCommandForTesting("list globals", false);
    REGRESS_ASSERT(!globalsResult.output.startsWithIgnoreCase("ERROR:"), "`list globals` failed during setup");
    const juce::String globalSlug = parseFirstSlugFromListOutput(globalsResult.output);

    auto blueResult = console->submitCommandForTesting("list blue", false);
    REGRESS_ASSERT(!blueResult.output.startsWithIgnoreCase("ERROR:"), "`list blue` failed during setup");
    const juce::String engineSlug = parseFirstSlugFromListOutput(blueResult.output);

    const juce::String slug = globalSlug.isNotEmpty() ? globalSlug : (engineSlug.isNotEmpty() ? engineSlug : juce::String("knob_drag_sensitivity"));
    const auto unlockSetup = console->submitCommandForTesting("unlock " + slug, false);
    REGRESS_ASSERT(!unlockSetup.output.startsWithIgnoreCase("ERROR:"),
                   "Failed to unlock selected benchmark target slug before set/add/sub/reset tests");

    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    double carrierPhase = 0.0;
    double lfoPhase = 0.0;
    bool sawInvalidAudio = false;

    auto processAudioBlock = [&]()
    {
        fillPitchModulatedSine(audio, 48000.0, carrierPhase, lfoPhase);
        proc.processBlock(audio, midi);
        if (hasNaNOrInf(audio))
            sawInvalidAudio = true;
    };

    std::vector<BenchCase> cases;
    cases.push_back({ "help", [] { return juce::String("help"); }, 10, true });
    cases.push_back({ "engine green", [] { return juce::String("engine green"); }, 8, true });
    cases.push_back({ "engine blue", [] { return juce::String("engine blue"); }, 8, true });
    cases.push_back({ "engine red", [] { return juce::String("engine red"); }, 8, true });
    cases.push_back({ "engine purple", [] { return juce::String("engine purple"); }, 8, true });
    cases.push_back({ "engine black", [] { return juce::String("engine black"); }, 8, true });
    cases.push_back({ "hq on", [] { return juce::String("hq on"); }, 8, true });
    cases.push_back({ "hq off", [] { return juce::String("hq off"); }, 8, true });
    cases.push_back({ "view overview", [] { return juce::String("view overview"); }, 4, true });
    cases.push_back({ "view modulation", [] { return juce::String("view modulation"); }, 4, true });
    cases.push_back({ "view tone", [] { return juce::String("view tone"); }, 4, true });
    cases.push_back({ "view engine", [] { return juce::String("view engine"); }, 4, true });
    cases.push_back({ "view layout", [] { return juce::String("view layout"); }, 4, true });
    cases.push_back({ "view validation", [] { return juce::String("view validation"); }, 4, true });
    cases.push_back({ "view settings", [] { return juce::String("view settings"); }, 4, true });
    cases.push_back({ "bypass on", [] { return juce::String("bypass on"); }, 6, true });
    cases.push_back({ "bypass off", [] { return juce::String("bypass off"); }, 6, true });
    cases.push_back({ "set", [slug] { return "set " + slug + " 1.250"; }, 12, true });
    cases.push_back({ "get", [slug] { return "get " + slug; }, 12, true });
    cases.push_back({ "add", [slug] { return "add " + slug + " 0.125"; }, 12, true });
    cases.push_back({ "sub", [slug] { return "sub " + slug + " 0.125"; }, 12, true });
    cases.push_back({ "reset target", [slug] { return "reset " + slug; }, 6, true });
    cases.push_back({ "lock", [slug] { return "lock " + slug; }, 6, true });
    cases.push_back({ "unlock", [slug] { return "unlock " + slug; }, 6, true });
    cases.push_back({ "toggle hq", [] { return juce::String("toggle hq"); }, 6, true });
    cases.push_back({ "macro", [] { return juce::String("macro depth 75"); }, 8, true });
    cases.push_back({ "sweep", [slug] { return "sweep " + slug + " 0.100 0.900 80"; }, 5, true });
    cases.push_back({ "undo", [] { return juce::String("undo"); }, 10, true });
    cases.push_back({ "redo", [] { return juce::String("redo"); }, 10, true });
    cases.push_back({ "history", [] { return juce::String("history"); }, 6, true });
    cases.push_back({ "watch", [slug] { return "watch " + slug; }, 6, true });
    cases.push_back({ "unwatch", [slug] { return "unwatch " + slug; }, 6, true });
    cases.push_back({ "solo", [] { return juce::String("solo dry"); }, 6, true });
    cases.push_back({ "unsolo", [] { return juce::String("unsolo"); }, 6, true });
    cases.push_back({ "fx 0", [] { return juce::String("fx 0"); }, 4, true });
    cases.push_back({ "fx 1", [] { return juce::String("fx 1"); }, 4, true });
    cases.push_back({ "fx 2", [] { return juce::String("fx 2"); }, 4, true });
    cases.push_back({ "dump green", [] { return juce::String("dump green"); }, 2, true });
    cases.push_back({ "diff factory", [] { return juce::String("diff factory"); }, 3, true });
    cases.push_back({ "search rate", [] { return juce::String("search rate"); }, 3, true });
    cases.push_back({ "stats", [] { return juce::String("stats"); }, 6, true });
    cases.push_back({ "list globals", [] { return juce::String("list globals"); }, 3, true });
    cases.push_back({ "list blue", [] { return juce::String("list blue"); }, 2, true });
    cases.push_back({ "export script", [] { return juce::String("export script"); }, 2, true });
    cases.push_back({ "tutorial core", [] { return juce::String("tutorial core"); }, 2, true });
    cases.push_back({ "tutorial next", [] { return juce::String("tutorial next"); }, 2, true });
    cases.push_back({ "tutorial next section", [] { return juce::String("tutorial next section"); }, 2, true });
    cases.push_back({ "tutorial skip", [] { return juce::String("tutorial skip"); }, 2, true });
    cases.push_back({ "clear", [] { return juce::String("clear"); }, 2, true });

    std::vector<std::pair<juce::String, BenchResult>> results;
    results.reserve(cases.size());

    for (const auto& bench : cases)
    {
        BenchResult stats;
        stats.samplesUs.reserve(static_cast<size_t>(bench.iterations));

        for (int i = 0; i < bench.iterations; ++i)
        {
            processAudioBlock();

            const juce::String command = bench.makeCommand();
            const auto t0 = Clock::now();
            const auto result = console->submitCommandForTesting(command, false);
            const auto t1 = Clock::now();

            const auto elapsedUs =
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            stats.samplesUs.push_back(static_cast<double>(elapsedUs));

            if (result.output.startsWithIgnoreCase("ERROR:"))
            {
                ++stats.errorCount;
                if (stats.firstError.isEmpty())
                    stats.firstError = result.output;
            }

            processAudioBlock();
        }

        if (bench.expectSuccess)
        {
            if (stats.errorCount != 0)
            {
                std::cerr << "FAIL: Console command `" << bench.label.toStdString() << "` returned error: "
                          << stats.firstError.toStdString() << "\n";
                ++g_failCount;
            }
        }

        results.emplace_back(bench.label, std::move(stats));
    }

    REGRESS_ASSERT(!sawInvalidAudio, "Console/audio-load benchmark produced NaN/Inf in processed audio");

    std::cout << "Console command latency under synthetic audio load (pitch-modulated sine)\n";
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "Target slug for tuning commands: " << slug.toStdString() << "\n";
    std::cout << "Units: milliseconds\n";

    double worstP95Ms = 0.0;
    double worstWarmP95Ms = 0.0;
    juce::String worstLabel;
    juce::String worstWarmLabel;
    for (const auto& entry : results)
    {
        const auto& label = entry.first;
        const auto& stats = entry.second;
        if (stats.samplesUs.empty())
            continue;

        const double minMs = *std::min_element(stats.samplesUs.begin(), stats.samplesUs.end()) / 1000.0;
        const double maxMs = *std::max_element(stats.samplesUs.begin(), stats.samplesUs.end()) / 1000.0;
        const double avgMs = std::accumulate(stats.samplesUs.begin(), stats.samplesUs.end(), 0.0)
                           / static_cast<double>(stats.samplesUs.size()) / 1000.0;
        const double p50Ms = percentile(stats.samplesUs, 50.0) / 1000.0;
        const double p95Ms = percentile(stats.samplesUs, 95.0) / 1000.0;
        const double p99Ms = percentile(stats.samplesUs, 99.0) / 1000.0;
        const double warmP95Ms = percentileWarm(stats.samplesUs, 95.0) / 1000.0;

        if (p95Ms > worstP95Ms)
        {
            worstP95Ms = p95Ms;
            worstLabel = label;
        }
        if (warmP95Ms > worstWarmP95Ms)
        {
            worstWarmP95Ms = warmP95Ms;
            worstWarmLabel = label;
        }

        std::cout << "  " << label.toStdString()
                  << "  n=" << stats.samplesUs.size()
                  << "  avg=" << avgMs
                  << "  p50=" << p50Ms
                  << "  p95=" << p95Ms
                  << "  warm_p95=" << warmP95Ms
                  << "  p99=" << p99Ms
                  << "  min=" << minMs
                  << "  max=" << maxMs
                  << "  errors=" << stats.errorCount
                  << "\n";
    }

    std::cout << "Worst p95 command: " << worstLabel.toStdString() << " (" << worstP95Ms << " ms)\n";
    std::cout << "Worst warm p95 command: " << worstWarmLabel.toStdString() << " (" << worstWarmP95Ms << " ms)\n";
    std::cout << "Note: persistence/file-dialog commands were intentionally excluded in this run (save defaults, cp json, import script).\n";

    auto warmP95For = [&results](const juce::String& label) -> double
    {
        for (const auto& entry : results)
        {
            if (entry.first.equalsIgnoreCase(label))
                return percentileWarm(entry.second.samplesUs, 95.0) / 1000.0;
        }
        return 0.0;
    };

    REGRESS_ASSERT(warmP95For("set") <= 90.0, "Warm p95 for `set` exceeded 90ms");
    REGRESS_ASSERT(warmP95For("reset target") <= 250.0, "Warm p95 for `reset <target>` exceeded 250ms");
    REGRESS_ASSERT(warmP95For("list blue") <= 140.0, "Warm p95 for `list blue` exceeded 140ms");
    REGRESS_ASSERT(warmP95For("stats") <= 140.0, "Warm p95 for `stats` exceeded 140ms");
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
    testConsoleCommandLatencyUnderAudioLoad();

    std::cout << "----------------------------\n";
    if (g_failCount == 0)
        std::cout << "PASS: All regression tests passed.\n";
    else
        std::cerr << "FAIL: " << g_failCount << " assertion(s) failed.\n";

    return g_failCount > 0 ? 1 : 0;
}
