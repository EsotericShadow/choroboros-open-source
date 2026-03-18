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

#pragma once

#include "../Plugin/PluginProcessor.h"
#include "DevPanelSupport.h"
#include <functional>
#include <vector>

struct DevPanelBuildContext
{
    std::function<juce::PropertyComponent*(const juce::Value&, const juce::String&, double, double, double, double)> makeLockable;
    std::function<juce::PropertyComponent*(const juce::String&, const char*, float, float, float, float, float)> makeLiveMappedControl;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<juce::String()>)> makeReadOnly;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<juce::String()>)> makeMultiLineReadOnly;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<std::vector<float>()>, devpanel::LinkGroup)> makeSparkline;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<devpanel::TransferCurvePropertyComponent::State()>, devpanel::LinkGroup)> makeTransferCurve;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<devpanel::SpectrumOverlayPropertyComponent::State()>, devpanel::LinkGroup)> makeSpectrumOverlay;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<devpanel::SignalFlowPropertyComponent::State()>, devpanel::LinkGroup)> makeSignalFlow;
    std::function<juce::PropertyComponent*(const juce::String&, std::function<devpanel::ValidationTraceMatrixPropertyComponent::State()>)> makeTraceMatrix;
    std::function<float(const char*)> readRawParam;
    std::function<float(const char*, bool&)> getActiveProfileRaw;
    std::function<ChoroborosAudioProcessor::AnalyzerSnapshot()> readAnalyzerSnapshot;
    std::function<void(const juce::String&, const juce::String&, const juce::String&, const juce::String&, const juce::String&)> registerControlMetadata;
};

