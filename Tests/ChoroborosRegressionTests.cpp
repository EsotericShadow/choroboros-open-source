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
static bool nearlyEqual(float a, float b, float epsilon = 1.0e-4f)
{
    return std::abs(a - b) <= epsilon;
}

static juce::MemoryBlock makeLegacyApvtsState(ChoroborosAudioProcessor& proc)
{
    juce::XmlElement xmlState("VALUETREE");

    const auto addParam = [&xmlState, &proc](const char* paramId)
    {
        if (const auto* param = proc.getValueTreeState().getRawParameterValue(paramId))
        {
            auto paramElement = std::make_unique<juce::XmlElement>("PARAM");
            paramElement->setAttribute("id", paramId);
            paramElement->setAttribute("value", static_cast<double>(param->load()));
            xmlState.addChildElement(paramElement.release());
        }
    };

    addParam(ChoroborosAudioProcessor::RATE_ID);
    addParam(ChoroborosAudioProcessor::DEPTH_ID);
    addParam(ChoroborosAudioProcessor::OFFSET_ID);
    addParam(ChoroborosAudioProcessor::WIDTH_ID);
    addParam(ChoroborosAudioProcessor::COLOR_ID);
    addParam(ChoroborosAudioProcessor::HQ_ID);
    addParam(ChoroborosAudioProcessor::MIX_ID);

    juce::MemoryBlock legacyState;
    juce::AudioProcessor::copyXmlToBinary(xmlState, legacyState);
    return legacyState;
}

static bool assignmentTablesEqual(const choroboros::CoreAssignmentTable& a,
                                  const choroboros::CoreAssignmentTable& b)
{
    for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
    {
        for (int mode = 0; mode < choroboros::kEngineModeCount; ++mode)
        {
            const bool hqEnabled = (mode == 1);
            if (a.get(engine, hqEnabled) != b.get(engine, hqEnabled))
                return false;
        }
    }
    return true;
}

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

    proc.setModularCoresEnabled(true);
    proc.setCoreAssignment(0, false, choroboros::CoreId::tape);
    proc.setCoreAssignment(1, true, choroboros::CoreId::bbd);
    proc.setCoreAssignment(3, false, choroboros::CoreId::ensemble);
    const auto expectedAssignments = proc.getCoreAssignments();

    juce::MemoryBlock state2;
    proc.getStateInformation(state2);

    proc.setStateInformation(state2.getData(), static_cast<int>(state2.getSize()));
    juce::MemoryBlock state3;
    proc.getStateInformation(state3);

    REGRESS_ASSERT(state2.getSize() == state3.getSize(),
        "State round-trip size mismatch");
    REGRESS_ASSERT(proc.isModularCoresEnabled(), "Modular cores flag was not restored by state round-trip");
    REGRESS_ASSERT(assignmentTablesEqual(proc.getCoreAssignments(), expectedAssignments),
                   "Core assignment table changed after state round-trip");

    const bool duplicateApplied = proc.setCoreAssignment(4, true, choroboros::CoreId::tape);
    REGRESS_ASSERT(duplicateApplied, "Duplicate core assignment should be warned but allowed");
    const auto duplicateWarnings = proc.getDuplicateAssignmentWarnings();
    REGRESS_ASSERT(!duplicateWarnings.empty(), "Duplicate assignment warnings should report duplicates");

    const juce::MemoryBlock legacyState = makeLegacyApvtsState(proc);
    REGRESS_ASSERT(legacyState.getSize() > 0, "Failed to synthesize legacy APVTS state");

    ChoroborosAudioProcessor legacyProc;
    legacyProc.prepareToPlay(44100.0, 512);
    legacyProc.setStateInformation(legacyState.getData(), static_cast<int>(legacyState.getSize()));

    choroboros::CoreAssignmentTable legacyExpected;
    legacyExpected.resetToLegacy();
    REGRESS_ASSERT(!legacyProc.isModularCoresEnabled(),
                   "Legacy state without modular flag should restore modular mode as disabled");
    REGRESS_ASSERT(assignmentTablesEqual(legacyProc.getCoreAssignments(), legacyExpected),
                   "Legacy state without core assignments should fall back to legacy assignment map");
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

static void testKnownBadBundledEngineProfilesMigrateToDefaults()
{
    const juce::String knownBadProfilesJson = R"JSON({
  "engineParamProfiles": {
    "green": {"valid": true, "rate": 10.0000, "depth": 1.0000, "offset": 180.0000, "width": 2.0000, "mix": 1.0000, "color": 0.9900},
    "blue": {"valid": true, "rate": 2.2600, "depth": 0.5600, "offset": 55.0000, "width": 1.1300, "mix": 0.5000, "color": 0.0000},
    "red": {"valid": true, "rate": 10.0000, "depth": 1.0000, "offset": 180.0000, "width": 2.0000, "mix": 1.0000, "color": 0.4400},
    "purple": {"valid": true, "rate": 10.0000, "depth": 1.0000, "offset": 180.0000, "width": 2.0000, "mix": 0.5000, "color": 1.0000},
    "black": {"valid": true, "rate": 10.0000, "depth": 1.0000, "offset": 180.0000, "width": 2.0000, "mix": 0.5000, "color": 1.0000}
  }
})JSON";

    ChoroborosAudioProcessor proc;
    const juce::var parsed = juce::JSON::parse(knownBadProfilesJson);
    REGRESS_ASSERT(!parsed.isVoid(), "Known-bad engine profile JSON failed to parse");
    const auto* root = parsed.getDynamicObject();
    REGRESS_ASSERT(root != nullptr, "Known-bad engine profile JSON missing root object");
    REGRESS_ASSERT(root != nullptr && root->hasProperty("engineParamProfiles"),
                   "Known-bad engine profile JSON missing engineParamProfiles");

    if (root == nullptr || !root->hasProperty("engineParamProfiles"))
        return;

    proc.loadEngineParamProfilesFromVar(root->getProperty("engineParamProfiles"));

    struct ExpectedMappedProfile
    {
        float rate;
        float depth;
        float offset;
        float width;
        float mix;
        float color;
    };

    static constexpr std::array<ExpectedMappedProfile, 5> expectedProfiles {{
        { 0.65f, 0.21f, 33.0f, 1.5f, 0.5f, 0.16f },
        { 0.26f, 0.53f, 59.0f, 1.0f, 0.5f, 0.41f },
        { 0.62f, 0.21f, 56.0f, 1.5f, 0.5f, 0.5f },
        { 0.12f, 0.52f, 52.0f, 2.0f, 0.69f, 0.13f },
        { 0.8f, 0.35f, 41.0f, 1.59f, 0.5f, 0.28f }
    }};

    const auto& profiles = proc.getEngineParamProfiles();
    auto expectNear = [](float actual, float expected, const juce::String& label)
    {
        REGRESS_ASSERT(std::abs(actual - expected) < 0.0005f,
                       (label + " expected " + juce::String(expected, 4)
                        + ", got " + juce::String(actual, 4)).toStdString());
    };

    for (int i = 0; i < static_cast<int>(expectedProfiles.size()); ++i)
    {
        const auto& expected = expectedProfiles[static_cast<size_t>(i)];
        const auto& actual = profiles[static_cast<size_t>(i)];
        REGRESS_ASSERT(actual.valid, ("Engine " + juce::String(i) + " migrated profile should remain valid").toStdString());
        expectNear(actual.rate, proc.unmapParameterValue(ChoroborosAudioProcessor::RATE_ID, expected.rate),
                   "Engine " + juce::String(i) + " profile rate mismatch");
        expectNear(actual.depth, proc.unmapParameterValue(ChoroborosAudioProcessor::DEPTH_ID, expected.depth),
                   "Engine " + juce::String(i) + " profile depth mismatch");
        expectNear(actual.offset, proc.unmapParameterValue(ChoroborosAudioProcessor::OFFSET_ID, expected.offset),
                   "Engine " + juce::String(i) + " profile offset mismatch");
        expectNear(actual.width, proc.unmapParameterValue(ChoroborosAudioProcessor::WIDTH_ID, expected.width),
                   "Engine " + juce::String(i) + " profile width mismatch");
        expectNear(actual.mix, proc.unmapParameterValue(ChoroborosAudioProcessor::MIX_ID, expected.mix),
                   "Engine " + juce::String(i) + " profile mix mismatch");
        expectNear(actual.color, proc.unmapParameterValue(ChoroborosAudioProcessor::COLOR_ID, expected.color),
                   "Engine " + juce::String(i) + " profile color mismatch");
    }
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

static void testCanonicalPresetStateValidation()
{
    // Test that invalid preset states are rejected
    PresetState validState;
    REGRESS_ASSERT(validState.isValid(), "Default PresetState should be valid");

    PresetState invalidRate;
    invalidRate.rate = 999.0f;  // Out of range
    REGRESS_ASSERT(!invalidRate.isValid(), "PresetState with out-of-range rate should be invalid");

    PresetState invalidDepth;
    invalidDepth.depth = -0.5f;  // Negative
    REGRESS_ASSERT(!invalidDepth.isValid(), "PresetState with negative depth should be invalid");
}

static void testCanonicalPresetStateRoundTrip()
{
    PresetState original;
    original.rate = 1.5f;
    original.depth = 0.3f;
    original.offset = 45.0f;
    original.width = 1.2f;
    original.color = 0.7f;
    original.mix = 0.8f;
    original.hqEnabled = true;
    original.modularCoresEnabled = true;
    original.coreAssignments.set(0, false, choroboros::CoreId::tape);
    original.coreAssignments.set(1, true, choroboros::CoreId::bbd);

    // Serialize to JSON
    const auto json = original.serializeToJson();
    REGRESS_ASSERT(!json.empty(), "JSON serialization should not be empty");

    // Deserialize from JSON
    const auto restored = PresetState::deserializeFromJson(json);
    REGRESS_ASSERT(restored.has_value(), "JSON deserialization should succeed");

    if (restored.has_value())
    {
        const auto& r = restored.value();
        REGRESS_ASSERT(r.rate == original.rate, "Rate should round-trip through JSON");
        REGRESS_ASSERT(r.depth == original.depth, "Depth should round-trip through JSON");
        REGRESS_ASSERT(r.hqEnabled == original.hqEnabled, "HQ should round-trip through JSON");
        REGRESS_ASSERT(r.modularCoresEnabled == original.modularCoresEnabled, "Modular cores should round-trip");
        REGRESS_ASSERT(assignmentTablesEqual(r.coreAssignments, original.coreAssignments),
                       "Core assignments should round-trip through JSON");
    }
}

static void testCanonicalPresetStateBinaryRoundTrip()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    // Capture state through canonical path
    const auto original = proc.capturePresetState();

    // Serialize to binary
    const auto binary = original.serializeToBinary();
    REGRESS_ASSERT(binary.getSize() > 0, "Binary serialization should not be empty");

    // Deserialize from binary
    const auto restored = PresetState::deserializeFromBinary(binary.getData(),
                                                             static_cast<int>(binary.getSize()));
    REGRESS_ASSERT(restored.has_value(), "Binary deserialization should succeed");

    if (restored.has_value())
    {
        const auto& r = restored.value();
        REGRESS_ASSERT(r.rate == original.rate, "Rate should round-trip through binary");
        REGRESS_ASSERT(r.hqEnabled == original.hqEnabled, "HQ should round-trip through binary");
    }
}

static void testFactoryPresetStates()
{
    // Test that all factory presets can be created and are valid
    for (int i = 0; i < 7; ++i)
    {
        const auto preset = PresetState::makeFactoryPreset(i);
        REGRESS_ASSERT(preset.has_value(), juce::String("Factory preset ") + juce::String(i) + " should be creatable");

        if (preset.has_value())
        {
            REGRESS_ASSERT(preset.value().isValid(),
                          juce::String("Factory preset ") + juce::String(i) + " should be valid");
        }
    }
}

static void testProcessorCanonicalStateApply()
{
    ChoroborosAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    // Create a preset state
    PresetState state;
    state.rate = 2.5f;
    state.depth = 0.6f;
    state.hqEnabled = true;
    state.modularCoresEnabled = true;
    state.coreAssignments.set(0, false, choroboros::CoreId::tape);

    // Apply through canonical path
    const bool applied = proc.applyPresetState(state, ApplyContext::UserPresetLoad);
    REGRESS_ASSERT(applied, "applyPresetState should succeed with valid state");

    // Verify the state was applied
    const auto captured = proc.capturePresetState();
    REGRESS_ASSERT(nearlyEqual(captured.rate, state.rate), "Rate should be applied");
    REGRESS_ASSERT(captured.hqEnabled == state.hqEnabled, "HQ should be applied");
    REGRESS_ASSERT(captured.modularCoresEnabled == state.modularCoresEnabled, "Modular cores should be applied");
}

/** Parse tier specification from command line.
 *
 * Tier 1: Headless service tests (no UI construction).
 *   - Canonical preset state validation and round-trip
 *   - DSP regression tests (no audio load)
 *   - Headless console command behavior tests
 *
 * Tier 2: GUI smoke tests (editor/DevPanel construction, no interaction).
 *   - Editor and DevPanel construction
 *   - Tab and component presence verification
 *   - Layout resizing
 *
 * Tier 3: Integration tests (full UI interaction under load).
 *   - Console command latency benchmarks under audio load
 *   - Preset round-trips with file I/O
 *   - Full state save/restore cycles
 *
 * Usage:
 *   ./ChoroborosRegressionTests --tier=1          (Tier 1 only, fast)
 *   ./ChoroborosRegressionTests --tier=1,2        (Tiers 1+2, smoke test)
 *   ./ChoroborosRegressionTests --tier=1,2,3      (All tiers, full suite)
 *   ./ChoroborosRegressionTests (default: --tier=1,2,3 for backward compat)
 */
struct TierConfig
{
    bool tier1 = false;  // Headless / service tests
    bool tier2 = false;  // GUI smoke tests
    bool tier3 = false;  // Integration / interaction tests

    static TierConfig parseFromArgs(int argc, char** argv)
    {
        TierConfig cfg;

        // Default: all tiers (backward compat with old --gui behavior)
        cfg.tier1 = true;
        cfg.tier2 = true;
        cfg.tier3 = true;

        for (int i = 1; i < argc; ++i)
        {
            const juce::String arg(argv[i] != nullptr ? argv[i] : "");

            if (arg == "--help" || arg == "-h")
            {
                std::cout << "Choroboros Regression Test Tiers\n"
                          << "================================\n"
                          << "\n"
                          << "Usage: ChoroborosRegressionTests [--tier=SPEC]\n"
                          << "\n"
                          << "Tier Specification:\n"
                          << "  --tier=1          Run Tier 1 only (headless, fast)\n"
                          << "  --tier=1,2        Run Tiers 1-2 (with GUI smoke tests)\n"
                          << "  --tier=1,2,3      Run all tiers (full suite, slow)\n"
                          << "  (no argument)     Default: all tiers\n"
                          << "\n"
                          << "Backward Compatibility:\n"
                          << "  --dsp-only        Equivalent to --tier=1\n"
                          << "  --gui             Equivalent to --tier=1,2,3\n"
                          << "\n"
                          << "Tier Descriptions:\n"
                          << "  Tier 1 (Headless/Service): No UI construction. Tests DSP, presets, service logic.\n"
                          << "  Tier 2 (GUI Smoke):        Editor/DevPanel construction, basic presence checks.\n"
                          << "  Tier 3 (Integration):      Full UI interaction, console latency under audio load.\n";
                exit(0);
            }

            // Backward compatibility: --dsp-only => tier 1 only
            if (arg == "--dsp-only")
            {
                cfg.tier1 = true;
                cfg.tier2 = false;
                cfg.tier3 = false;
            }
            // Backward compatibility: --gui => all tiers
            else if (arg == "--gui")
            {
                cfg.tier1 = true;
                cfg.tier2 = true;
                cfg.tier3 = true;
            }
            // New syntax: --tier=SPEC
            else if (arg.startsWith("--tier="))
            {
                cfg.tier1 = false;
                cfg.tier2 = false;
                cfg.tier3 = false;

                const juce::String spec = arg.substring(7);  // "1,2,3"
                juce::StringArray parts;
                parts.addTokens(spec, ",", "");
                for (const auto& part : parts)
                {
                    const int tier = part.trim().getIntValue();
                    if (tier == 1) cfg.tier1 = true;
                    else if (tier == 2) cfg.tier2 = true;
                    else if (tier == 3) cfg.tier3 = true;
                }
            }
        }

        return cfg;
    }

    juce::String describe() const
    {
        juce::StringArray parts;
        if (tier1) parts.add("1");
        if (tier2) parts.add("2");
        if (tier3) parts.add("3");
        if (parts.isEmpty())
            return "(none)";
        return parts.joinIntoString(",");
    }
};

int main(int argc, char** argv)
{
    const TierConfig tiers = TierConfig::parseFromArgs(argc, argv);

    // GUI initialization (needed for Tiers 2-3, but safe to call anyway)
    juce::ScopedJuceInitialiser_GUI init;

    std::cout << "Choroboros Regression Harness\n";
    std::cout << "=============================\n";
    std::cout << "Running Tiers: " << tiers.describe() << "\n";
    std::cout << "\n";

    if (tiers.tier1)
    {
        std::cout << "Tier 1: Headless / Service Tests\n";
        std::cout << "--------------------------------\n";

        // Canonical preset state tests
        testCanonicalPresetStateValidation();
        testCanonicalPresetStateRoundTrip();
        testCanonicalPresetStateBinaryRoundTrip();
        testFactoryPresetStates();
        testProcessorCanonicalStateApply();

        // DSP regression tests (no UI, no audio load complexity)
        testProcessBlockSizes();
        testEngineHQTorture();
        testStateRoundTrip();
        testMaxBlockChannels();
        testKnownBadBundledEngineProfilesMigrateToDefaults();

        // TODO: Add Tier 1 headless console command behavior tests here
        // when ConsoleEngine API stabilizes.

        std::cout << "Tier 1 complete.\n";
        std::cout << "\n";
    }

    if (tiers.tier2)
    {
        std::cout << "Tier 2: GUI Smoke Tests\n";
        std::cout << "----------------------\n";

        // TODO: Add editor/DevPanel construction and presence verification tests
        // Lightweight checks: can we construct without crashing?
        // Example:
        //   ChoroborosAudioProcessor proc;
        //   ChoroborosPluginEditor editor(proc);
        //   editor.setBounds(0, 0, 1024, 600);
        //   editor.resized();
        //   DevPanel panel(editor, proc);
        //   panel.setBounds(0, 0, 1280, 760);
        //   panel.resized();
        //   // Verify components exist

        std::cout << "Tier 2 smoke tests not yet implemented.\n";
        std::cout << "Tier 2 complete.\n";
        std::cout << "\n";
    }

    if (tiers.tier3)
    {
        std::cout << "Tier 3: Integration Tests\n";
        std::cout << "------------------------\n";

        // Console command latency benchmark (heavier UI interaction test)
        testConsoleCommandLatencyUnderAudioLoad();

        // TODO: Add other integration tests (file I/O, full state cycles, etc.)

        std::cout << "Tier 3 complete.\n";
        std::cout << "\n";
    }

    std::cout << "=============================\n";
    if (g_failCount == 0)
        std::cout << "PASS: All regression tests passed.\n";
    else
        std::cerr << "FAIL: " << g_failCount << " assertion(s) failed.\n";

    return g_failCount > 0 ? 1 : 0;
}
