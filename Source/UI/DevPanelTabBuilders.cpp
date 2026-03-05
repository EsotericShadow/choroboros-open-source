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

#include "DevPanel.h"
#include "DevPanelBuildContext.h"
#include "../Plugin/PluginEditor.h"
#include "../Plugin/PluginProcessor.h"
#include "DevPanelSupport.h"

using namespace devpanel;

void DevPanel::buildOverviewTab(DevPanelBuildContext& ctx)
{
    const auto& makeReadOnly = ctx.makeReadOnly;
    const auto& makeSignalFlow = ctx.makeSignalFlow;
    const auto& makeSparkline = ctx.makeSparkline;
    const auto& readRawParam = ctx.readRawParam;
    const auto& readAnalyzerSnapshot = ctx.readAnalyzerSnapshot;
    juce::Array<juce::PropertyComponent*> overviewActiveParams;
    overviewActiveParams.add(makeReadOnly("Engine + Mode", [this]() -> juce::String
    {
        static const juce::String names[] { "Green", "Blue", "Red", "Purple", "Black" };
        const int idx = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        return names[idx] + (processor.isHqEnabled() ? " HQ" : " NQ");
    }));
    overviewActiveParams.add(makeReadOnly("Rate (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::RATE_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped, 3) + " Hz";
    }));
    overviewActiveParams.add(makeReadOnly("Depth (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::DEPTH_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped * 100.0f, 1) + " %";
    }));
    overviewActiveParams.add(makeReadOnly("Offset (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::OFFSET_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::OFFSET_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped, 2) + " deg";
    }));
    overviewActiveParams.add(makeReadOnly("Width (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::WIDTH_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::WIDTH_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped, 3) + " x";
    }));
    overviewActiveParams.add(makeReadOnly("Color (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::COLOR_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped, 3);
    }));
    overviewActiveParams.add(makeReadOnly("Mix (raw -> mapped)", [this, readRawParam]() -> juce::String
    {
        const float raw = readRawParam(ChoroborosAudioProcessor::MIX_ID);
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::MIX_ID, raw);
        return juce::String(raw, 3) + " -> " + juce::String(mapped * 100.0f, 1) + " %";
    }));
    setSectionRowHeight(overviewActiveParams, kRowHeightStandard);
    addPanelSection(overviewPanel, "Active Parameters (start here)", overviewActiveParams, true);

    juce::Array<juce::PropertyComponent*> overviewDerivedState;
    overviewDerivedState.add(makeReadOnly("HPF Effective", [this]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        return juce::String(rt.hpfCutoffHz.load(), 1) + " Hz (Q " + juce::String(rt.hpfQ.load(), 3) + ")";
    }));
    overviewDerivedState.add(makeReadOnly("LPF Effective", [this]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        return juce::String(rt.lpfCutoffHz.load(), 1) + " Hz (Q " + juce::String(rt.lpfQ.load(), 3) + ")";
    }));
    overviewDerivedState.add(makeReadOnly("Centre Delay (base + scale*depth)", [this, readRawParam]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        const float depthMapped = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID,
                                                              readRawParam(ChoroborosAudioProcessor::DEPTH_ID));
        const float centreMs = rt.centreDelayBaseMs.load() + rt.centreDelayScale.load() * depthMapped;
        return juce::String(centreMs, 3) + " ms";
    }));
    overviewDerivedState.add(makeReadOnly("Red NQ BBD Min Delay @ Max Clock", [this]() -> juce::String
    {
        const bool isRedNQ = processor.getCurrentEngineColorIndex() == 2 && !processor.isHqEnabled();
        if (!isRedNQ)
            return "n/a (active only in Red NQ)";

        const auto& rt = processor.getDspInternals();
        const double fs = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
        const float stages = juce::jmax(1.0f, rt.bbdStages.load());
        const float maxClockRatio = juce::jlimit(0.01f, 1.0f, rt.bbdClockMaxRatio.load());
        const float maxClockHz = static_cast<float>(fs * maxClockRatio);
        const float minDelayMs = (stages / juce::jmax(1.0f, maxClockHz)) * 1000.0f;
        return juce::String(minDelayMs, 3) + " ms @ " + juce::String(maxClockHz, 1) + " Hz";
    }));
    overviewDerivedState.add(makeReadOnly("Red Saturation Drive Target", [this, readRawParam]() -> juce::String
    {
        if (processor.getCurrentEngineColorIndex() != 2)
            return "n/a (Red only)";

        const auto& rt = processor.getDspInternals();
        const float colorMapped = juce::jlimit(0.0f, 1.0f,
                                               processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                           readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        const float drive = 1.0f + rt.saturationDriveScale.load() * colorMapped;
        if (colorMapped <= 0.0f)
            return "x1.000 (exact bypass)";
        return "x" + juce::String(drive, 3);
    }));
    auto* overviewSignalFlowCard = makeSignalFlow("Signal Chain", [this, readRawParam, readAnalyzerSnapshot]() -> SignalFlowPropertyComponent::State
    {
        SignalFlowPropertyComponent::State state;
        static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };

        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();
        const bool isRedNQ = engine == 2 && !hq;
        const bool isRedHQ = engine == 2 && hq;
        const auto& rt = processor.getDspInternals();

        const float rateMapped = processor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID,
                                                             readRawParam(ChoroborosAudioProcessor::RATE_ID));
        const float depthMapped = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID,
                                                              readRawParam(ChoroborosAudioProcessor::DEPTH_ID));
        const float colorMapped = juce::jlimit(0.0f, 1.0f,
                                               processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                           readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        const float mixMapped = juce::jlimit(0.0f, 1.0f,
                                             processor.mapParameterValue(ChoroborosAudioProcessor::MIX_ID,
                                                                         readRawParam(ChoroborosAudioProcessor::MIX_ID)));

        state.modeLabel = engineNames[engine] + (hq ? " HQ" : " NQ");
        const auto analyzer = readAnalyzerSnapshot();
        const auto toNorm = [](float db)
        {
            return juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 72.0f);
        };
        if (analyzer.valid)
        {
            state.preLevel = toNorm(analyzer.inputPeakDb);
            state.wetLevel = toNorm(analyzer.wetPeakDb);
            state.postLevel = toNorm(analyzer.outputPeakDb);
            state.live = true;
            state.clipped = analyzer.outputPeakDb > -0.5f;
        }
        else
        {
            state.preLevel = juce::jlimit(0.0f, 1.0f, 0.25f + depthMapped * 0.65f);
            state.wetLevel = mixMapped;
            state.postLevel = juce::jlimit(0.0f, 1.0f, 0.20f + mixMapped * 0.60f + colorMapped * 0.20f);
            state.live = false;
            state.clipped = false;
        }

        state.stages[0] = { "Input", juce::String(rateMapped, 2) + " Hz", true };
        state.stages[1] = {
            "PreEmph",
            isRedNQ ? "Bypassed"
                    : (juce::String(rt.preEmphasisFreqHz.load(), 0) + " Hz"),
            !isRedNQ
        };
        state.stages[2] = {
            "HP/LP",
            juce::String(rt.hpfCutoffHz.load(), 0) + " / " + juce::String(rt.lpfCutoffHz.load(), 0),
            true
        };

        juce::String coreName = "Chorus Core";
        if (engine == 0) coreName = "Bloom Core";
        if (engine == 1) coreName = "Focus Core";
        if (engine == 2) coreName = hq ? "Tape Core" : "BBD Core";
        if (engine == 3) coreName = hq ? "Orbit Core" : "Warp Core";
        if (engine == 4) coreName = hq ? "Ensemble Core" : "Intensity Core";
        state.stages[3] = { "Core", coreName, true };

        state.stages[4] = {
            "Color",
            juce::String(colorMapped, 3),
            true
        };

        const float redDrive = 1.0f + rt.saturationDriveScale.load() * colorMapped;
        const float tapeDrive = 1.0f + rt.tapeDriveScale.load() * colorMapped;
        state.stages[5] = {
            "Saturate",
            isRedNQ ? ("x" + juce::String(redDrive, 2))
                    : (isRedHQ ? ("Tape x" + juce::String(tapeDrive, 2)) : "Inactive"),
            isRedNQ || isRedHQ
        };
        state.stages[6] = {
            "Output",
            juce::String(mixMapped * 100.0f, 1) + "% wet",
            true
        };

        return state;
    }, LinkGroup::overview);
    overviewSignalFlowCard->setPreferredHeight(272);
    overviewVisualDeck.addAndMakeVisible(overviewSignalFlowCard);
    overviewVisualDeckCards.add(overviewSignalFlowCard);

    auto* overviewDelayTrajectoryCard = makeSparkline("Delay Trajectory (ms)", [readAnalyzerSnapshot]() -> std::vector<float>
    {
        std::vector<float> values;
        constexpr int pointCount = ChoroborosAudioProcessor::ANALYZER_WAVEFORM_POINTS;
        values.reserve(pointCount);
        const auto snapshot = readAnalyzerSnapshot();
        for (int i = 0; i < pointCount; ++i)
            values.push_back(snapshot.delayTrajectoryMs[static_cast<size_t>(i)]);

        return values;
    }, LinkGroup::overview);
    overviewDelayTrajectoryCard->setPreferredHeight(172);
    overviewVisualDeck.addAndMakeVisible(overviewDelayTrajectoryCard);
    overviewVisualDeckCards.add(overviewDelayTrajectoryCard);

    setSectionRowHeight(overviewDerivedState, kRowHeightCompact);
    addPanelSection(overviewPanel, "Derived State", overviewDerivedState, false);
}

void DevPanel::buildModulationTab(DevPanelBuildContext& ctx)
{
    const auto& makeLiveMappedControl = ctx.makeLiveMappedControl;
    const auto& makeReadOnly = ctx.makeReadOnly;
    const auto& makeSparkline = ctx.makeSparkline;
    const auto& readRawParam = ctx.readRawParam;
    const auto& readAnalyzerSnapshot = ctx.readAnalyzerSnapshot;
    juce::Array<juce::PropertyComponent*> modulationControls;
    modulationControls.add(makeLiveMappedControl("LFO Rate (Hz)",
                                                 ChoroborosAudioProcessor::RATE_ID,
                                                 0.005f, 20.0f, 0.001f, 0.65f, 1.0f));
    modulationControls.add(makeLiveMappedControl("LFO Depth (%)",
                                                 ChoroborosAudioProcessor::DEPTH_ID,
                                                 0.0f, 200.0f, 0.1f, 1.0f, 100.0f));
    modulationControls.add(makeLiveMappedControl("Stereo Offset (deg)",
                                                 ChoroborosAudioProcessor::OFFSET_ID,
                                                 0.0f, 360.0f, 0.5f, 1.0f, 1.0f));
    modulationControls.add(makeLiveMappedControl("Stereo Width (x)",
                                                 ChoroborosAudioProcessor::WIDTH_ID,
                                                 0.0f, 4.0f, 0.01f, 1.0f, 1.0f));
    setSectionRowHeight(modulationControls, kRowHeightControl);
    addPanelSection(modulationPanel, "LFO Controls (edit here -> cards above)", modulationControls, true);

    juce::Array<juce::PropertyComponent*> modulationReadouts;
    modulationReadouts.add(makeReadOnly("LFO Rate", [this, readRawParam]() -> juce::String
    {
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::RATE_ID,
                                                         readRawParam(ChoroborosAudioProcessor::RATE_ID));
        return juce::String(mapped, 3) + " Hz";
    }));
    modulationReadouts.add(makeReadOnly("Stereo Offset", [this, readRawParam]() -> juce::String
    {
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::OFFSET_ID,
                                                         readRawParam(ChoroborosAudioProcessor::OFFSET_ID));
        return juce::String(mapped, 2) + " deg";
    }));
    modulationReadouts.add(makeReadOnly("Depth", [this, readRawParam]() -> juce::String
    {
        const float mapped = processor.mapParameterValue(ChoroborosAudioProcessor::DEPTH_ID,
                                                         readRawParam(ChoroborosAudioProcessor::DEPTH_ID));
        return juce::String(mapped * 100.0f, 1) + " %";
    }));
    modulationReadouts.add(makeReadOnly("Stereo Correlation (est.)", [this, readRawParam]() -> juce::String
    {
        const float offsetDeg = processor.mapParameterValue(ChoroborosAudioProcessor::OFFSET_ID,
                                                            readRawParam(ChoroborosAudioProcessor::OFFSET_ID));
        const float width = processor.mapParameterValue(ChoroborosAudioProcessor::WIDTH_ID,
                                                        readRawParam(ChoroborosAudioProcessor::WIDTH_ID));
        const float correlation = juce::jlimit(-1.0f, 1.0f,
                                               std::cos(juce::degreesToRadians(offsetDeg))
                                               * juce::jlimit(0.0f, 2.0f, width));
        return juce::String(correlation, 3);
    }));
    setSectionRowHeight(modulationReadouts, kRowHeightCompact);
    addPanelSection(modulationPanel, "LFO Readouts (passive monitor)", modulationReadouts, false);

    juce::Array<juce::PropertyComponent*> modulationVisuals;
    auto* lfoLeftCard = makeSparkline("LFO Left", [readAnalyzerSnapshot]() -> std::vector<float>
    {
        std::vector<float> values;
        constexpr int pointCount = ChoroborosAudioProcessor::ANALYZER_WAVEFORM_POINTS;
        values.reserve(pointCount);
        const auto snapshot = readAnalyzerSnapshot();
        for (int i = 0; i < pointCount; ++i)
            values.push_back(snapshot.lfoLeft[static_cast<size_t>(i)]);
        return values;
    }, LinkGroup::modulation);
    lfoLeftCard->setPreferredHeight(146);
    modulationVisuals.add(lfoLeftCard);

    auto* lfoRightCard = makeSparkline("LFO Right", [readAnalyzerSnapshot]() -> std::vector<float>
    {
        std::vector<float> values;
        constexpr int pointCount = ChoroborosAudioProcessor::ANALYZER_WAVEFORM_POINTS;
        values.reserve(pointCount);
        const auto snapshot = readAnalyzerSnapshot();
        for (int i = 0; i < pointCount; ++i)
            values.push_back(snapshot.lfoRight[static_cast<size_t>(i)]);
        return values;
    }, LinkGroup::modulation);
    lfoRightCard->setPreferredHeight(146);
    modulationVisuals.add(lfoRightCard);

    auto* modulationDelayTrajectoryCard = makeSparkline("Delay Trajectory", [readAnalyzerSnapshot]() -> std::vector<float>
    {
        std::vector<float> values;
        constexpr int pointCount = ChoroborosAudioProcessor::ANALYZER_WAVEFORM_POINTS;
        values.reserve(pointCount);
        const auto snapshot = readAnalyzerSnapshot();
        for (int i = 0; i < pointCount; ++i)
            values.push_back(snapshot.delayTrajectoryMs[static_cast<size_t>(i)]);
        return values;
    }, LinkGroup::modulation);
    modulationDelayTrajectoryCard->setPreferredHeight(160);
    modulationVisuals.add(modulationDelayTrajectoryCard);
    for (auto* visual : modulationVisuals)
    {
        if (visual != nullptr)
        {
            modulationVisualDeck.addAndMakeVisible(visual);
            modulationVisualDeckCards.add(visual);
        }
    }
}

void DevPanel::buildToneTab(DevPanelBuildContext& ctx)
{
    const auto& makeReadOnly = ctx.makeReadOnly;
    const auto& makeSpectrumOverlay = ctx.makeSpectrumOverlay;
    const auto& makeTransferCurve = ctx.makeTransferCurve;
    const auto& makeLockable = ctx.makeLockable;
    const auto& readRawParam = ctx.readRawParam;
    const auto& readAnalyzerSnapshot = ctx.readAnalyzerSnapshot;
    const auto& registerControlMetadata = ctx.registerControlMetadata;
    juce::Array<juce::PropertyComponent*> toneReadouts;
    toneReadouts.add(makeReadOnly("HPF / LPF", [this]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        return juce::String(rt.hpfCutoffHz.load(), 1) + " Hz / "
               + juce::String(rt.lpfCutoffHz.load(), 1) + " Hz";
    }));
    toneReadouts.add(makeReadOnly("Pre-Emphasis Gain", [this]() -> juce::String
    {
        return juce::String(processor.getDspInternals().preEmphasisGain.load(), 3) + " x";
    }));
    toneReadouts.add(makeReadOnly("Compressor", [this]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        return "T " + juce::String(rt.compressorThresholdDb.load(), 1) + " dB, R "
               + juce::String(rt.compressorRatio.load(), 2);
    }));
    toneReadouts.add(makeReadOnly("Tonal Tilt (est.)", [this]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        const float tilt = juce::Decibels::gainToDecibels(juce::jmax(0.01f, rt.preEmphasisGain.load()));
        return juce::String(tilt, 2) + " dB";
    }));
    setSectionRowHeight(toneReadouts, kRowHeightCompact);

    juce::Array<juce::PropertyComponent*> toneVisuals;
    auto* spectrumOverlayCard = makeSpectrumOverlay("Spectrum + HP/LP Overlay", [this, readAnalyzerSnapshot]() -> SpectrumOverlayPropertyComponent::State
    {
        const auto& rt = processor.getDspInternals();
        const auto snapshot = readAnalyzerSnapshot();
        SpectrumOverlayPropertyComponent::State s;
        s.sampleRate = snapshot.sampleRate > 100.0 ? snapshot.sampleRate
                                                    : (processor.getSampleRate() > 100.0 ? processor.getSampleRate() : 48000.0);
        s.hpfHz = rt.hpfCutoffHz.load();
        s.lpfHz = rt.lpfCutoffHz.load();
        s.preEmphasisGain = rt.preEmphasisGain.load();
        s.hasMeasuredSpectrum = snapshot.valid;
        s.frozen = processor.isAnalyzerFrozen();
        s.outputPeakDb = snapshot.outputPeakDb;
        s.clipped = snapshot.valid && snapshot.outputPeakDb > -0.5f;
        s.spectrumNorm = snapshot.outputSpectrum;
        return s;
    }, LinkGroup::tone);
    spectrumOverlayCard->setPreferredHeight(256);
    toneVisuals.add(spectrumOverlayCard);

    auto* transferCurveCard = makeTransferCurve("Saturation Transfer Curve", [this, readRawParam, readAnalyzerSnapshot]() -> TransferCurvePropertyComponent::State
    {
        TransferCurvePropertyComponent::State state;
        const auto snapshot = readAnalyzerSnapshot();
        state.measured = snapshot.valid;
        state.frozen = processor.isAnalyzerFrozen();
        state.outputPeakDb = snapshot.outputPeakDb;
        state.clipped = snapshot.valid && snapshot.outputPeakDb > -0.5f;
        state.input = snapshot.transferInput;
        state.output = snapshot.transferOutput;

        if (processor.getCurrentEngineColorIndex() != 2)
            return state;
        const auto& rt = processor.getDspInternals();
        const float colorMapped = juce::jlimit(0.0f, 1.0f,
                                               processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                           readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        state.drive = 1.0f + rt.saturationDriveScale.load() * colorMapped;
        return state;
    }, LinkGroup::tone);
    transferCurveCard->setPreferredHeight(228);
    toneVisuals.add(transferCurveCard);
    for (auto* visual : toneVisuals)
    {
        if (visual != nullptr)
        {
            toneVisualDeck.addAndMakeVisible(visual);
            toneVisualDeckCards.add(visual);
        }
    }

    juce::Array<juce::PropertyComponent*> toneAnalyzerControls;
    auto* freezeAnalyzerProp = new juce::BooleanPropertyComponent(
        makeBoolValue([this] { return processor.isAnalyzerFrozen(); },
                      [this](bool v) { processor.setAnalyzerFrozen(v); }),
        "Freeze Analyzer", "Freeze");
    freezeAnalyzerProp->setTooltip("Freeze analyzer snapshots at the current frame.");
    liveReadoutProperties.add(freezeAnalyzerProp);
    toneAnalyzerControls.add(freezeAnalyzerProp);
    registerControlMetadata("Freeze Analyzer", "analyzer_controls", "runtime_toggle", "freeze_state", {});

    auto* holdAnalyzerProp = new juce::BooleanPropertyComponent(
        makeBoolValue([this] { return processor.isAnalyzerPeakHoldEnabled(); },
                      [this](bool v) { processor.setAnalyzerPeakHoldEnabled(v); }),
        "Spectrum Peak Hold", "Hold");
    holdAnalyzerProp->setTooltip("Keep transient peaks visible with gradual decay.");
    liveReadoutProperties.add(holdAnalyzerProp);
    toneAnalyzerControls.add(holdAnalyzerProp);
    registerControlMetadata("Spectrum Peak Hold", "analyzer_controls", "runtime_toggle", "hold_state", {});

    toneAnalyzerControls.add(makeLockable(
        makeFloatValue([this] { return static_cast<float>(processor.getAnalyzerRefreshHz()); },
                       [this](float v) { processor.setAnalyzerRefreshHz(static_cast<int>(std::round(v))); }),
        "Analyzer Refresh (Hz)", 5.0, 60.0, 1.0, 1.0));
    setSectionRowHeight(toneAnalyzerControls, kRowHeightCompact);
    addPanelSection(tonePanel, "Analyzer Controls (for cards above)", toneAnalyzerControls, false);
    addPanelSection(tonePanel, "Tone Readouts (passive monitor)", toneReadouts, true);
}

void DevPanel::buildEngineTab(DevPanelBuildContext& ctx)
{
    const auto& makeReadOnly = ctx.makeReadOnly;
    const auto& makeSignalFlow = ctx.makeSignalFlow;
    const auto& readRawParam = ctx.readRawParam;
    const auto& readAnalyzerSnapshot = ctx.readAnalyzerSnapshot;
    juce::Array<juce::PropertyComponent*> engineReadouts;
    engineReadouts.add(makeReadOnly("Engine Character", [this]() -> juce::String
    {
        static const juce::String names[] { "Green Bloom", "Blue Focus", "Red Vintage", "Purple Warp/Orbit", "Black Intensity/Ensemble" };
        const int idx = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        return names[idx] + (processor.isHqEnabled() ? " (HQ)" : " (NQ)");
    }));
    engineReadouts.add(makeReadOnly("Color Semantics", [this]() -> juce::String
    {
        switch (processor.getCurrentEngineColorIndex())
        {
            case 0: return "Bloom amount + tonal bloom.";
            case 1: return "Focus amount + presence shaping.";
            case 2: return processor.isHqEnabled() ? "Tape tone + drive." : "BBD/tape coloration + saturation.";
            case 3: return processor.isHqEnabled() ? "Orbit eccentricity + rate." : "Warp intensity.";
            case 4: return processor.isHqEnabled() ? "Ensemble tap behavior." : "Intensity depth/glide.";
            default: return "Unknown.";
        }
    }));
    engineReadouts.add(makeReadOnly("Applicability", [this]() -> juce::String
    {
        const bool isRedNQ = processor.getCurrentEngineColorIndex() == 2 && !processor.isHqEnabled();
        const bool isRedHQ = processor.getCurrentEngineColorIndex() == 2 && processor.isHqEnabled();
        if (isRedNQ)
            return "BBD active, Tape inactive.";
        if (isRedHQ)
            return "Tape active, BBD inactive.";
        return "Engine-specific non-Red core active.";
    }));
    setSectionRowHeight(engineReadouts, kRowHeightCompact);
    addPanelSection(enginePanel, "Active Engine Context (editing target)", engineReadouts, true);

    juce::Array<juce::PropertyComponent*> engineMacroDerived;
    engineMacroDerived.add(makeReadOnly("Macro A", [this, readRawParam]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();
        const float color = juce::jlimit(0.0f, 1.0f,
                                         processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                     readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        if (engine == 0)
        {
            const float norm = std::pow(color, juce::jmax(0.1f, rt.greenBloomExponent.load()));
            const float cutoff = juce::jmap(norm, 0.0f, 1.0f, rt.greenBloomCutoffMaxHz.load(), rt.greenBloomCutoffMinHz.load());
            return "Bloom LPF " + juce::String(cutoff, 1) + " Hz";
        }
        if (engine == 1)
        {
            const float norm = std::pow(color, juce::jmax(0.1f, rt.blueFocusExponent.load()));
            const float hp = juce::jmap(norm, 0.0f, 1.0f, rt.blueFocusHpMinHz.load(), rt.blueFocusHpMaxHz.load());
            return "Focus HP " + juce::String(hp, 1) + " Hz";
        }
        if (engine == 2 && !hq)
        {
            const float drive = 1.0f + rt.saturationDriveScale.load() * color;
            return "Red Drive x" + juce::String(drive, 3);
        }
        if (engine == 2 && hq)
        {
            const float tone = juce::jmap(color, 0.0f, 1.0f, rt.tapeToneMaxHz.load(), rt.tapeToneMinHz.load());
            return "Tape Tone " + juce::String(tone, 1) + " Hz";
        }
        if (engine == 3 && !hq)
        {
            const float k = rt.purpleWarpKBase.load() + rt.purpleWarpKScale.load() * color;
            return "Warp K " + juce::String(k, 3);
        }
        if (engine == 3 && hq)
        {
            const float thetaRate = rt.purpleOrbitThetaRateBaseHz.load() + rt.purpleOrbitThetaRateScaleHz.load() * color;
            return "Orbit Theta " + juce::String(thetaRate, 3) + " Hz";
        }
        if (engine == 4 && !hq)
        {
            const float intensity = rt.blackNqDepthBase.load() + rt.blackNqDepthScale.load() * color;
            return "Intensity " + juce::String(intensity, 3);
        }
        const float tapMix = rt.blackHqTap2MixBase.load() + rt.blackHqTap2MixScale.load() * color;
        return "Tap2 Mix " + juce::String(tapMix, 3);
    }));
    engineMacroDerived.add(makeReadOnly("Macro B", [this, readRawParam]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();
        const float color = juce::jlimit(0.0f, 1.0f,
                                         processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                     readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        if (engine == 0)
        {
            const float wet = rt.greenBloomWetBlend.load() * color;
            return "Bloom Wet " + juce::String(wet * 100.0f, 1) + "%";
        }
        if (engine == 1)
        {
            const float norm = std::pow(color, juce::jmax(0.1f, rt.blueFocusExponent.load()));
            const float lp = juce::jmap(norm, 0.0f, 1.0f, rt.blueFocusLpMaxHz.load(), rt.blueFocusLpMinHz.load());
            return "Focus LP " + juce::String(lp, 1) + " Hz";
        }
        if (engine == 2 && !hq)
        {
            const float bbdDepth = rt.bbdDepthMs.load() * color;
            return "BBD Mod " + juce::String(bbdDepth, 3) + " ms";
        }
        if (engine == 2 && hq)
        {
            const float drive = 1.0f + rt.tapeDriveScale.load() * color;
            return "Tape Drive x" + juce::String(drive, 3);
        }
        if (engine == 3 && !hq)
        {
            return "Warp A/B " + juce::String(rt.purpleWarpA.load(), 3) + " / " + juce::String(rt.purpleWarpB.load(), 3);
        }
        if (engine == 3 && hq)
        {
            const float ecc = rt.purpleOrbitEccentricity.load()
                              * (1.0f + color * (rt.purpleOrbitEccentricity2Ratio.load() - 1.0f));
            return "Orbit Ecc " + juce::String(ecc, 3);
        }
        if (engine == 4 && !hq)
            return "Glide " + juce::String(rt.blackNqDelayGlideMs.load(), 3) + " ms";

        const float secondTapDepth = rt.blackHqSecondTapDepthBase.load()
                                     + rt.blackHqSecondTapDepthScale.load() * color;
        return "Tap2 Depth " + juce::String(secondTapDepth, 3);
    }));
    engineMacroDerived.add(makeReadOnly("Macro C", [this, readRawParam]() -> juce::String
    {
        const auto& rt = processor.getDspInternals();
        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();
        const float color = juce::jlimit(0.0f, 1.0f,
                                         processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                     readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        if (engine == 2 && !hq)
        {
            const float fs = static_cast<float>(processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0);
            const float clockMin = rt.bbdClockMinHz.load();
            const float clockMax = juce::jmax(clockMin, fs * juce::jlimit(0.01f, 1.0f, rt.bbdClockMaxRatio.load()));
            const float clock = juce::jmap(color, 0.0f, 1.0f, clockMin, clockMax);
            return "BBD Clock " + juce::String(clock, 1) + " Hz";
        }
        if (engine == 2 && hq)
        {
            const float wow = rt.tapeWowDepthBase.load() + rt.tapeWowDepthSpread.load() * color;
            return "Wow Depth " + juce::String(wow, 5);
        }
        if (engine == 4 && hq)
        {
            const float offset = rt.blackHqSecondTapDelayOffsetBase.load()
                                 + rt.blackHqSecondTapDelayOffsetScale.load() * color;
            return "Tap2 Offset " + juce::String(offset, 3) + " ms";
        }
        return "n/a";
    }));

    juce::Array<juce::PropertyComponent*> engineVisuals;
    auto* engineSignalFlowCard = makeSignalFlow("Engine Signal Flow", [this, readRawParam, readAnalyzerSnapshot]() -> SignalFlowPropertyComponent::State
    {
        SignalFlowPropertyComponent::State state;
        static const juce::String names[] { "Green", "Blue", "Red", "Purple", "Black" };
        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();
        const auto& rt = processor.getDspInternals();
        const auto analyzer = readAnalyzerSnapshot();
        const float colorMapped = juce::jlimit(0.0f, 1.0f,
                                               processor.mapParameterValue(ChoroborosAudioProcessor::COLOR_ID,
                                                                           readRawParam(ChoroborosAudioProcessor::COLOR_ID)));
        const float mixMapped = juce::jlimit(0.0f, 1.0f,
                                             processor.mapParameterValue(ChoroborosAudioProcessor::MIX_ID,
                                                                         readRawParam(ChoroborosAudioProcessor::MIX_ID)));
        const bool isRedNQ = engine == 2 && !hq;
        const bool isRedHQ = engine == 2 && hq;

        state.modeLabel = names[engine] + (hq ? " HQ" : " NQ");
        state.stages[0] = { "Input", "Audio In", true };
        state.stages[1] = { "Tone", juce::String(rt.preEmphasisFreqHz.load(), 0) + " Hz", true };
        state.stages[2] = { "Filters", juce::String(rt.hpfCutoffHz.load(), 0) + " / " + juce::String(rt.lpfCutoffHz.load(), 0), true };
        state.stages[3] = { "Core", isRedNQ ? "BBD" : (isRedHQ ? "Tape" : "Chorus"), true };
        state.stages[4] = { "Color", juce::String(colorMapped, 3), true };
        state.stages[5] = { "Drive", isRedNQ || isRedHQ ? "Active" : "Bypass", isRedNQ || isRedHQ };
        state.stages[6] = { "Output", juce::String(mixMapped * 100.0f, 1) + "% wet", true };

        auto toNorm = [](float db)
        {
            return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        };
        if (analyzer.valid)
        {
            state.preLevel = toNorm(analyzer.inputPeakDb);
            state.wetLevel = toNorm(analyzer.wetPeakDb);
            state.postLevel = toNorm(analyzer.outputPeakDb);
            state.live = true;
            state.clipped = analyzer.outputPeakDb > -0.5f;
        }
        else
        {
            state.live = false;
            state.clipped = false;
        }
        return state;
    }, LinkGroup::engine);
    engineSignalFlowCard->setPreferredHeight(286);
    engineVisuals.add(engineSignalFlowCard);
    for (auto* visual : engineVisuals)
    {
        if (visual != nullptr)
        {
            engineVisualDeck.addAndMakeVisible(visual);
            engineVisualDeckCards.add(visual);
        }
    }
    setSectionRowHeight(engineMacroDerived, kRowHeightCompact);
    addPanelSection(enginePanel, "Engine Macro Derived", engineMacroDerived, false);
}

void DevPanel::buildValidationTab(DevPanelBuildContext& ctx)
{
    const auto& makeReadOnly = ctx.makeReadOnly;
    const auto& makeTraceMatrix = ctx.makeTraceMatrix;
    const auto& readRawParam = ctx.readRawParam;
    const auto& getActiveProfileRaw = ctx.getActiveProfileRaw;
    const auto& readAnalyzerSnapshot = ctx.readAnalyzerSnapshot;
    const auto& registerControlMetadata = ctx.registerControlMetadata;
    juce::Array<juce::PropertyComponent*> validationTelemetry;
    validationTelemetry.add(makeReadOnly("Audio Thread Time", [this]() -> juce::String
    {
        const auto& t = processor.getLiveTelemetry();
        return juce::String(t.lastProcessMs.load(), 3) + " ms (last), "
             + juce::String(t.maxProcessMs.load(), 3) + " ms (peak)";
    }));
    validationTelemetry.add(makeReadOnly("Signal Peak Hold (In)", [this]() -> juce::String
    {
        const auto& t = processor.getLiveTelemetry();
        const float lDb = juce::Decibels::gainToDecibels(juce::jmax(0.000001f, t.inputPeakL.load()));
        const float rDb = juce::Decibels::gainToDecibels(juce::jmax(0.000001f, t.inputPeakR.load()));
        return "L " + juce::String(lDb, 1) + " dBFS, R " + juce::String(rDb, 1) + " dBFS";
    }));
    validationTelemetry.add(makeReadOnly("Signal Peak Hold (Out)", [this]() -> juce::String
    {
        const auto& t = processor.getLiveTelemetry();
        const float lDb = juce::Decibels::gainToDecibels(juce::jmax(0.000001f, t.outputPeakL.load()));
        const float rDb = juce::Decibels::gainToDecibels(juce::jmax(0.000001f, t.outputPeakR.load()));
        return "L " + juce::String(lDb, 1) + " dBFS, R " + juce::String(rDb, 1) + " dBFS";
    }));
    validationTelemetry.add(makeReadOnly("Callbacks / Writes", [this]() -> juce::String
    {
        const auto& t = processor.getLiveTelemetry();
        return juce::String(static_cast<long long>(t.processBlockCount.load())) + " blocks, "
             + juce::String(static_cast<long long>(t.parameterWriteCount.load())) + " param writes";
    }));
    validationTelemetry.add(makeReadOnly("Mode Switches", [this]() -> juce::String
    {
        const auto& t = processor.getLiveTelemetry();
        return juce::String(static_cast<long long>(t.engineSwitchCount.load())) + " engine, "
             + juce::String(static_cast<long long>(t.hqToggleCount.load())) + " HQ";
    }));
    validationTelemetry.add(makeReadOnly("Host Audio Config", [this]() -> juce::String
    {
        const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 0.0;
        const int bs = processor.getBlockSize();
        return juce::String(sr, 0) + " Hz, block " + juce::String(bs) + " samples";
    }));
    auto* analyzerFrameProp = makeReadOnly("Analyzer Frame", [readAnalyzerSnapshot]() -> juce::String
    {
        const auto snapshot = readAnalyzerSnapshot();
        return snapshot.valid
            ? ("seq " + juce::String(static_cast<long long>(snapshot.sequence))
               + ", " + juce::String(snapshot.sampleRate, 0) + " Hz")
            : "pending";
    });
    validationTelemetry.add(analyzerFrameProp);
    analyzerTelemetryProperties.add(analyzerFrameProp);

    auto* analyzerDelayProbeProp = makeReadOnly("Analyzer Delay Probe", [readAnalyzerSnapshot]() -> juce::String
    {
        const auto snapshot = readAnalyzerSnapshot();
        return snapshot.valid
            ? (juce::String(snapshot.centerDelayMs, 3) + " ms center, depth "
               + juce::String(snapshot.modulationDepthMs, 3) + " ms")
            : "pending";
    });
    validationTelemetry.add(analyzerDelayProbeProp);
    analyzerTelemetryProperties.add(analyzerDelayProbeProp);
    setSectionRowHeight(validationTelemetry, kRowHeightCompact);
    addPanelSection(validationPanel, "Live Telemetry (stream)", validationTelemetry, true);

    auto* validationTraceMatrixCard = makeTraceMatrix("Trace Matrix", [this, readRawParam, getActiveProfileRaw]() -> ValidationTraceMatrixPropertyComponent::State
    {
        ValidationTraceMatrixPropertyComponent::State state;
        state.snapshotSource = "active engine param profile (raw)";
        const auto& rt = processor.getDspInternals();
        const int engine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        const bool hq = processor.isHqEnabled();

        auto addRow = [&](const juce::String& control, const char* paramId,
                          std::function<juce::String(float)> effectiveProvider)
        {
            ValidationTraceMatrixPropertyComponent::Row row;
            row.control = control;
            row.uiRaw = readRawParam(paramId);
            row.mapped = processor.mapParameterValue(paramId, row.uiRaw);
            row.snapshotRaw = getActiveProfileRaw(paramId, row.profileValid);
            row.snapshotMapped = processor.mapParameterValue(paramId, row.snapshotRaw);
            row.inSync = row.profileValid && std::abs(row.uiRaw - row.snapshotRaw) <= 1.0e-4f;
            row.effective = effectiveProvider(row.mapped);
            state.rows.push_back(std::move(row));
        };

        addRow("Rate", ChoroborosAudioProcessor::RATE_ID, [&rt](float mapped) -> juce::String
        {
            return juce::String(mapped, 3) + " Hz (" + juce::String(rt.rateSmoothingMs.load(), 1) + " ms)";
        });
        addRow("Depth", ChoroborosAudioProcessor::DEPTH_ID, [&rt](float mapped) -> juce::String
        {
            const float centreMs = rt.centreDelayBaseMs.load() + rt.centreDelayScale.load() * mapped;
            return "Centre " + juce::String(centreMs, 3) + " ms";
        });
        addRow("Offset", ChoroborosAudioProcessor::OFFSET_ID, [](float mapped) -> juce::String
        {
            return juce::String(mapped, 2) + " deg";
        });
        addRow("Width", ChoroborosAudioProcessor::WIDTH_ID, [&rt](float mapped) -> juce::String
        {
            return juce::String(mapped, 3) + "x (" + juce::String(rt.widthSmoothingMs.load(), 1) + " ms)";
        });
        addRow("Color", ChoroborosAudioProcessor::COLOR_ID, [engine, hq, &rt](float mapped) -> juce::String
        {
            const float normalized = juce::jlimit(0.0f, 1.0f, mapped);
            if (engine == 2 && !hq)
            {
                const float drive = 1.0f + rt.saturationDriveScale.load() * normalized;
                return "Drive x" + juce::String(drive, 3);
            }
            if (engine == 2 && hq)
            {
                const float drive = 1.0f + rt.tapeDriveScale.load() * normalized;
                return "Tape x" + juce::String(drive, 3);
            }
            if (engine == 0) return "Bloom " + juce::String(normalized, 3);
            if (engine == 1) return "Focus " + juce::String(normalized, 3);
            if (engine == 3) return hq ? ("Orbit " + juce::String(normalized, 3)) : ("Warp " + juce::String(normalized, 3));
            if (engine == 4) return hq ? ("Ensemble " + juce::String(normalized, 3)) : ("Intensity " + juce::String(normalized, 3));
            return juce::String(normalized, 3);
        });
        addRow("Mix", ChoroborosAudioProcessor::MIX_ID, [](float mapped) -> juce::String
        {
            return juce::String(juce::jlimit(0.0f, 1.0f, mapped) * 100.0f, 1) + "% wet";
        });

        return state;
    });
    validationTraceMatrixCard->setPreferredHeight(500);
    if (validationVisualDeckCards.isEmpty())
        validationVisualDeckCards.add(nullptr); // Telemetry subview uses inspector-only readouts.
    validationVisualDeck.addAndMakeVisible(validationTraceMatrixCard);
    validationVisualDeckCards.add(validationTraceMatrixCard);

    auto* validationConsole = new CommandConsolePropertyComponent("Validation Console",
                                                                  [this](const juce::String& text)
                                                                  {
                                                                      return executeConsoleCommand(text);
                                                                  },
                                                                  "Power console for direct Dev Panel state control. Type `help` for commands.",
                                                                  [this]()
                                                                  {
                                                                      return buildConsoleWatchHudText();
                                                                  });
    validationConsole->setPreferredHeight(264);
    liveReadoutProperties.add(validationConsole);
    validationVisualDeck.addAndMakeVisible(validationConsole);
    validationVisualDeckCards.add(validationConsole);
    registerControlMetadata("Validation Console", {}, "command_console", "interactive", "console_only");
}

void DevPanel::buildInternalsTab(DevPanelBuildContext& ctx)
{
    const auto& makeLockable = ctx.makeLockable;
    auto getProfilePanel = [this](int engineIndex, bool hqEnabled) -> juce::PropertyPanel&
    {
        switch (juce::jlimit(0, 4, engineIndex))
        {
            case 0: return hqEnabled ? internalsGreenHqPanel : internalsGreenNqPanel;
            case 1: return hqEnabled ? internalsBlueHqPanel : internalsBlueNqPanel;
            case 2: return hqEnabled ? internalsRedHqPanel : internalsRedNqPanel;
            case 3: return hqEnabled ? internalsPurpleHqPanel : internalsPurpleNqPanel;
            case 4: default: return hqEnabled ? internalsBlackHqPanel : internalsBlackNqPanel;
        }
    };

    auto addInternal = [&](int engineIndex, bool hqEnabled, const juce::String& name, std::atomic<float>& target,
                           double min, double max, double step, double skew = 1.0)
    {
        return makeLockable(
            makeFloatValue([&target] { return target.load(); },
                           [&, engineIndex, hqEnabled](float v)
                           {
                               target.store(v);
                               processor.syncEngineInternalsToActiveDsp(engineIndex, hqEnabled);
                           }),
            name, min, max, step, skew);
    };

    auto addInternalsSectionsForEngine = [&](int engineIndex, bool hqEnabled,
                                             ChorusDSP::RuntimeTuning& engineTuning,
                                             bool firstOpen)
    {
        auto& profilePanel = getProfilePanel(engineIndex, hqEnabled);
        const bool isRedNQ = (engineIndex == 2 && !hqEnabled);
        const bool usesPreEmphasis = !isRedNQ;
        // saturationDriveScale is only used by Red NQ post-chorus drive.
        const bool usesSaturation = isRedNQ;
        juce::Array<juce::PropertyComponent*> dspTimingAndMotion;
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Rate Smooth (ms)", engineTuning.rateSmoothingMs, 0.0, 200.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Depth Smooth (ms)", engineTuning.depthSmoothingMs, 0.0, 500.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Depth Rate Limit", engineTuning.depthRateLimit, 0.01, 5.0, 0.01, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Smooth (ms)", engineTuning.centreDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Base (ms)", engineTuning.centreDelayBaseMs, 0.0, 30.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Centre Scale", engineTuning.centreDelayScale, 0.0, 30.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Color Smooth (ms)", engineTuning.colorSmoothingMs, 0.0, 200.0, 0.1, 1.0));
        dspTimingAndMotion.add(addInternal(engineIndex, hqEnabled, "Width Smooth (ms)", engineTuning.widthSmoothingMs, 0.0, 200.0, 0.1, 1.0));

        juce::Array<juce::PropertyComponent*> dspFiltering;
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "HPF Cutoff (Hz)", engineTuning.hpfCutoffHz, 5.0, 200.0, 0.1, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "HPF Q", engineTuning.hpfQ, 0.1, 2.0, 0.001, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "LPF Cutoff (Hz)", engineTuning.lpfCutoffHz, 100.0, 20000.0, 1.0, 1.0));
        dspFiltering.add(addInternal(engineIndex, hqEnabled, "LPF Q", engineTuning.lpfQ, 0.1, 4.0, 0.001, 1.0));
        if (usesPreEmphasis)
        {
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Freq (Hz)", engineTuning.preEmphasisFreqHz, 200.0, 12000.0, 1.0, 1.0));
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Q", engineTuning.preEmphasisQ, 0.1, 4.0, 0.001, 1.0));
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Gain", engineTuning.preEmphasisGain, 0.1, 4.0, 0.001, 1.0));
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Level Smooth", engineTuning.preEmphasisLevelSmoothing, 0.0, 1.0, 0.001, 1.0));
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Quiet Thresh", engineTuning.preEmphasisQuietThreshold, 0.0, 1.0, 0.001, 1.0));
            dspFiltering.add(addInternal(engineIndex, hqEnabled, "PreEmph Max Amount", engineTuning.preEmphasisMaxAmount, 0.0, 1.0, 0.001, 1.0));
        }

        juce::Array<juce::PropertyComponent*> dspCompressor;
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Attack (ms)", engineTuning.compressorAttackMs, 0.1, 200.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Release (ms)", engineTuning.compressorReleaseMs, 1.0, 500.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Threshold (dB)", engineTuning.compressorThresholdDb, -30.0, 6.0, 0.1, 1.0));
        dspCompressor.add(addInternal(engineIndex, hqEnabled, "Comp Ratio", engineTuning.compressorRatio, 1.0, 12.0, 0.01, 1.0));

        addPanelSection(profilePanel, "Timing + Motion", dspTimingAndMotion, firstOpen);
        addPanelSection(profilePanel, usesPreEmphasis ? "Filtering + Emphasis" : "Filtering", dspFiltering, false);
        addPanelSection(profilePanel, "Compressor", dspCompressor, false);
        if (usesSaturation)
        {
            juce::Array<juce::PropertyComponent*> dspSaturation;
            dspSaturation.add(addInternal(engineIndex, hqEnabled, "Saturation Drive Scale", engineTuning.saturationDriveScale, 0.0, 6.0, 0.01, 1.0));
            addPanelSection(profilePanel, "Saturation", dspSaturation, false);
        }

        if (engineIndex == 0)
        {
            juce::Array<juce::PropertyComponent*> greenBloom;
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Exponent", engineTuning.greenBloomExponent, 0.1, 4.0, 0.01, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Depth Scale", engineTuning.greenBloomDepthScale, 0.0, 2.0, 0.01, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Centre Offset (ms)", engineTuning.greenBloomCentreOffsetMs, 0.0, 8.0, 0.01, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Cutoff Max (Hz)", engineTuning.greenBloomCutoffMaxHz, 1000.0, 20000.0, 1.0, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Cutoff Min (Hz)", engineTuning.greenBloomCutoffMinHz, 100.0, 10000.0, 1.0, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Wet Blend", engineTuning.greenBloomWetBlend, 0.0, 1.0, 0.001, 1.0));
            greenBloom.add(addInternal(engineIndex, hqEnabled, "Green Bloom Gain", engineTuning.greenBloomGain, 0.0, 1.0, 0.001, 1.0));
            addPanelSection(profilePanel, "Green Bloom", greenBloom, false);
        }

        if (engineIndex == 1)
        {
            juce::Array<juce::PropertyComponent*> blueFocus;
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus Exponent", engineTuning.blueFocusExponent, 0.1, 4.0, 0.01, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus HP Min (Hz)", engineTuning.blueFocusHpMinHz, 20.0, 2000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus HP Max (Hz)", engineTuning.blueFocusHpMaxHz, 20.0, 6000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus LP Max (Hz)", engineTuning.blueFocusLpMaxHz, 2000.0, 20000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus LP Min (Hz)", engineTuning.blueFocusLpMinHz, 500.0, 20000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Presence Freq Min (Hz)", engineTuning.bluePresenceFreqMinHz, 200.0, 10000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Presence Freq Max (Hz)", engineTuning.bluePresenceFreqMaxHz, 200.0, 12000.0, 1.0, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Presence Q Min", engineTuning.bluePresenceQMin, 0.1, 4.0, 0.001, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Presence Q Max", engineTuning.bluePresenceQMax, 0.1, 6.0, 0.001, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Presence Gain Max (dB)", engineTuning.bluePresenceGainMaxDb, 0.0, 18.0, 0.01, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus Wet Blend", engineTuning.blueFocusWetBlend, 0.0, 1.0, 0.001, 1.0));
            blueFocus.add(addInternal(engineIndex, hqEnabled, "Blue Focus Output Gain", engineTuning.blueFocusOutputGain, 0.0, 1.0, 0.001, 1.0));
            addPanelSection(profilePanel, "Blue Focus", blueFocus, false);
        }

        if (engineIndex == 3 && !hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> purpleWarp;
            purpleWarp.add(addInternal(engineIndex, hqEnabled, "Purple Warp A", engineTuning.purpleWarpA, 0.0, 1.5, 0.001, 1.0));
            purpleWarp.add(addInternal(engineIndex, hqEnabled, "Purple Warp B", engineTuning.purpleWarpB, 0.0, 1.5, 0.001, 1.0));
            purpleWarp.add(addInternal(engineIndex, hqEnabled, "Purple Warp K Base", engineTuning.purpleWarpKBase, 0.1, 6.0, 0.001, 1.0));
            purpleWarp.add(addInternal(engineIndex, hqEnabled, "Purple Warp K Scale", engineTuning.purpleWarpKScale, 0.0, 6.0, 0.001, 1.0));
            purpleWarp.add(addInternal(engineIndex, hqEnabled, "Purple Warp Delay Smooth (ms)", engineTuning.purpleWarpDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
            addPanelSection(profilePanel, "Purple Warp", purpleWarp, false);
        }

        if (engineIndex == 3 && hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> purpleOrbit;
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Eccentricity", engineTuning.purpleOrbitEccentricity, 0.0, 1.5, 0.001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Theta Base (Hz)", engineTuning.purpleOrbitThetaRateBaseHz, 0.0, 0.5, 0.0001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Theta Scale (Hz)", engineTuning.purpleOrbitThetaRateScaleHz, 0.0, 0.5, 0.0001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Theta2 Ratio", engineTuning.purpleOrbitThetaRate2Ratio, 0.1, 3.0, 0.001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Eccentricity2 Ratio", engineTuning.purpleOrbitEccentricity2Ratio, 0.0, 3.0, 0.001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Mix1", engineTuning.purpleOrbitMix1, 0.0, 1.0, 0.001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Stereo Theta Offset", engineTuning.purpleOrbitStereoThetaOffset, -1.0, 1.0, 0.001, 1.0));
            purpleOrbit.add(addInternal(engineIndex, hqEnabled, "Purple Orbit Delay Smooth (ms)", engineTuning.purpleOrbitDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
            addPanelSection(profilePanel, "Purple Orbit", purpleOrbit, false);
        }

        if (engineIndex == 4 && !hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> blackNq;
            blackNq.add(addInternal(engineIndex, hqEnabled, "Black NQ Depth Base", engineTuning.blackNqDepthBase, 0.0, 3.0, 0.001, 1.0));
            blackNq.add(addInternal(engineIndex, hqEnabled, "Black NQ Depth Scale", engineTuning.blackNqDepthScale, 0.0, 3.0, 0.001, 1.0));
            blackNq.add(addInternal(engineIndex, hqEnabled, "Black NQ Delay Glide (ms)", engineTuning.blackNqDelayGlideMs, 0.0, 100.0, 0.01, 1.0));
            addPanelSection(profilePanel, "Black Intensity", blackNq, false);
        }

        if (engineIndex == 4 && hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> blackHq;
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Mix Base", engineTuning.blackHqTap2MixBase, 0.0, 1.0, 0.001, 1.0));
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Mix Scale", engineTuning.blackHqTap2MixScale, 0.0, 1.0, 0.001, 1.0));
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Depth Base", engineTuning.blackHqSecondTapDepthBase, 0.0, 3.0, 0.001, 1.0));
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Depth Scale", engineTuning.blackHqSecondTapDepthScale, 0.0, 3.0, 0.001, 1.0));
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Delay Offset Base", engineTuning.blackHqSecondTapDelayOffsetBase, 0.0, 12.0, 0.001, 1.0));
            blackHq.add(addInternal(engineIndex, hqEnabled, "Black HQ Tap2 Delay Offset Scale", engineTuning.blackHqSecondTapDelayOffsetScale, 0.0, 12.0, 0.001, 1.0));
            addPanelSection(profilePanel, "Black Ensemble", blackHq, false);
        }

        // BBD internals are only used by Red NQ (engine 2, HQ off).
        if (engineIndex == 2 && !hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> bbdDelayAndDepth;
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Smooth (ms)", engineTuning.bbdDelaySmoothingMs, 0.0, 500.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Min (ms)", engineTuning.bbdDelayMinMs, 1.0, 50.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Delay Max (ms)", engineTuning.bbdDelayMaxMs, 10.0, 200.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Centre Base (ms)", engineTuning.bbdCentreBaseMs, 0.0, 50.0, 0.1, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Centre Scale", engineTuning.bbdCentreScale, 0.0, 4.0, 0.01, 1.0));
            bbdDelayAndDepth.add(addInternal(engineIndex, hqEnabled, "BBD Depth (ms)", engineTuning.bbdDepthMs, 0.0, 40.0, 0.1, 1.0));

            juce::Array<juce::PropertyComponent*> bbdFilter;
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Smooth (ms)", engineTuning.bbdFilterSmoothingMs, 0.0, 200.0, 0.1, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Min (Hz)", engineTuning.bbdFilterCutoffMinHz, 200.0, 8000.0, 1.0, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Max (Hz)", engineTuning.bbdFilterCutoffMaxHz, 1000.0, 20000.0, 1.0, 1.0));
            bbdFilter.add(addInternal(engineIndex, hqEnabled, "BBD Filter Scale", engineTuning.bbdFilterCutoffScale, 0.05, 1.0, 0.001, 1.0));

            juce::Array<juce::PropertyComponent*> bbdClock;
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Smooth (ms)", engineTuning.bbdClockSmoothingMs, 0.0, 200.0, 0.1, 1.0));
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Min (Hz)", engineTuning.bbdClockMinHz, 200.0, 10000.0, 1.0, 1.0));
            bbdClock.add(addInternal(engineIndex, hqEnabled, "BBD Clock Max Ratio", engineTuning.bbdClockMaxRatio, 0.1, 1.0, 0.001, 1.0));

            juce::Array<juce::PropertyComponent*> bbdStructure;
            bbdStructure.add(addInternal(engineIndex, hqEnabled, "BBD Stages", engineTuning.bbdStages, 256.0, 2048.0, 256.0, 1.0));
            bbdStructure.add(addInternal(engineIndex, hqEnabled, "BBD Filter Max Ratio", engineTuning.bbdFilterMaxRatio, 0.1, 0.5, 0.01, 1.0));

            addPanelSection(bbdPanel, "Delay + Depth", bbdDelayAndDepth, firstOpen);
            addPanelSection(bbdPanel, "Filter", bbdFilter, false);
            addPanelSection(bbdPanel, "Clock", bbdClock, false);
            addPanelSection(bbdPanel, "Structure", bbdStructure, false);
        }

        // Tape internals are only used by Red HQ (engine 2, HQ on).
        if (engineIndex == 2 && hqEnabled)
        {
            juce::Array<juce::PropertyComponent*> tapeDelayAndTone;
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Delay Smooth (ms)", engineTuning.tapeDelaySmoothingMs, 0.0, 800.0, 0.1, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Centre Base (ms)", engineTuning.tapeCentreBaseMs, 0.0, 50.0, 0.1, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Centre Scale", engineTuning.tapeCentreScale, 0.0, 4.0, 0.01, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Max (Hz)", engineTuning.tapeToneMaxHz, 1000.0, 20000.0, 1.0, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Min (Hz)", engineTuning.tapeToneMinHz, 1000.0, 20000.0, 1.0, 1.0));
            tapeDelayAndTone.add(addInternal(engineIndex, hqEnabled, "Tape Tone Smooth", engineTuning.tapeToneSmoothingCoeff, 0.0, 1.0, 0.0001, 1.0));

            juce::Array<juce::PropertyComponent*> tapeMotionAndModulation;
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape LFO Ratio", engineTuning.tapeLfoRatioScale, 0.0, 0.2, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape LFO Smooth", engineTuning.tapeLfoModSmoothingCoeff, 0.0, 0.05, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Smooth", engineTuning.tapeRatioSmoothingCoeff, 0.0, 0.05, 0.0001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Phase Damp", engineTuning.tapePhaseDamping, 0.9, 1.0, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Freq Base", engineTuning.tapeWowFreqBase, 0.0, 2.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Freq Spread", engineTuning.tapeWowFreqSpread, 0.0, 0.5, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Freq Base", engineTuning.tapeFlutterFreqBase, 0.0, 12.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Freq Spread", engineTuning.tapeFlutterFreqSpread, 0.0, 2.0, 0.001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Depth Base", engineTuning.tapeWowDepthBase, 0.0, 0.01, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Wow Depth Spread", engineTuning.tapeWowDepthSpread, 0.0, 0.002, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Depth Base", engineTuning.tapeFlutterDepthBase, 0.0, 0.01, 0.00001, 1.0));
            tapeMotionAndModulation.add(addInternal(engineIndex, hqEnabled, "Tape Flutter Depth Spread", engineTuning.tapeFlutterDepthSpread, 0.0, 0.002, 0.00001, 1.0));

            juce::Array<juce::PropertyComponent*> tapeDriveAndOutput;
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Drive Scale", engineTuning.tapeDriveScale, 0.0, 2.0, 0.001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Min", engineTuning.tapeRatioMin, 0.9, 1.0, 0.0001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Ratio Max", engineTuning.tapeRatioMax, 1.0, 1.1, 0.0001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Wet Gain", engineTuning.tapeWetGain, 0.5, 2.0, 0.001, 1.0));
            tapeDriveAndOutput.add(addInternal(engineIndex, hqEnabled, "Tape Hermite Tension", engineTuning.tapeHermiteTension, 0.0, 1.0, 0.001, 1.0));

            addPanelSection(tapePanel, "Delay + Tone", tapeDelayAndTone, firstOpen);
            addPanelSection(tapePanel, "Motion + Modulation", tapeMotionAndModulation, false);
            addPanelSection(tapePanel, "Drive + Output", tapeDriveAndOutput, false);
        }
    };

    addInternalsSectionsForEngine(0, false, processor.getEngineDspInternals(0, false), true);
    addInternalsSectionsForEngine(0, true, processor.getEngineDspInternals(0, true), false);
    addInternalsSectionsForEngine(1, false, processor.getEngineDspInternals(1, false), false);
    addInternalsSectionsForEngine(1, true, processor.getEngineDspInternals(1, true), false);
    addInternalsSectionsForEngine(2, false, processor.getEngineDspInternals(2, false), false);
    addInternalsSectionsForEngine(2, true, processor.getEngineDspInternals(2, true), false);
    addInternalsSectionsForEngine(3, false, processor.getEngineDspInternals(3, false), false);
    addInternalsSectionsForEngine(3, true, processor.getEngineDspInternals(3, true), false);
    addInternalsSectionsForEngine(4, false, processor.getEngineDspInternals(4, false), false);
    addInternalsSectionsForEngine(4, true, processor.getEngineDspInternals(4, true), false);
}

void DevPanel::buildLayoutTab(DevPanelBuildContext& ctx)
{
    const auto& makeLockable = ctx.makeLockable;
    const auto& registerControlMetadata = ctx.registerControlMetadata;
    auto& layout = editor.getLayoutTuning();
    juce::Array<juce::PropertyComponent*> layoutSliderProps;
    juce::Array<juce::PropertyComponent*> layoutGreenProps;
    juce::Array<juce::PropertyComponent*> layoutBlueProps;
    juce::Array<juce::PropertyComponent*> layoutRedProps;
    juce::Array<juce::PropertyComponent*> layoutPurpleProps;
    juce::Array<juce::PropertyComponent*> layoutBlackProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalValueTypographyProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutColorValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutMixValueFxProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalMainValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalColorValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalMixValueFlipProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalTopButtonsProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalEngineSelectorProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalHqSwitchProps;
    juce::Array<juce::PropertyComponent*> layoutGlobalKnobResponseProps;
    auto addLayout = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name, int& target, int min, int max)
    {
        props.add(makeLockable(
            makeFloatValue([&] { return static_cast<float>(target); },
                           [&](float v)
                           {
                               target = static_cast<int>(std::round(v));
                               editor.applyLayout();
                           }),
            name, static_cast<double>(min), static_cast<double>(max), 1.0, 1.0));
    };
    auto addLayoutToGroup = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& name, int& target, int min, int max)
    {
        addLayout(props, name, target, min, max);
    };
    auto addLayoutGlobal = [&](const juce::String& name, int& target, int min, int max)
    {
        addLayoutToGroup(layoutGlobalValueTypographyProps, name, target, min, max);
    };
    auto addColourChannels = [&](juce::Array<juce::PropertyComponent*>& props, const juce::String& baseName, int& target)
    {
        auto addChannel = [&](const juce::String& channelName, int shift)
        {
            props.add(makeLockable(
                makeFloatValue([&target, shift]
                               {
                                   return static_cast<float>((target >> shift) & 0xff);
                               },
                               [this, &target, shift](float v)
                               {
                                   const int clamped = juce::jlimit(0, 255, static_cast<int>(std::round(v)));
                                   target = (target & ~(0xff << shift)) | (clamped << shift);
                                   this->editor.applyLayout();
                               }),
                baseName + " " + channelName, 0.0, 255.0, 1.0, 1.0));
        };
        addChannel("A", 24);
        addChannel("R", 16);
        addChannel("G", 8);
        addChannel("B", 0);
    };
    auto addLayoutByColor = [&](const juce::String& baseName,
                                int& green, int& blue, int& red, int& purple, int& black,
                                int min, int max)
    {
        addLayout(layoutGreenProps, baseName, green, min, max);
        addLayout(layoutBlueProps, baseName, blue, min, max);
        addLayout(layoutRedProps, baseName, red, min, max);
        addLayout(layoutPurpleProps, baseName, purple, min, max);
        addLayout(layoutBlackProps, baseName, black, min, max);
    };

    addLayoutByColor("Main Knob Size", layout.mainKnobSizeGreen, layout.mainKnobSizeBlue, layout.mainKnobSizeRed, layout.mainKnobSizePurple, layout.mainKnobSizeBlack, 40, 260);
    addLayoutByColor("Knob Top Y", layout.knobTopYGreen, layout.knobTopYBlue, layout.knobTopYRed, layout.knobTopYPurple, layout.knobTopYBlack, 0, 300);
    addLayoutByColor("Rate Center X", layout.rateCenterXGreen, layout.rateCenterXBlue, layout.rateCenterXRed, layout.rateCenterXPurple, layout.rateCenterXBlack, 0, 800);
    addLayoutByColor("Depth Center X", layout.depthCenterXGreen, layout.depthCenterXBlue, layout.depthCenterXRed, layout.depthCenterXPurple, layout.depthCenterXBlack, 0, 800);
    addLayoutByColor("Offset Center X", layout.offsetCenterXGreen, layout.offsetCenterXBlue, layout.offsetCenterXRed, layout.offsetCenterXPurple, layout.offsetCenterXBlack, 0, 800);
    addLayoutByColor("Width Center X", layout.widthCenterXGreen, layout.widthCenterXBlue, layout.widthCenterXRed, layout.widthCenterXPurple, layout.widthCenterXBlack, 0, 800);
    addLayoutByColor("Slider Track Start X", layout.sliderTrackStartXGreen, layout.sliderTrackStartXBlue, layout.sliderTrackStartXRed, layout.sliderTrackStartXPurple, layout.sliderTrackStartXBlack, 0, 800);
    addLayoutByColor("Slider Track Start Y", layout.sliderTrackStartYGreen, layout.sliderTrackStartYBlue, layout.sliderTrackStartYRed, layout.sliderTrackStartYPurple, layout.sliderTrackStartYBlack, 0, 500);
    addLayoutByColor("Slider Track End X", layout.sliderTrackEndXGreen, layout.sliderTrackEndXBlue, layout.sliderTrackEndXRed, layout.sliderTrackEndXPurple, layout.sliderTrackEndXBlack, 0, 800);
    addLayoutByColor("Slider Track End Y", layout.sliderTrackEndYGreen, layout.sliderTrackEndYBlue, layout.sliderTrackEndYRed, layout.sliderTrackEndYPurple, layout.sliderTrackEndYBlack, 0, 500);
    addLayoutByColor("Slider Size (%)", layout.sliderSizeGreen, layout.sliderSizeBlue, layout.sliderSizeRed, layout.sliderSizePurple, layout.sliderSizeBlack, 10, 500);
    addLayout(layoutSliderProps, "Mix Knob Y (legacy)", layout.mixKnobY, 0, 500);
    addLayoutByColor("Mix Knob Y", layout.mixKnobYGreen, layout.mixKnobYBlue, layout.mixKnobYRed, layout.mixKnobYPurple, layout.mixKnobYBlack, 0, 500);
    addLayout(layoutSliderProps, "Color Value Center X", layout.colorValueCenterX, 0, 800);
    addLayoutByColor("Mix Knob Size", layout.mixKnobSizeGreen, layout.mixKnobSizeBlue, layout.mixKnobSizeRed, layout.mixKnobSizePurple, layout.mixKnobSizeBlack, 10, 260);
    addLayoutByColor("Mix Center X", layout.mixCenterXGreen, layout.mixCenterXBlue, layout.mixCenterXRed, layout.mixCenterXPurple, layout.mixCenterXBlack, 0, 800);
    addLayoutByColor("Value Label Y", layout.valueLabelYGreen, layout.valueLabelYBlue, layout.valueLabelYRed, layout.valueLabelYPurple, layout.valueLabelYBlack, 0, 500);
    addLayoutByColor("Rate Value X Offset", layout.rateValueOffsetXGreen, layout.rateValueOffsetXBlue, layout.rateValueOffsetXRed, layout.rateValueOffsetXPurple, layout.rateValueOffsetXBlack, -200, 200);
    addLayoutByColor("Depth Value X Offset", layout.depthValueOffsetXGreen, layout.depthValueOffsetXBlue, layout.depthValueOffsetXRed, layout.depthValueOffsetXPurple, layout.depthValueOffsetXBlack, -200, 200);
    addLayoutByColor("Offset Value X Offset", layout.offsetValueOffsetXGreen, layout.offsetValueOffsetXBlue, layout.offsetValueOffsetXRed, layout.offsetValueOffsetXPurple, layout.offsetValueOffsetXBlack, -200, 200);
    addLayoutByColor("Width Value X Offset", layout.widthValueOffsetXGreen, layout.widthValueOffsetXBlue, layout.widthValueOffsetXRed, layout.widthValueOffsetXPurple, layout.widthValueOffsetXBlack, -200, 200);
    addLayoutByColor("Color Value Y", layout.colorValueYGreen, layout.colorValueYBlue, layout.colorValueYRed, layout.colorValueYPurple, layout.colorValueYBlack, 0, 500);
    addLayoutByColor("Color Value X Offset", layout.colorValueXOffsetGreen, layout.colorValueXOffsetBlue, layout.colorValueXOffsetRed, layout.colorValueXOffsetPurple, layout.colorValueXOffsetBlack, -200, 200);
    addLayoutByColor("Mix Value Y", layout.mixValueYGreen, layout.mixValueYBlue, layout.mixValueYRed, layout.mixValueYPurple, layout.mixValueYBlack, 0, 500);
    addLayoutByColor("Mix Value X Offset", layout.mixValueOffsetXGreen, layout.mixValueOffsetXBlue, layout.mixValueOffsetXRed, layout.mixValueOffsetXPurple, layout.mixValueOffsetXBlack, -200, 200);

    addLayoutGlobal("Rate Value Y Offset", layout.rateValueOffsetY, -200, 200);
    addLayoutGlobal("Depth Value Y Offset", layout.depthValueOffsetY, -200, 200);
    addLayoutGlobal("Offset Value Y Offset", layout.offsetValueOffsetY, -200, 200);
    addLayoutGlobal("Width Value Y Offset", layout.widthValueOffsetY, -200, 200);
    addLayoutGlobal("Main Knobs Value Font Size", layout.knobValueFontSize, 8, 48);
    addLayoutGlobal("Color Value Font Size", layout.colorValueFontSize, 8, 48);
    addLayoutGlobal("Mix Value Font Size", layout.mixValueFontSize, 8, 48);
    addLayoutGlobal("Value Text Alpha (%)", layout.valueTextAlphaPct, 0, 100);
    addLayoutGlobal("Value Text Colour Mode (0=Auto,1=Custom)", layout.valueTextColourMode, 0, 1);
    addColourChannels(layoutGlobalValueTypographyProps, "Value Text Colour", layout.valueTextColour);

    addLayoutToGroup(layoutGlobalValueFxProps, "FX Enabled (0/1)", layout.valueFxEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalValueFxProps, "Glow Alpha (%)", layout.valueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Glow Spread (x0.01 px)", layout.valueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutGlobalValueFxProps, "Per-Char Offset X (x0.01 px)", layout.valueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutGlobalValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.valueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Alpha (%)", layout.valueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.valueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.valueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Shear (%)", layout.valueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutGlobalValueFxProps, "Top Reflect Rotate (deg)", layout.valueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Alpha (%)", layout.valueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.valueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.valueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Shear (%)", layout.valueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutGlobalValueFxProps, "Bottom Reflect Rotate (deg)", layout.valueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Blur (x0.01 px)", layout.valueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Squash (%)", layout.valueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutGlobalValueFxProps, "Reflect Motion (%)", layout.valueReflectMotionPct, 0, 200);

    addLayoutToGroup(layoutColorValueFxProps, "FX Enabled (0/1)", layout.colorValueFxEnabled, 0, 1);
    addLayoutToGroup(layoutColorValueFxProps, "Glow Alpha (%)", layout.colorValueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Glow Spread (x0.01 px)", layout.colorValueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutColorValueFxProps, "Per-Char Offset X (x0.01 px)", layout.colorValueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutColorValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.colorValueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Alpha (%)", layout.colorValueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.colorValueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.colorValueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Shear (%)", layout.colorValueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutColorValueFxProps, "Top Reflect Rotate (deg)", layout.colorValueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Alpha (%)", layout.colorValueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.colorValueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.colorValueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Shear (%)", layout.colorValueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutColorValueFxProps, "Bottom Reflect Rotate (deg)", layout.colorValueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Blur (x0.01 px)", layout.colorValueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Squash (%)", layout.colorValueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutColorValueFxProps, "Reflect Motion (%)", layout.colorValueReflectMotionPct, 0, 200);

    addLayoutToGroup(layoutMixValueFxProps, "FX Enabled (0/1)", layout.mixValueFxEnabled, 0, 1);
    addLayoutToGroup(layoutMixValueFxProps, "Glow Alpha (%)", layout.mixValueGlowAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Glow Spread (x0.01 px)", layout.mixValueGlowSpreadPxTimes100, 0, 800);
    addLayoutToGroup(layoutMixValueFxProps, "Per-Char Offset X (x0.01 px)", layout.mixValueFxPerCharOffsetXPxTimes100, -600, 600);
    addLayoutToGroup(layoutMixValueFxProps, "Per-Char Offset Y (x0.01 px)", layout.mixValueFxPerCharOffsetYPxTimes100, -600, 600);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Alpha (%)", layout.mixValueTopReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Offset X (x0.01 px)", layout.mixValueTopReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Offset Y (x0.01 px)", layout.mixValueTopReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Shear (%)", layout.mixValueTopReflectShearPct, -100, 100);
    addLayoutToGroup(layoutMixValueFxProps, "Top Reflect Rotate (deg)", layout.mixValueTopReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Alpha (%)", layout.mixValueBottomReflectAlphaPct, 0, 40);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Offset X (x0.01 px)", layout.mixValueBottomReflectOffsetXPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Offset Y (x0.01 px)", layout.mixValueBottomReflectOffsetYPxTimes100, -1200, 1200);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Shear (%)", layout.mixValueBottomReflectShearPct, -100, 100);
    addLayoutToGroup(layoutMixValueFxProps, "Bottom Reflect Rotate (deg)", layout.mixValueBottomReflectRotateDeg, -180, 180);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Blur (x0.01 px)", layout.mixValueReflectBlurPxTimes100, 0, 800);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Squash (%)", layout.mixValueReflectSquashPct, 0, 95);
    addLayoutToGroup(layoutMixValueFxProps, "Reflect Motion (%)", layout.mixValueReflectMotionPct, 0, 200);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Enabled (0/1)", layout.mainValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Duration (ms)", layout.mainValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Up (x0.01 px)", layout.mainValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Down (x0.01 px)", layout.mainValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel Out (%)", layout.mainValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Travel In (%)", layout.mainValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Shear (%)", layout.mainValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalMainValueFlipProps, "Scale Amount (%)", layout.mainValueFlipMinScalePct, 0, 95);

    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Enabled (0/1)", layout.colorValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Duration (ms)", layout.colorValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Up (x0.01 px)", layout.colorValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Down (x0.01 px)", layout.colorValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel Out (%)", layout.colorValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Travel In (%)", layout.colorValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Shear (%)", layout.colorValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalColorValueFlipProps, "Scale Amount (%)", layout.colorValueFlipMinScalePct, 0, 95);

    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Enabled (0/1)", layout.mixValueFlipEnabled, 0, 1);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Duration (ms)", layout.mixValueFlipDurationMs, 20, 1000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Up (x0.01 px)", layout.mixValueFlipTravelUpPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Down (x0.01 px)", layout.mixValueFlipTravelDownPxTimes100, 0, 2000);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel Out (%)", layout.mixValueFlipTravelOutPct, 0, 400);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Travel In (%)", layout.mixValueFlipTravelInPct, 0, 400);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Shear (%)", layout.mixValueFlipShearPct, 0, 100);
    addLayoutToGroup(layoutGlobalMixValueFlipProps, "Scale Amount (%)", layout.mixValueFlipMinScalePct, 0, 95);

    addLayout(layoutGlobalTopButtonsProps, "Buttons Width", layout.topButtonsWidth, 20, 220);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Height", layout.topButtonsHeight, 8, 80);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Gap", layout.topButtonsGap, 0, 40);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Right Margin", layout.topButtonsRightMargin, 0, 120);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Top Y", layout.topButtonsTopY, 0, 80);
    addLayout(layoutGlobalTopButtonsProps, "Buttons Font Size", layout.topButtonsFontSize, 6, 48);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Text Colour", layout.topButtonsTextColour);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Bg Colour", layout.topButtonsBackgroundColour);
    addColourChannels(layoutGlobalTopButtonsProps, "Buttons Bg On Colour", layout.topButtonsOnBackgroundColour);

    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector X", layout.engineSelectorX, 0, 680);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector Y", layout.engineSelectorY, 0, 360);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector W", layout.engineSelectorW, 40, 260);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector H", layout.engineSelectorH, 10, 80);
    addLayout(layoutGlobalEngineSelectorProps, "Engine Selector Font Size", layout.engineSelectorFontSize, 6, 48);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Text", layout.engineSelectorTextColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Bg", layout.engineSelectorBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Outline", layout.engineSelectorOutlineColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Selector Arrow", layout.engineSelectorArrowColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Bg", layout.engineSelectorPopupBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Text", layout.engineSelectorPopupTextColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Highlight Bg", layout.engineSelectorPopupHighlightedBackgroundColour);
    addColourChannels(layoutGlobalEngineSelectorProps, "Engine Popup Highlight Text", layout.engineSelectorPopupHighlightedTextColour);

    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Size", layout.hqSwitchSize, 20, 180);
    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Offset X", layout.hqSwitchOffsetX, -300, 300);
    addLayout(layoutGlobalHqSwitchProps, "Flip Switch Offset Y", layout.hqSwitchOffsetY, -300, 300);
    addLayout(layoutGlobalKnobResponseProps, "Knob Drag Sensitivity (%)", layout.knobDragSensitivityPct, 10, 400);
    addLayout(layoutGlobalKnobResponseProps, "Knob Roll-Off Speed (%)", layout.knobRollOffSpeedPct, 10, 400);
    addLayout(layoutGlobalKnobResponseProps, "Rate Visual Response (ms)", layout.rateKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Depth Visual Response (ms)", layout.depthKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Offset Visual Response (ms)", layout.offsetKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Width Visual Response (ms)", layout.widthKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Mix Visual Response (ms)", layout.mixKnobVisualResponseMs, 1, 1000);
    addLayout(layoutGlobalKnobResponseProps, "Knob Sweep Start (deg)", layout.knobSweepStartDeg, 0, 360);
    addLayout(layoutGlobalKnobResponseProps, "Knob Sweep End (deg)", layout.knobSweepEndDeg, 0, 360);
    addLayout(layoutGlobalKnobResponseProps, "Knob Frame Count", layout.knobFrameCount, 2, 156);

    addPanelSection(layoutGreenPanel, "Layout Controls", layoutGreenProps, true);
    addPanelSection(layoutBluePanel, "Layout Controls", layoutBlueProps, true);
    addPanelSection(layoutRedPanel, "Layout Controls", layoutRedProps, true);
    addPanelSection(layoutPurplePanel, "Layout Controls", layoutPurpleProps, true);
    addPanelSection(layoutBlackPanel, "Layout Controls", layoutBlackProps, true);

    addPanelSection(layoutGlobalPanel, "Global (Mix Knob Y, Color Value X)", layoutSliderProps, true);
    addPanelSection(layoutGlobalPanel, "Global Value Typography", layoutGlobalValueTypographyProps, false);
    addPanelSection(layoutGlobalPanel, "Global Value FX - Main Knobs", layoutGlobalValueFxProps, false);
    addPanelSection(layoutGlobalPanel, "Global Value FX - Color", layoutColorValueFxProps, false);
    addPanelSection(layoutGlobalPanel, "Global Value FX - Mix", layoutMixValueFxProps, false);
    addPanelSection(layoutGlobalPanel, "Value Flip - Main Knobs", layoutGlobalMainValueFlipProps, false);
    addPanelSection(layoutGlobalPanel, "Value Flip - Color", layoutGlobalColorValueFlipProps, false);
    addPanelSection(layoutGlobalPanel, "Value Flip - Mix", layoutGlobalMixValueFlipProps, false);
    addPanelSection(layoutGlobalPanel, "Top Buttons (About/Help/Feedback)", layoutGlobalTopButtonsProps, false);
    addPanelSection(layoutGlobalPanel, "Engine Selector (Combo + Popup)", layoutGlobalEngineSelectorProps, false);
    addPanelSection(layoutGlobalPanel, "HQ Flip Switch", layoutGlobalHqSwitchProps, false);
    addPanelSection(layoutGlobalPanel, "Global Knob Response", layoutGlobalKnobResponseProps, false);

    auto* layoutPreviewCard = new LayoutPreviewPropertyComponent(
        "Live UI Preview (Grid + Coordinates)",
        [this]() -> LayoutPreviewPropertyComponent::State
        {
            LayoutPreviewPropertyComponent::State state;
            const auto& layout = editor.getLayoutTuning();
            static const juce::String engineNames[] { "Green", "Blue", "Red", "Purple", "Black" };
            const int engineIndex = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
            const bool hqEnabled = processor.isHqEnabled();
            const int subTab = getSelectedSubTab();
            const bool emphasizeGlobal = (subTab == 3);
            const bool emphasizeEngineLayout = !emphasizeGlobal;

            state.profileLabel = engineNames[engineIndex] + (hqEnabled ? " HQ" : " NQ");
            state.subtitle = emphasizeGlobal
                ? "Global layout focus: top buttons, engine selector, HQ switch."
                : "Engine layout focus: knobs, slider, mix, and value labels.";
            state.footer = "Grid units are editor-space pixels (700x363) used by Layout controls.";

            const auto pickByColor = [engineIndex](int green, int blue, int red, int purple, int black, int fallback) -> int
            {
                if (engineIndex == 0) return green;
                if (engineIndex == 1) return blue;
                if (engineIndex == 2) return red;
                if (engineIndex == 3) return purple;
                if (engineIndex == 4) return black;
                return fallback;
            };

            const int mainKnobSize = pickByColor(layout.mainKnobSizeGreen, layout.mainKnobSizeBlue, layout.mainKnobSizeRed,
                                                 layout.mainKnobSizePurple, layout.mainKnobSizeBlack, layout.mainKnobSize);
            const int knobTopY = pickByColor(layout.knobTopYGreen, layout.knobTopYBlue, layout.knobTopYRed,
                                             layout.knobTopYPurple, layout.knobTopYBlack, layout.knobTopY);
            const int rateCenterX = pickByColor(layout.rateCenterXGreen, layout.rateCenterXBlue, layout.rateCenterXRed,
                                                layout.rateCenterXPurple, layout.rateCenterXBlack, layout.rateCenterX);
            const int depthCenterX = pickByColor(layout.depthCenterXGreen, layout.depthCenterXBlue, layout.depthCenterXRed,
                                                 layout.depthCenterXPurple, layout.depthCenterXBlack, layout.depthCenterX);
            const int offsetCenterX = pickByColor(layout.offsetCenterXGreen, layout.offsetCenterXBlue, layout.offsetCenterXRed,
                                                  layout.offsetCenterXPurple, layout.offsetCenterXBlack, layout.offsetCenterX);
            const int widthCenterX = pickByColor(layout.widthCenterXGreen, layout.widthCenterXBlue, layout.widthCenterXRed,
                                                 layout.widthCenterXPurple, layout.widthCenterXBlack, layout.widthCenterX);

            const int trackStartX = pickByColor(layout.sliderTrackStartXGreen, layout.sliderTrackStartXBlue, layout.sliderTrackStartXRed,
                                                layout.sliderTrackStartXPurple, layout.sliderTrackStartXBlack, layout.sliderTrackStartX);
            const int trackStartY = pickByColor(layout.sliderTrackStartYGreen, layout.sliderTrackStartYBlue, layout.sliderTrackStartYRed,
                                                layout.sliderTrackStartYPurple, layout.sliderTrackStartYBlack, layout.sliderTrackStartY);
            const int trackEndX = pickByColor(layout.sliderTrackEndXGreen, layout.sliderTrackEndXBlue, layout.sliderTrackEndXRed,
                                              layout.sliderTrackEndXPurple, layout.sliderTrackEndXBlack, layout.sliderTrackEndX);
            const int trackEndY = pickByColor(layout.sliderTrackEndYGreen, layout.sliderTrackEndYBlue, layout.sliderTrackEndYRed,
                                              layout.sliderTrackEndYPurple, layout.sliderTrackEndYBlack, layout.sliderTrackEndY);
            const float sizeScale = static_cast<float>(pickByColor(layout.sliderSizeGreen, layout.sliderSizeBlue, layout.sliderSizeRed,
                                                                    layout.sliderSizePurple, layout.sliderSizeBlack, layout.sliderSize)) * 0.01f;
            const int sliderH = juce::jmax(6, juce::roundToInt(18.0f * sizeScale));
            const int sliderX = juce::jmin(trackStartX, trackEndX);
            const int sliderW = juce::jmax(1, std::abs(trackEndX - trackStartX));
            const int trackCenterY = (trackStartY + trackEndY) / 2;
            const int sliderY = trackCenterY - (sliderH / 2);

            const int mixKnobSize = pickByColor(layout.mixKnobSizeGreen, layout.mixKnobSizeBlue, layout.mixKnobSizeRed,
                                                layout.mixKnobSizePurple, layout.mixKnobSizeBlack, layout.mixKnobSize);
            const int mixCenterX = pickByColor(layout.mixCenterXGreen, layout.mixCenterXBlue, layout.mixCenterXRed,
                                               layout.mixCenterXPurple, layout.mixCenterXBlack, layout.mixCenterX);
            const int mixKnobY = pickByColor(layout.mixKnobYGreen, layout.mixKnobYBlue, layout.mixKnobYRed,
                                             layout.mixKnobYPurple, layout.mixKnobYBlack, layout.mixKnobY);

            const int valueLabelY = pickByColor(layout.valueLabelYGreen, layout.valueLabelYBlue, layout.valueLabelYRed,
                                                layout.valueLabelYPurple, layout.valueLabelYBlack, layout.valueLabelY);
            const int rateValueOffsetX = pickByColor(layout.rateValueOffsetXGreen, layout.rateValueOffsetXBlue, layout.rateValueOffsetXRed,
                                                     layout.rateValueOffsetXPurple, layout.rateValueOffsetXBlack, layout.rateValueOffsetX);
            const int depthValueOffsetX = pickByColor(layout.depthValueOffsetXGreen, layout.depthValueOffsetXBlue, layout.depthValueOffsetXRed,
                                                      layout.depthValueOffsetXPurple, layout.depthValueOffsetXBlack, layout.depthValueOffsetX);
            const int offsetValueOffsetX = pickByColor(layout.offsetValueOffsetXGreen, layout.offsetValueOffsetXBlue, layout.offsetValueOffsetXRed,
                                                       layout.offsetValueOffsetXPurple, layout.offsetValueOffsetXBlack, layout.offsetValueOffsetX);
            const int widthValueOffsetX = pickByColor(layout.widthValueOffsetXGreen, layout.widthValueOffsetXBlue, layout.widthValueOffsetXRed,
                                                      layout.widthValueOffsetXPurple, layout.widthValueOffsetXBlack, layout.widthValueOffsetX);

            const int colorValueY = pickByColor(layout.colorValueYGreen, layout.colorValueYBlue, layout.colorValueYRed,
                                                layout.colorValueYPurple, layout.colorValueYBlack, layout.colorValueY);
            const int colorValueXOffset = pickByColor(layout.colorValueXOffsetGreen, layout.colorValueXOffsetBlue, layout.colorValueXOffsetRed,
                                                      layout.colorValueXOffsetPurple, layout.colorValueXOffsetBlack, layout.colorValueXOffset);
            const int mixValueY = pickByColor(layout.mixValueYGreen, layout.mixValueYBlue, layout.mixValueYRed,
                                              layout.mixValueYPurple, layout.mixValueYBlack, layout.mixValueY);
            const int mixValueOffsetX = pickByColor(layout.mixValueOffsetXGreen, layout.mixValueOffsetXBlue, layout.mixValueOffsetXRed,
                                                    layout.mixValueOffsetXPurple, layout.mixValueOffsetXBlack, layout.mixValueOffsetX);

            auto addElement = [&state](const juce::String& label,
                                       juce::Rectangle<float> bounds,
                                       juce::Colour colour,
                                       bool highlighted,
                                       bool showCoordinates = true)
            {
                LayoutPreviewPropertyComponent::Element element;
                element.label = label;
                element.bounds = bounds;
                element.colour = colour;
                element.highlighted = highlighted;
                element.showCoordinates = showCoordinates;
                state.elements.push_back(std::move(element));
            };

            const juce::Colour knobColour = visualOverview().withAlpha(0.82f);
            const juce::Colour sliderColour = visualModulation().withAlpha(0.80f);
            const juce::Colour valueColour = juce::Colour(0xffd4d8de).withAlpha(0.78f);
            const juce::Colour mixColour = visualValidation().withAlpha(0.88f);
            const juce::Colour globalColour = juce::Colour(0xffd6b168).withAlpha(0.82f);

            addElement("Rate Knob", { static_cast<float>(rateCenterX - mainKnobSize / 2), static_cast<float>(knobTopY),
                                      static_cast<float>(mainKnobSize), static_cast<float>(mainKnobSize) },
                       knobColour, emphasizeEngineLayout);
            addElement("Depth Knob", { static_cast<float>(depthCenterX - mainKnobSize / 2), static_cast<float>(knobTopY),
                                       static_cast<float>(mainKnobSize), static_cast<float>(mainKnobSize) },
                       knobColour, emphasizeEngineLayout);
            addElement("Offset Knob", { static_cast<float>(offsetCenterX - mainKnobSize / 2), static_cast<float>(knobTopY),
                                        static_cast<float>(mainKnobSize), static_cast<float>(mainKnobSize) },
                       knobColour, emphasizeEngineLayout);
            addElement("Width Knob", { static_cast<float>(widthCenterX - mainKnobSize / 2), static_cast<float>(knobTopY),
                                       static_cast<float>(mainKnobSize), static_cast<float>(mainKnobSize) },
                       knobColour, emphasizeEngineLayout);
            addElement("Color Slider", { static_cast<float>(sliderX), static_cast<float>(sliderY),
                                         static_cast<float>(sliderW), static_cast<float>(sliderH) },
                       sliderColour, emphasizeEngineLayout);
            addElement("Mix Knob", { static_cast<float>(mixCenterX - mixKnobSize / 2), static_cast<float>(mixKnobY),
                                     static_cast<float>(mixKnobSize), static_cast<float>(mixKnobSize) },
                       mixColour, emphasizeEngineLayout);

            addElement("Rate Value", { static_cast<float>(rateCenterX - layout.valueLabelWidth / 2 + rateValueOffsetX),
                                       static_cast<float>(valueLabelY + layout.rateValueOffsetY),
                                       static_cast<float>(layout.valueLabelWidth), static_cast<float>(layout.valueLabelHeight) },
                       valueColour, emphasizeEngineLayout, false);
            addElement("Depth Value", { static_cast<float>(depthCenterX - layout.valueLabelWidth / 2 + depthValueOffsetX),
                                        static_cast<float>(valueLabelY + layout.depthValueOffsetY),
                                        static_cast<float>(layout.valueLabelWidth), static_cast<float>(layout.valueLabelHeight) },
                       valueColour, emphasizeEngineLayout, false);
            addElement("Offset Value", { static_cast<float>(offsetCenterX - layout.valueLabelWidth / 2 + offsetValueOffsetX),
                                         static_cast<float>(valueLabelY + layout.offsetValueOffsetY),
                                         static_cast<float>(layout.valueLabelWidth), static_cast<float>(layout.valueLabelHeight) },
                       valueColour, emphasizeEngineLayout, false);
            addElement("Width Value", { static_cast<float>(widthCenterX - layout.valueLabelWidth / 2 + widthValueOffsetX),
                                        static_cast<float>(valueLabelY + layout.widthValueOffsetY),
                                        static_cast<float>(layout.valueLabelWidth), static_cast<float>(layout.valueLabelHeight) },
                       valueColour, emphasizeEngineLayout, false);

            const int colorValueX = layout.colorValueCenterX - (layout.colorValueWidth / 2) + colorValueXOffset;
            addElement("Color Value", { static_cast<float>(colorValueX), static_cast<float>(colorValueY),
                                        static_cast<float>(layout.colorValueWidth), static_cast<float>(layout.colorValueHeight) },
                       valueColour, emphasizeEngineLayout, false);
            addElement("Mix Value", { static_cast<float>(mixCenterX - layout.mixValueWidth / 2 + mixValueOffsetX),
                                      static_cast<float>(mixValueY),
                                      static_cast<float>(layout.mixValueWidth), static_cast<float>(layout.mixValueHeight) },
                       valueColour, emphasizeEngineLayout, false);

            const int rightEdge = 700 - layout.topButtonsRightMargin;
            const int topButtonW = layout.topButtonsWidth;
            const int topButtonH = layout.topButtonsHeight;
            const int topButtonGap = layout.topButtonsGap;
            const int topButtonY = layout.topButtonsTopY;
            addElement("Feedback", { static_cast<float>(rightEdge - topButtonW), static_cast<float>(topButtonY),
                                     static_cast<float>(topButtonW), static_cast<float>(topButtonH) },
                       globalColour, emphasizeGlobal);
            addElement("Help", { static_cast<float>(rightEdge - (2 * topButtonW) - topButtonGap), static_cast<float>(topButtonY),
                                 static_cast<float>(topButtonW), static_cast<float>(topButtonH) },
                       globalColour, emphasizeGlobal);
            addElement("About", { static_cast<float>(rightEdge - (3 * topButtonW) - (2 * topButtonGap)), static_cast<float>(topButtonY),
                                  static_cast<float>(topButtonW), static_cast<float>(topButtonH) },
                       globalColour, emphasizeGlobal);

            addElement("Engine Selector", { static_cast<float>(layout.engineSelectorX), static_cast<float>(layout.engineSelectorY),
                                            static_cast<float>(layout.engineSelectorW), static_cast<float>(layout.engineSelectorH) },
                       globalColour, emphasizeGlobal);

            const int hqSize = layout.hqSwitchSize;
            const int hqCenterX = 350 + layout.hqSwitchOffsetX;
            const int hqCenterY = 152 + layout.hqSwitchOffsetY;
            addElement("HQ Switch", { static_cast<float>(hqCenterX - hqSize / 2), static_cast<float>(hqCenterY - hqSize / 2),
                                      static_cast<float>(hqSize), static_cast<float>(hqSize) },
                       globalColour, emphasizeGlobal);

            return state;
        },
        "Live GUI preview for Look & Feel tuning with editable coordinate context.");
    layoutPreviewCard->setPreferredHeight(560);
    liveReadoutProperties.add(layoutPreviewCard);
    lookFeelVisualDeck.addAndMakeVisible(layoutPreviewCard);
    lookFeelVisualDeckCards.add(layoutPreviewCard);
    registerControlMetadata("Look & Feel Live Preview", "layout_preview", "layout_state", "ui_bounds_probe", {});
}
