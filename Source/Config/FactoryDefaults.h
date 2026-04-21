#pragma once

#include <juce_core/juce_core.h>
#include "../DSP/CoreAssignments.h"
#include <optional>

namespace choroboros::factory
{
struct EngineProfile
{
    bool valid = false;
    float rate = 0.5f;
    float depth = 0.5f;
    float offset = 90.0f;
    float width = 1.0f;
    float mix = 0.5f;
    float color = 0.5f;
};

struct PresetDefinition
{
    juce::String name;
    float rate = 0.5f;
    float depth = 0.5f;
    float offset = 90.0f;
    float width = 1.0f;
    float color = 0.5f;
    float mix = 0.5f;
    bool hqEnabled = false;
    int engineColorIndex = 0;
    bool modularCoresEnabled = false;
    choroboros::CoreAssignmentTable coreAssignments;
};

std::optional<EngineProfile> getEngineProfile(int engineIndex);
int getPresetCount();
juce::String getPresetName(int index);
std::optional<PresetDefinition> getPreset(int index);
} // namespace choroboros::factory
