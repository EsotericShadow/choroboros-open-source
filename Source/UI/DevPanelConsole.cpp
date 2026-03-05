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
#include "../Plugin/PluginEditor.h"
#include "../Plugin/PluginProcessor.h"
#include "../Config/DefaultsPersistence.h"
#include "DevPanelSupport.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace devpanel;

namespace
{
constexpr double kConsoleValueEpsilon = 1.0e-6;

juce::String formatConsoleValue(double value, int decimals = 6)
{
    juce::String text(value, juce::jlimit(0, 8, decimals));
    if (text.containsChar('.'))
    {
        while (text.endsWithChar('0'))
            text = text.dropLastCharacters(1);
        if (text.endsWithChar('.'))
            text = text.dropLastCharacters(1);
    }
    if (text.isEmpty())
        return "0";
    return text;
}

bool parseConsoleDouble(const juce::String& token, double& outValue)
{
    const juce::String trimmed = token.trim();
    if (trimmed.isEmpty())
        return false;
    auto utf8 = trimmed.toRawUTF8();
    char* endPtr = nullptr;
    const double parsed = std::strtod(utf8, &endPtr);
    if (endPtr == utf8 || (endPtr != nullptr && *endPtr != '\0'))
        return false;
    outValue = parsed;
    return std::isfinite(parsed);
}

juce::String slugifyParameterName(const juce::String& input)
{
    juce::String cleaned = input.toLowerCase();
    cleaned = cleaned.replace("&", " ");
    cleaned = cleaned.replaceCharacters("()[]{}<>:/\\,+-*=%#'\"`|", "                              ");

    juce::StringArray tokens;
    tokens.addTokens(cleaned, " \t\r\n", "");
    tokens.trim();
    tokens.removeEmptyStrings();

    const juce::StringArray unitTokens { "hz", "khz", "ms", "db", "dbfs", "deg", "px", "pct", "percent",
                                         "seconds", "second", "secs", "sec", "samples", "sample", "s" };

    juce::StringArray outputTokens;
    outputTokens.ensureStorageAllocated(tokens.size());
    for (const auto& token : tokens)
    {
        if (unitTokens.contains(token))
            continue;
        juce::String tokenClean = token.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");
        if (tokenClean.isNotEmpty())
            outputTokens.add(tokenClean);
    }

    if (outputTokens.isEmpty())
        return "param";
    return outputTokens.joinIntoString("_");
}

juce::String canonicalSlugForList(const juce::String& slug)
{
    const int underscore = slug.lastIndexOfChar('_');
    if (underscore <= 0 || underscore >= slug.length() - 1)
        return slug;

    const juce::String tail = slug.substring(underscore + 1);
    if (!tail.containsOnly("0123456789"))
        return slug;
    return slug.substring(0, underscore);
}

int parseEngineIndexToken(const juce::String& token)
{
    const auto lower = token.trim().toLowerCase();
    if (lower == "green") return 0;
    if (lower == "blue") return 1;
    if (lower == "red") return 2;
    if (lower == "purple") return 3;
    if (lower == "black") return 4;
    return -1;
}

juce::String engineNameForIndex(int engineIndex)
{
    static const juce::String names[] { "green", "blue", "red", "purple", "black" };
    return names[juce::jlimit(0, 4, engineIndex)];
}

bool parseOnOffToken(const juce::String& token, bool& valueOut)
{
    const auto lower = token.trim().toLowerCase();
    if (lower == "on" || lower == "true" || lower == "1")
    {
        valueOut = true;
        return true;
    }
    if (lower == "off" || lower == "false" || lower == "0")
    {
        valueOut = false;
        return true;
    }
    return false;
}

bool parseModeToken(const juce::String& token, bool& hqEnabledOut)
{
    const auto lower = token.trim().toLowerCase();
    if (lower == "hq")
    {
        hqEnabledOut = true;
        return true;
    }
    if (lower == "nq")
    {
        hqEnabledOut = false;
        return true;
    }
    return false;
}

template <typename Fn>
void forEachRuntimeTuningValue(const ChorusDSP::RuntimeTuning& tuning, Fn&& fn)
{
    fn("rate_smoothing_ms", tuning.rateSmoothingMs.load());
    fn("depth_smoothing_ms", tuning.depthSmoothingMs.load());
    fn("depth_rate_limit", tuning.depthRateLimit.load());
    fn("centre_delay_smoothing_ms", tuning.centreDelaySmoothingMs.load());
    fn("centre_delay_base_ms", tuning.centreDelayBaseMs.load());
    fn("centre_delay_scale", tuning.centreDelayScale.load());
    fn("color_smoothing_ms", tuning.colorSmoothingMs.load());
    fn("width_smoothing_ms", tuning.widthSmoothingMs.load());
    fn("hpf_cutoff_hz", tuning.hpfCutoffHz.load());
    fn("hpf_q", tuning.hpfQ.load());
    fn("lpf_cutoff_hz", tuning.lpfCutoffHz.load());
    fn("lpf_q", tuning.lpfQ.load());
    fn("pre_emphasis_freq_hz", tuning.preEmphasisFreqHz.load());
    fn("pre_emphasis_q", tuning.preEmphasisQ.load());
    fn("pre_emphasis_gain", tuning.preEmphasisGain.load());
    fn("pre_emphasis_level_smoothing", tuning.preEmphasisLevelSmoothing.load());
    fn("pre_emphasis_quiet_threshold", tuning.preEmphasisQuietThreshold.load());
    fn("pre_emphasis_max_amount", tuning.preEmphasisMaxAmount.load());
    fn("compressor_attack_ms", tuning.compressorAttackMs.load());
    fn("compressor_release_ms", tuning.compressorReleaseMs.load());
    fn("compressor_threshold_db", tuning.compressorThresholdDb.load());
    fn("compressor_ratio", tuning.compressorRatio.load());
    fn("saturation_drive_scale", tuning.saturationDriveScale.load());
    fn("green_bloom_exponent", tuning.greenBloomExponent.load());
    fn("green_bloom_depth_scale", tuning.greenBloomDepthScale.load());
    fn("green_bloom_centre_offset_ms", tuning.greenBloomCentreOffsetMs.load());
    fn("green_bloom_cutoff_max_hz", tuning.greenBloomCutoffMaxHz.load());
    fn("green_bloom_cutoff_min_hz", tuning.greenBloomCutoffMinHz.load());
    fn("green_bloom_wet_blend", tuning.greenBloomWetBlend.load());
    fn("green_bloom_gain", tuning.greenBloomGain.load());
    fn("blue_focus_exponent", tuning.blueFocusExponent.load());
    fn("blue_focus_hp_min_hz", tuning.blueFocusHpMinHz.load());
    fn("blue_focus_hp_max_hz", tuning.blueFocusHpMaxHz.load());
    fn("blue_focus_lp_max_hz", tuning.blueFocusLpMaxHz.load());
    fn("blue_focus_lp_min_hz", tuning.blueFocusLpMinHz.load());
    fn("blue_presence_freq_min_hz", tuning.bluePresenceFreqMinHz.load());
    fn("blue_presence_freq_max_hz", tuning.bluePresenceFreqMaxHz.load());
    fn("blue_presence_q_min", tuning.bluePresenceQMin.load());
    fn("blue_presence_q_max", tuning.bluePresenceQMax.load());
    fn("blue_presence_gain_max_db", tuning.bluePresenceGainMaxDb.load());
    fn("blue_focus_wet_blend", tuning.blueFocusWetBlend.load());
    fn("blue_focus_output_gain", tuning.blueFocusOutputGain.load());
    fn("purple_warp_a", tuning.purpleWarpA.load());
    fn("purple_warp_b", tuning.purpleWarpB.load());
    fn("purple_warp_k_base", tuning.purpleWarpKBase.load());
    fn("purple_warp_k_scale", tuning.purpleWarpKScale.load());
    fn("purple_warp_delay_smoothing_ms", tuning.purpleWarpDelaySmoothingMs.load());
    fn("purple_orbit_eccentricity", tuning.purpleOrbitEccentricity.load());
    fn("purple_orbit_theta_rate_base_hz", tuning.purpleOrbitThetaRateBaseHz.load());
    fn("purple_orbit_theta_rate_scale_hz", tuning.purpleOrbitThetaRateScaleHz.load());
    fn("purple_orbit_theta_rate2_ratio", tuning.purpleOrbitThetaRate2Ratio.load());
    fn("purple_orbit_eccentricity2_ratio", tuning.purpleOrbitEccentricity2Ratio.load());
    fn("purple_orbit_mix1", tuning.purpleOrbitMix1.load());
    fn("purple_orbit_stereo_theta_offset", tuning.purpleOrbitStereoThetaOffset.load());
    fn("purple_orbit_delay_smoothing_ms", tuning.purpleOrbitDelaySmoothingMs.load());
    fn("black_nq_depth_base", tuning.blackNqDepthBase.load());
    fn("black_nq_depth_scale", tuning.blackNqDepthScale.load());
    fn("black_nq_delay_glide_ms", tuning.blackNqDelayGlideMs.load());
    fn("black_hq_tap2_mix_base", tuning.blackHqTap2MixBase.load());
    fn("black_hq_tap2_mix_scale", tuning.blackHqTap2MixScale.load());
    fn("black_hq_second_tap_depth_base", tuning.blackHqSecondTapDepthBase.load());
    fn("black_hq_second_tap_depth_scale", tuning.blackHqSecondTapDepthScale.load());
    fn("black_hq_second_tap_delay_offset_base", tuning.blackHqSecondTapDelayOffsetBase.load());
    fn("black_hq_second_tap_delay_offset_scale", tuning.blackHqSecondTapDelayOffsetScale.load());
    fn("bbd_delay_smoothing_ms", tuning.bbdDelaySmoothingMs.load());
    fn("bbd_delay_min_ms", tuning.bbdDelayMinMs.load());
    fn("bbd_delay_max_ms", tuning.bbdDelayMaxMs.load());
    fn("bbd_centre_base_ms", tuning.bbdCentreBaseMs.load());
    fn("bbd_centre_scale", tuning.bbdCentreScale.load());
    fn("bbd_depth_ms", tuning.bbdDepthMs.load());
    fn("bbd_clock_smoothing_ms", tuning.bbdClockSmoothingMs.load());
    fn("bbd_filter_smoothing_ms", tuning.bbdFilterSmoothingMs.load());
    fn("bbd_filter_cutoff_min_hz", tuning.bbdFilterCutoffMinHz.load());
    fn("bbd_filter_cutoff_max_hz", tuning.bbdFilterCutoffMaxHz.load());
    fn("bbd_filter_cutoff_scale", tuning.bbdFilterCutoffScale.load());
    fn("bbd_clock_min_hz", tuning.bbdClockMinHz.load());
    fn("bbd_clock_max_ratio", tuning.bbdClockMaxRatio.load());
    fn("bbd_stages", tuning.bbdStages.load());
    fn("bbd_filter_max_ratio", tuning.bbdFilterMaxRatio.load());
    fn("tape_delay_smoothing_ms", tuning.tapeDelaySmoothingMs.load());
    fn("tape_centre_base_ms", tuning.tapeCentreBaseMs.load());
    fn("tape_centre_scale", tuning.tapeCentreScale.load());
    fn("tape_tone_max_hz", tuning.tapeToneMaxHz.load());
    fn("tape_tone_min_hz", tuning.tapeToneMinHz.load());
    fn("tape_tone_smoothing_coeff", tuning.tapeToneSmoothingCoeff.load());
    fn("tape_drive_scale", tuning.tapeDriveScale.load());
    fn("tape_lfo_ratio_scale", tuning.tapeLfoRatioScale.load());
    fn("tape_lfo_mod_smoothing_coeff", tuning.tapeLfoModSmoothingCoeff.load());
    fn("tape_ratio_smoothing_coeff", tuning.tapeRatioSmoothingCoeff.load());
    fn("tape_phase_damping", tuning.tapePhaseDamping.load());
    fn("tape_wow_freq_base", tuning.tapeWowFreqBase.load());
    fn("tape_wow_freq_spread", tuning.tapeWowFreqSpread.load());
    fn("tape_flutter_freq_base", tuning.tapeFlutterFreqBase.load());
    fn("tape_flutter_freq_spread", tuning.tapeFlutterFreqSpread.load());
    fn("tape_wow_depth_base", tuning.tapeWowDepthBase.load());
    fn("tape_wow_depth_spread", tuning.tapeWowDepthSpread.load());
    fn("tape_flutter_depth_base", tuning.tapeFlutterDepthBase.load());
    fn("tape_flutter_depth_spread", tuning.tapeFlutterDepthSpread.load());
    fn("tape_ratio_min", tuning.tapeRatioMin.load());
    fn("tape_ratio_max", tuning.tapeRatioMax.load());
    fn("tape_wet_gain", tuning.tapeWetGain.load());
    fn("tape_hermite_tension", tuning.tapeHermiteTension.load());
}
} // namespace

void DevPanel::registerConsoleTarget(juce::PropertyComponent* property, const juce::String& name)
{
    auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(property);
    if (lockable == nullptr)
        return;

    ConsoleTargetBinding binding;
    binding.property = property;
    binding.displayName = name.trim();
    binding.baselineValue = lockable->getCurrentValueForCommand();
    binding.lastKnownValue = binding.baselineValue;

    const juce::String lower = binding.displayName.toLowerCase();
    int matchedEngine = -1;
    int matchCount = 0;
    for (int engine = 0; engine < 5; ++engine)
    {
        const juce::String engineName = engineNameForIndex(engine);
        if (lower.contains(engineName))
        {
            matchedEngine = engine;
            ++matchCount;
        }
    }
    if (matchCount == 1)
    {
        binding.engineSpecific = true;
        binding.engineScope = matchedEngine;
    }

    juce::String baseSlug = slugifyParameterName(binding.displayName);
    if (baseSlug.isEmpty())
        baseSlug = "param";

    juce::String uniqueSlug = baseSlug;
    int suffix = 2;
    auto slugExists = [this](const juce::String& slugToFind) -> bool
    {
        for (const auto& item : consoleTargets)
        {
            if (item.slug == slugToFind)
                return true;
        }
        return false;
    };

    while (slugExists(uniqueSlug))
    {
        uniqueSlug = baseSlug + "_" + juce::String(suffix);
        ++suffix;
    }
    binding.slug = uniqueSlug;

    const std::string slugKey = binding.slug.toStdString();
    consoleTargets.push_back(std::move(binding));
    consoleTargetIndexBySlug[slugKey] = consoleTargets.size() - 1;
    consoleListOutputCache.clear();
    consoleFactoryValuesReady = false;
}

void DevPanel::appendRecentTouchLogLine(const juce::String& line) const
{
    if (line.trim().isEmpty() || recentTouchesLogFile == juce::File())
        return;

    if (!recentTouchesLogFile.getParentDirectory().exists())
        recentTouchesLogFile.getParentDirectory().createDirectory();

    if (recentTouchesLogFile.existsAsFile() && recentTouchesLogFile.getSize() > 512 * 1024)
    {
        juce::StringArray lines;
        lines.addLines(recentTouchesLogFile.loadFileAsString());
        while (lines.size() > 240)
            lines.remove(0);
        recentTouchesLogFile.replaceWithText(lines.joinIntoString("\n") + "\n");
    }

    recentTouchesLogFile.appendText(line + "\n", false, false);
}

juce::String DevPanel::buildConsoleWatchHudText() const
{
    if (consoleWatchSlugs.isEmpty())
        return "Watch: (none)";

    juce::StringArray parts;
    const int maxVisible = juce::jmin(6, consoleWatchSlugs.size());
    for (int i = 0; i < maxVisible; ++i)
    {
        const auto& slug = consoleWatchSlugs[i];
        const auto it = std::find_if(consoleTargets.begin(), consoleTargets.end(),
                                     [&slug](const ConsoleTargetBinding& binding)
                                     {
                                         return binding.slug == slug;
                                     });
        if (it == consoleTargets.end())
        {
            parts.add(slug + "=?");
            continue;
        }

        auto* lockable = dynamic_cast<const LockableFloatPropertyComponent*>(it->property);
        if (lockable == nullptr)
        {
            parts.add(slug + "=?");
            continue;
        }
        parts.add(slug + "=" + formatConsoleValue(lockable->getCurrentValueForCommand()));
    }

    juce::String hud = "Watch: " + parts.joinIntoString(" | ");
    if (consoleWatchSlugs.size() > maxVisible)
        hud << " | +" << juce::String(consoleWatchSlugs.size() - maxVisible) << " more";
    return hud;
}

void DevPanel::cancelConsoleSweeps()
{
    consoleSweeps.clear();
}

void DevPanel::updateConsoleSweeps()
{
    if (consoleSweeps.empty())
        return;

    bool anyUpdated = false;
    for (int i = static_cast<int>(consoleSweeps.size()) - 1; i >= 0; --i)
    {
        auto& sweep = consoleSweeps[static_cast<size_t>(i)];
        if (sweep.lockable == nullptr)
        {
            consoleSweeps.erase(consoleSweeps.begin() + i);
            continue;
        }

        ++sweep.tick;
        const double alpha = juce::jlimit(0.0, 1.0,
                                          static_cast<double>(sweep.tick)
                                              / static_cast<double>(juce::jmax(1, sweep.totalTicks)));
        const double value = sweep.startValue + (sweep.endValue - sweep.startValue) * alpha;
        sweep.lockable->setValueFromCommand(value);

        for (auto& binding : consoleTargets)
        {
            if (binding.slug == sweep.slug)
            {
                binding.lastKnownValue = sweep.lockable->getCurrentValueForCommand();
                break;
            }
        }

        anyUpdated = true;
        if (sweep.tick >= sweep.totalTicks)
            consoleSweeps.erase(consoleSweeps.begin() + i);
    }

    if (anyUpdated)
        repaint();
}

ConsoleCommandResult DevPanel::executeConsoleCommand(const juce::String& command)
{
    ConsoleCommandResult result;
    const juce::String trimmed = command.trim();
    if (trimmed.isEmpty())
        return result;

    juce::String parsedCommand = trimmed;
    {
        std::unordered_set<std::string> visitedAliases;
        for (int depth = 0; depth < 8; ++depth)
        {
            const juce::String head = parsedCommand.upToFirstOccurrenceOf(" ", false, false).trim().toLowerCase();
            if (head.isEmpty() || head == "alias")
                break;
            const auto it = consoleAliases.find(head.toStdString());
            if (it == consoleAliases.end())
                break;

            if (!visitedAliases.insert(head.toStdString()).second)
            {
                result.output = "ERROR: alias loop detected while expanding `" + head + "`.";
                return result;
            }

            const juce::String tail = parsedCommand.fromFirstOccurrenceOf(" ", false, false).trim();
            parsedCommand = it->second + (tail.isNotEmpty() ? (" " + tail) : "");
        }
    }

    const juce::String timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S");
    consoleCommandHistory.insert(0, timestamp + "  " + trimmed);
    while (consoleCommandHistory.size() > 240)
        consoleCommandHistory.remove(consoleCommandHistory.size() - 1);

    auto refreshAllPanels = [this]
    {
        auto refreshPanel = [](juce::PropertyPanel& panel) { panel.refreshAll(); };
        refreshPanel(mappingPanel);
        refreshPanel(uiPanel);
        refreshPanel(overviewPanel);
        refreshPanel(modulationPanel);
        refreshPanel(tonePanel);
        refreshPanel(enginePanel);
        refreshPanel(validationPanel);
        refreshPanel(internalsGreenNqPanel);
        refreshPanel(internalsGreenHqPanel);
        refreshPanel(internalsBlueNqPanel);
        refreshPanel(internalsBlueHqPanel);
        refreshPanel(internalsRedNqPanel);
        refreshPanel(internalsRedHqPanel);
        refreshPanel(internalsPurpleNqPanel);
        refreshPanel(internalsPurpleHqPanel);
        refreshPanel(internalsBlackNqPanel);
        refreshPanel(internalsBlackHqPanel);
        refreshPanel(bbdPanel);
        refreshPanel(tapePanel);
        refreshPanel(layoutGlobalPanel);
        refreshPanel(layoutTextAnimationPanel);
        refreshPanel(layoutGreenPanel);
        refreshPanel(layoutBluePanel);
        refreshPanel(layoutRedPanel);
        refreshPanel(layoutPurplePanel);
        refreshPanel(layoutBlackPanel);
    };

    auto refreshAllLockables = [this]
    {
        for (auto* property : lockableProperties)
            if (property != nullptr)
                property->refresh();
    };

    auto syncBindingValue = [](ConsoleTargetBinding& binding, bool trackPrevious)
    {
        auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property);
        if (lockable == nullptr)
            return;
        const double current = lockable->getCurrentValueForCommand();
        if (trackPrevious && std::abs(current - binding.lastKnownValue) > kConsoleValueEpsilon)
        {
            binding.previousValue = binding.lastKnownValue;
            binding.hasPreviousValue = true;
        }
        binding.lastKnownValue = current;
    };

    auto syncAllBindings = [this, &syncBindingValue](bool trackPrevious)
    {
        for (auto& binding : consoleTargets)
            syncBindingValue(binding, trackPrevious);
    };

    struct ConsoleSnapshot
    {
        juce::MemoryBlock processorState;
        LayoutTuning layout;
        int selectedTab = 0;
        std::array<int, 7> selectedSubTabs {};
        bool bypassActive = false;
        float bypassStoredMix = 0.0f;
        bool soloActive = false;
        juce::String soloNode;
        float soloStoredMix = 0.0f;
        std::vector<std::pair<LockableFloatPropertyComponent*, bool>> lockStates;
    };

    auto captureSnapshot = [this]() -> ConsoleSnapshot
    {
        ConsoleSnapshot snapshot;
        processor.getStateInformation(snapshot.processorState);
        snapshot.layout = editor.getLayoutTuning();
        snapshot.selectedTab = selectedRightTab;
        snapshot.selectedSubTabs = selectedSubTabs;
        snapshot.bypassActive = consoleBypassActive;
        snapshot.bypassStoredMix = consoleBypassStoredMixRaw;
        snapshot.soloActive = consoleSoloActive;
        snapshot.soloNode = consoleSoloNode;
        snapshot.soloStoredMix = consoleSoloStoredMixRaw;
        snapshot.lockStates.reserve(static_cast<size_t>(lockableProperties.size()));
        for (auto* property : lockableProperties)
        {
            if (auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(property))
                snapshot.lockStates.emplace_back(lockable, lockable->isLocked());
        }
        return snapshot;
    };

    auto restoreSnapshot = [this, &refreshAllPanels, &refreshAllLockables](const ConsoleSnapshot& snapshot,
                                                                            bool fullUiRefresh = true)
    {
        processor.setStateInformation(snapshot.processorState.getData(),
                                      static_cast<int>(snapshot.processorState.getSize()));
        editor.getLayoutTuning() = snapshot.layout;
        editor.applyLayout();
        selectedRightTab = juce::jlimit(0, 6, snapshot.selectedTab);
        selectedSubTabs = snapshot.selectedSubTabs;
        markLazyUiStateDirty();
        consoleBypassActive = snapshot.bypassActive;
        consoleBypassStoredMixRaw = snapshot.bypassStoredMix;
        consoleSoloActive = snapshot.soloActive;
        consoleSoloNode = snapshot.soloNode;
        consoleSoloStoredMixRaw = snapshot.soloStoredMix;
        for (const auto& lockState : snapshot.lockStates)
        {
            if (lockState.first != nullptr)
                lockState.first->setLocked(lockState.second);
        }

        if (fullUiRefresh)
        {
            refreshAllPanels();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            refreshSecondaryTabButtons();
            resized();
            repaint();
        }
        else
        {
            refreshAllLockables();
        }

        for (auto& binding : consoleTargets)
        {
            if (auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property))
                binding.lastKnownValue = lockable->getCurrentValueForCommand();
        }
    };

    auto pushAction = [this](ConsoleAction action)
    {
        consoleUndoStack.push_back(std::move(action));
        constexpr size_t maxHistory = 256;
        if (consoleUndoStack.size() > maxHistory)
            consoleUndoStack.erase(consoleUndoStack.begin());
        consoleRedoStack.clear();
    };

    auto runSnapshotAction = [this, captureSnapshot, restoreSnapshot, &pushAction](const juce::String& label,
                                                                                    std::function<void()> applyChange)
    {
        cancelConsoleSweeps();
        auto before = std::make_shared<ConsoleSnapshot>(captureSnapshot());
        applyChange();
        // Keep current/previous value tracking coherent for `get` after macro actions.
        for (auto& binding : consoleTargets)
        {
            auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property);
            if (lockable == nullptr)
                continue;
            const double current = lockable->getCurrentValueForCommand();
            if (std::abs(current - binding.lastKnownValue) > kConsoleValueEpsilon)
            {
                binding.previousValue = binding.lastKnownValue;
                binding.hasPreviousValue = true;
                binding.lastKnownValue = current;
            }
        }
        auto after = std::make_shared<ConsoleSnapshot>(captureSnapshot());
        ConsoleAction action;
        action.label = label;
        action.undo = [restoreSnapshot, before]() { restoreSnapshot(*before); };
        action.redo = [restoreSnapshot, after]() { restoreSnapshot(*after); };
        pushAction(std::move(action));
    };

    auto findTarget = [this](const juce::String& userToken) -> ConsoleTargetBinding*
    {
        const juce::String normalized = slugifyParameterName(userToken);
        const auto it = consoleTargetIndexBySlug.find(normalized.toStdString());
        if (it != consoleTargetIndexBySlug.end() && it->second < consoleTargets.size())
            return &consoleTargets[it->second];

        for (auto& binding : consoleTargets)
        {
            if (binding.slug == normalized || binding.slug.equalsIgnoreCase(userToken))
                return &binding;
        }
        return nullptr;
    };

    auto setMappedParameter = [this](const juce::String& paramId, float mappedValue)
    {
        if (auto* param = processor.getValueTreeState().getParameter(paramId))
        {
            const float rawValue = processor.unmapParameterValue(paramId, mappedValue);
            const auto& range = processor.getValueTreeState().getParameterRange(paramId);
            const float normalized = juce::jlimit(0.0f, 1.0f, range.convertTo0to1(rawValue));
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
    };

    auto setRawParameter = [this](const juce::String& paramId, float rawValue)
    {
        if (auto* param = processor.getValueTreeState().getParameter(paramId))
        {
            const auto& range = processor.getValueTreeState().getParameterRange(paramId);
            const float normalized = juce::jlimit(0.0f, 1.0f, range.convertTo0to1(rawValue));
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
    };

    auto ensureFactoryValueCache = [this, &captureSnapshot, &restoreSnapshot, &refreshAllLockables]() -> bool
    {
        if (consoleFactoryValuesReady && !consoleFactoryValues.empty())
            return true;

        const auto snapshot = captureSnapshot();
        processor.resetToFactoryDefaults();
        editor.resetLayoutToFactoryDefaults();
        refreshAllLockables();

        consoleFactoryValues.clear();
        for (auto& binding : consoleTargets)
        {
            if (auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property))
                consoleFactoryValues[binding.slug.toStdString()] = lockable->getCurrentValueForCommand();
        }

        restoreSnapshot(snapshot, false);
        consoleFactoryValuesReady = !consoleFactoryValues.empty();
        return consoleFactoryValuesReady;
    };

    auto resolveNumericTarget = [&](const juce::String& token,
                                    ConsoleTargetBinding*& bindingOut,
                                    LockableFloatPropertyComponent*& lockableOut,
                                    juce::String& errorOut) -> bool
    {
        bindingOut = findTarget(token);
        if (bindingOut == nullptr)
        {
            errorOut = "ERROR: unknown target slug `" + token + "`.";
            return false;
        }
        lockableOut = dynamic_cast<LockableFloatPropertyComponent*>(bindingOut->property);
        if (lockableOut == nullptr)
        {
            errorOut = "ERROR: target `" + bindingOut->slug + "` is not numeric/writable.";
            return false;
        }
        return true;
    };

    auto applyNumericChange = [&](ConsoleTargetBinding& binding,
                                  LockableFloatPropertyComponent& lockable,
                                  double requestedValue,
                                  const juce::String& historyLabel,
                                  juce::String& outputValue,
                                  bool requireUnlocked = true) -> bool
    {
        if (requireUnlocked && lockable.isLocked())
        {
            outputValue = "ERROR: target `" + binding.slug + "` is locked. Use `unlock " + binding.slug + "` first.";
            return false;
        }

        const double oldValue = lockable.getCurrentValueForCommand();
        for (int i = static_cast<int>(consoleSweeps.size()) - 1; i >= 0; --i)
        {
            if (consoleSweeps[static_cast<size_t>(i)].slug == binding.slug)
                consoleSweeps.erase(consoleSweeps.begin() + i);
        }
        lockable.setValueFromCommand(requestedValue);
        const double newValue = lockable.getCurrentValueForCommand();

        binding.previousValue = oldValue;
        binding.hasPreviousValue = true;
        binding.lastKnownValue = newValue;

        if (std::abs(newValue - oldValue) <= kConsoleValueEpsilon)
        {
            outputValue = binding.slug + " unchanged (" + formatConsoleValue(newValue) + ").";
            return true;
        }

        lockable.refresh();

        auto* lockablePtr = &lockable;
        ConsoleAction actionEntry;
        actionEntry.label = historyLabel;
        actionEntry.undo = [lockablePtr, oldValue]()
        {
            if (lockablePtr != nullptr)
                lockablePtr->setValueFromCommand(oldValue);
        };
        actionEntry.redo = [lockablePtr, newValue]()
        {
            if (lockablePtr != nullptr)
                lockablePtr->setValueFromCommand(newValue);
        };
        pushAction(std::move(actionEntry));
        return true;
    };

    auto setMacroPercent = [this](const juce::String& paramId, double percent) -> bool
    {
        auto* param = processor.getValueTreeState().getParameter(paramId);
        if (param == nullptr)
            return false;
        const float normalized = juce::jlimit(0.0f, 1.0f, static_cast<float>(percent * 0.01));
        param->beginChangeGesture();
        param->setValueNotifyingHost(normalized);
        param->endChangeGesture();
        return true;
    };

    juce::StringArray tokens;
    tokens.addTokens(parsedCommand, " \t", "\"'");
    tokens.trim();
    tokens.removeEmptyStrings();
    if (tokens.isEmpty())
        return result;

    const juce::String action = tokens[0].toLowerCase();

    if (action == "help")
    {
        result.output =
            "Engine & Plugin State:\n"
            "  engine <green|blue|red|purple|black>\n"
            "  hq <on|off>\n"
            "  view <overview|modulation|tone|engine|layout|validation|settings>\n"
            "  bypass <on|off>\n"
            "  slot show\n"
            "  slot set <green|blue|red|purple|black> <nq|hq> <core_id>\n"
            "  solo <node>\n"
            "  unsolo\n"
            "Parameter Tuning:\n"
            "  set <target> <value>\n"
            "  add <target> <delta>\n"
            "  sub <target> <delta>\n"
            "  get <target>\n"
            "  reset <target>\n"
            "  reset all\n"
            "  lock <target>\n"
            "  unlock <target>\n"
            "  toggle <target>\n"
            "  macro <rate|depth|offset|width|color|mix> <0..100>\n"
            "  sweep <target> <start> <end> <time_ms>\n"
            "History:\n"
            "  undo [n], redo [n], history\n"
            "Tutorials:\n"
            "  tutorial\n"
            "  tutorial <topic>\n"
            "  tutorial next\n"
            "  tutorial next section\n"
            "  tutorial skip|exit\n"
            "Layout:\n"
            "  fx <0|1|2>\n"
            "Introspection & Utilities:\n"
            "  watch <target>\n"
            "  unwatch <target>\n"
            "  dump <color>\n"
            "  diff factory\n"
            "  search <term>\n"
            "  stats\n"
            "  list <color> [all] [full]\n"
            "  list globals [full]\n"
            "  core list\n"
            "  core show <id>\n"
            "  export script\n"
            "  import script\n"
            "  alias <name> <command>\n"
            "  cp json\n"
            "  save defaults\n"
            "  clear";
        return result;
    }

    if (action == "clear")
    {
        result.clearOutput = true;
        result.output = "Console cleared.";
        return result;
    }

    if (action == "alias")
    {
        if (tokens.size() < 3)
        {
            result.output = "ERROR: usage: alias <name> <command>";
            return result;
        }

        const juce::String aliasName = slugifyParameterName(tokens[1]);
        if (aliasName.isEmpty())
        {
            result.output = "ERROR: alias name is invalid.";
            return result;
        }
        if (aliasName == "alias")
        {
            result.output = "ERROR: alias name cannot be `alias`.";
            return result;
        }

        const juce::String commandBody = parsedCommand.fromFirstOccurrenceOf(tokens[1], false, false)
                                              .trim()
                                              .fromFirstOccurrenceOf(" ", false, false)
                                              .trim();
        if (commandBody.isEmpty())
        {
            result.output = "ERROR: alias command body cannot be empty.";
            return result;
        }

        consoleAliases[aliasName.toStdString()] = commandBody;
        result.output = "Alias `" + aliasName + "` => `" + commandBody + "`";
        return result;
    }

    if (action == "cp" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("json"))
    {
        juce::SystemClipboard::copyTextToClipboard(buildJson());
        result.output = "Copied current Dev Panel state JSON to clipboard.";
        return result;
    }

    if (action == "save" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("defaults"))
    {
        saveCurrentAsDefaults();
        result.output = "Saved current state as user defaults.";
        return result;
    }

    if (action == "tutorial")
    {
        if (tokens.size() >= 2)
        {
            const juce::String op = tokens[1].toLowerCase();
            if (op == "exit" || op == "skip")
            {
                juce::String status;
                stopTutorial(true, status);
                result.output = status;
                return result;
            }

            if (op == "next" || op == "section" || op == "tab")
            {
                if (!tutorialActive)
                {
                    result.output = "No active tutorial. Start one with `tutorial` or `tutorial <topic>`.";
                    return result;
                }

                const bool advanceSection = (op == "section" || op == "tab")
                                         || (tokens.size() >= 3
                                             && (tokens[2].equalsIgnoreCase("section")
                                                 || tokens[2].equalsIgnoreCase("tab")));

                if (advanceSection)
                    advanceTutorialSection();
                else
                    advanceTutorialStep();

                if (!tutorialActive)
                {
                    result.output = "Tutorial complete.";
                    return result;
                }

                const int total = static_cast<int>(tutorialSteps.size());
                const int oneBased = juce::jlimit(1, juce::jmax(1, total), tutorialStepIndex + 1);
                juce::String title;
                if (juce::isPositiveAndBelow(tutorialStepIndex, total))
                    title = tutorialSteps[static_cast<size_t>(tutorialStepIndex)].title;
                result.output = juce::String(advanceSection ? "Skipped to next section: " : "Advanced to next step: ")
                              + "Step " + juce::String(oneBased) + "/" + juce::String(total)
                              + (title.isNotEmpty() ? (" - " + title) : ".");
                return result;
            }
        }

        const juce::String topic = tokens.size() >= 2 ? tokens[1] : "core";
        juce::String status;
        if (!startTutorial(topic, status))
        {
            result.output = status.isNotEmpty() ? status : ("ERROR: unable to start tutorial `" + topic + "`.");
            return result;
        }
        result.output = status;
        return result;
    }

    if (action == "core")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: core <list|show>";
            return result;
        }

        const juce::String op = tokens[1].toLowerCase();
        if (op == "list")
        {
            juce::StringArray lines;
            lines.add("Assignable cores:");
            const auto& descriptors = ChorusDSP::getCorePackageDescriptors();
            for (const auto& descriptor : descriptors)
            {
                lines.add("  " + juce::String(descriptor.token)
                          + "  [" + juce::String(descriptor.displayName) + "]"
                          + "  macros=" + juce::String(descriptor.macroLabel));
            }
            lines.add("Duplicate policy: warn but allow.");
            result.output = lines.joinIntoString("\n");
            return result;
        }

        if (op == "show")
        {
            if (tokens.size() < 3)
            {
                result.output = "ERROR: usage: core show <core_id>";
                return result;
            }

            choroboros::CoreId coreId = choroboros::CoreId::lagrange3;
            const juce::String token = tokens[2].trim().toLowerCase();
            bool parsed = choroboros::parseCoreIdToken(token.toStdString(), coreId);
            if (!parsed && token.containsOnly("0123456789"))
            {
                const int idx = token.getIntValue();
                if (idx >= 0 && idx < static_cast<int>(choroboros::coreIdCount()))
                {
                    coreId = static_cast<choroboros::CoreId>(idx);
                    parsed = true;
                }
            }
            if (!parsed)
            {
                result.output = "ERROR: unknown core id `" + tokens[2] + "`. Use `core list`.";
                return result;
            }

            const auto& descriptor = choroboros::descriptorForCore(coreId);
            const auto assignments = choroboros::findAssignmentsForCore(processor.getCoreAssignments(), coreId);
            juce::StringArray lines;
            lines.add("Core: " + juce::String(descriptor.displayName) + " (" + juce::String(descriptor.token) + ")");
            lines.add("  macro_label=" + juce::String(descriptor.macroLabel));
            lines.add("  semantics=" + juce::String(descriptor.macroSemantics));
            lines.add("  notes=" + juce::String(descriptor.notes));
            lines.add("  assignments=" + juce::String(static_cast<int>(assignments.size())));
            if (assignments.empty())
            {
                lines.add("  (unassigned)");
            }
            else
            {
                for (const auto& slot : assignments)
                {
                    lines.add("  - " + engineNameForIndex(slot.engineColor)
                              + " " + juce::String(slot.hqEnabled ? "hq" : "nq"));
                }
            }
            result.output = lines.joinIntoString("\n");
            return result;
        }

        result.output = "ERROR: usage: core <list|show>";
        return result;
    }

    if (action == "slot")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: slot <show|set>";
            return result;
        }

        const juce::String op = tokens[1].toLowerCase();
        if (op == "show")
        {
            const auto& assignments = processor.getCoreAssignments();
            const int activeEngine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
            const bool activeHq = processor.isHqEnabled();
            juce::StringArray lines;
            lines.add("Slot assignments (modular cores " + juce::String(processor.isModularCoresEnabled() ? "on" : "off") + "):");
            for (int engine = 0; engine < choroboros::kEngineColorCount; ++engine)
            {
                for (int mode = 0; mode < choroboros::kEngineModeCount; ++mode)
                {
                    const bool hq = mode == 1;
                    const auto coreId = assignments.get(engine, hq);
                    const bool activeSlot = (engine == activeEngine && hq == activeHq);
                    lines.add("  " + juce::String(activeSlot ? "* " : "  ")
                              + engineNameForIndex(engine)
                              + " " + juce::String(hq ? "hq" : "nq")
                              + " -> " + juce::String(choroboros::coreIdToToken(coreId))
                              + " [" + juce::String(choroboros::coreIdToDisplayName(coreId)) + "]");
                }
            }
            const auto dupes = processor.getDuplicateAssignmentWarnings();
            if (!dupes.empty())
                lines.add("WARNING: " + juce::String(static_cast<int>(dupes.size())) + " duplicated slot assignments currently active (allowed).");
            result.output = lines.joinIntoString("\n");
            return result;
        }

        if (op == "set")
        {
            if (tokens.size() < 5)
            {
                result.output = "ERROR: usage: slot set <green|blue|red|purple|black> <nq|hq> <core_id>";
                return result;
            }

            const int engineIndex = parseEngineIndexToken(tokens[2]);
            if (engineIndex < 0)
            {
                result.output = "ERROR: invalid engine. Use: green, blue, red, purple, black.";
                return result;
            }

            bool hqEnabled = false;
            if (!parseModeToken(tokens[3], hqEnabled))
            {
                result.output = "ERROR: invalid mode. Use nq or hq.";
                return result;
            }

            choroboros::CoreId coreId = choroboros::CoreId::lagrange3;
            const juce::String coreToken = tokens[4].trim().toLowerCase();
            bool parsed = choroboros::parseCoreIdToken(coreToken.toStdString(), coreId);
            if (!parsed && coreToken.containsOnly("0123456789"))
            {
                const int idx = coreToken.getIntValue();
                if (idx >= 0 && idx < static_cast<int>(choroboros::coreIdCount()))
                {
                    coreId = static_cast<choroboros::CoreId>(idx);
                    parsed = true;
                }
            }
            if (!parsed)
            {
                result.output = "ERROR: unknown core id `" + tokens[4] + "`. Use `core list`.";
                return result;
            }

            const bool duplicate = choroboros::assignmentIsDuplicate(processor.getCoreAssignments(), engineIndex, hqEnabled, coreId);
            runSnapshotAction("slot set " + engineNameForIndex(engineIndex) + " " + (hqEnabled ? "hq" : "nq"),
                              [this, engineIndex, hqEnabled, coreId]
            {
                processor.setCoreAssignment(engineIndex, hqEnabled, coreId);
                updateActiveProfileLabel();
                updateRightTabVisibility();
                resized();
                repaint();
            });

            result.output = juce::String(duplicate ? "WARNING: duplicate core assignment applied: " : "Applied: ")
                          + engineNameForIndex(engineIndex) + " " + juce::String(hqEnabled ? "hq" : "nq")
                          + " -> " + juce::String(choroboros::coreIdToToken(coreId))
                          + " [" + juce::String(choroboros::coreIdToDisplayName(coreId)) + "]";
            return result;
        }

        result.output = "ERROR: usage: slot <show|set>";
        return result;
    }

    if (action == "engine")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: engine <green|blue|red|purple|black>";
            return result;
        }

        const int engineIndex = parseEngineIndexToken(tokens[1]);
        if (engineIndex < 0)
        {
            result.output = "ERROR: invalid engine. Use: green, blue, red, purple, black.";
            return result;
        }

        runSnapshotAction("engine " + engineNameForIndex(engineIndex), [this, engineIndex]
        {
            if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
            {
                const auto& range = processor.getValueTreeState().getParameterRange(ChoroborosAudioProcessor::ENGINE_COLOR_ID);
                const float normalized = range.convertTo0to1(static_cast<float>(engineIndex));
                param->beginChangeGesture();
                param->setValueNotifyingHost(normalized);
                param->endChangeGesture();
            }
            updateActiveProfileLabel();
            updateRightTabVisibility();
            resized();
            repaint();
        });

        result.output = "Engine switched to " + engineNameForIndex(engineIndex) + ".";
        return result;
    }

    if (action == "hq")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: hq <on|off>";
            return result;
        }

        bool hqEnabled = false;
        if (!parseOnOffToken(tokens[1], hqEnabled))
        {
            result.output = "ERROR: usage: hq <on|off>";
            return result;
        }

        runSnapshotAction("hq " + juce::String(hqEnabled ? "on" : "off"), [this, hqEnabled]
        {
            if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::HQ_ID))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(hqEnabled ? 1.0f : 0.0f);
                param->endChangeGesture();
            }
            updateActiveProfileLabel();
            updateRightTabVisibility();
            resized();
            repaint();
        });

        result.output = "HQ " + juce::String(hqEnabled ? "enabled." : "disabled.");
        return result;
    }

    if (action == "view")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: view <tab_name>";
            return result;
        }

        const juce::String tabToken = tokens[1].toLowerCase();
        int targetTab = -1;
        if (tabToken == "overview") targetTab = 0;
        else if (tabToken == "modulation" || tabToken == "internals") targetTab = 1;
        else if (tabToken == "tone" || tabToken == "bbd") targetTab = 2;
        else if (tabToken == "engine" || tabToken == "tape") targetTab = 3;
        else if (tabToken == "layout" || tabToken == "look" || tabToken == "lookfeel") targetTab = 4;
        else if (tabToken == "validation") targetTab = 5;
        else if (tabToken == "settings") targetTab = 6;

        if (targetTab < 0)
        {
            result.output = "ERROR: unsupported tab. Try: overview, modulation, tone, engine, layout, validation, settings.";
            return result;
        }

        runSnapshotAction("view " + tabToken, [this, targetTab]
        {
            selectedRightTab = targetTab;
            selectedSubTabs[static_cast<size_t>(targetTab)] =
                juce::jlimit(0, juce::jmax(0, getSubTabCount(targetTab) - 1),
                             selectedSubTabs[static_cast<size_t>(targetTab)]);
            markLazyUiStateDirty();
            refreshSecondaryTabButtons();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            resized();
            repaint();
        });

        result.output = "Switched to view: " + juce::String(tabToken) + ".";
        return result;
    }

    if (action == "bypass")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: bypass <on|off>";
            return result;
        }

        bool shouldBypass = false;
        if (!parseOnOffToken(tokens[1], shouldBypass))
        {
            result.output = "ERROR: usage: bypass <on|off>";
            return result;
        }

        runSnapshotAction("bypass " + juce::String(shouldBypass ? "on" : "off"), [this, shouldBypass, &setMappedParameter, &setRawParameter, &refreshAllPanels]
        {
            if (shouldBypass)
            {
                if (auto* raw = processor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::MIX_ID))
                    consoleBypassStoredMixRaw = raw->load();
                setMappedParameter(ChoroborosAudioProcessor::MIX_ID, 0.0f);
                consoleBypassActive = true;
            }
            else
            {
                setRawParameter(ChoroborosAudioProcessor::MIX_ID, consoleBypassStoredMixRaw);
                consoleBypassActive = false;
            }
            refreshAllPanels();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            repaint();
        });

        result.output = shouldBypass
            ? "Bypass enabled (dry path via Mix=0)."
            : "Bypass disabled (restored previous Mix value).";
        return result;
    }

    if (action == "add" || action == "sub")
    {
        if (tokens.size() < 3)
        {
            result.output = "ERROR: usage: " + action + " <target> <delta>";
            return result;
        }

        ConsoleTargetBinding* binding = nullptr;
        LockableFloatPropertyComponent* lockable = nullptr;
        juce::String errorText;
        if (!resolveNumericTarget(tokens[1], binding, lockable, errorText))
        {
            result.output = errorText;
            return result;
        }

        double delta = 0.0;
        if (!parseConsoleDouble(tokens[2], delta))
        {
            result.output = "ERROR: invalid numeric delta `" + tokens[2] + "`.";
            return result;
        }
        if (action == "sub")
            delta = -delta;

        const double current = lockable->getCurrentValueForCommand();
        juce::String outcome;
        if (!applyNumericChange(*binding, *lockable, current + delta, action + " " + binding->slug, outcome))
        {
            result.output = outcome;
            return result;
        }
        if (outcome.startsWithIgnoreCase("ERROR:") || outcome.containsIgnoreCase("unchanged"))
            result.output = outcome;
        else
            result.output = "Set " + binding->slug + " = " + formatConsoleValue(lockable->getCurrentValueForCommand()) + ".";
        return result;
    }

    if (action == "set")
    {
        if (tokens.size() < 3)
        {
            result.output = "ERROR: usage: set <target> <value>";
            return result;
        }

        ConsoleTargetBinding* binding = nullptr;
        LockableFloatPropertyComponent* lockable = nullptr;
        juce::String errorText;
        if (!resolveNumericTarget(tokens[1], binding, lockable, errorText))
        {
            result.output = errorText;
            return result;
        }

        double requestedValue = 0.0;
        if (!parseConsoleDouble(tokens[2], requestedValue))
        {
            result.output = "ERROR: invalid numeric value `" + tokens[2] + "`.";
            return result;
        }

        juce::String outcome;
        if (!applyNumericChange(*binding, *lockable, requestedValue, "set " + binding->slug, outcome))
        {
            result.output = outcome;
            return result;
        }
        if (outcome.startsWithIgnoreCase("ERROR:") || outcome.containsIgnoreCase("unchanged"))
        {
            result.output = outcome;
            return result;
        }
        result.output = "Set " + binding->slug + " = "
                      + formatConsoleValue(lockable->getCurrentValueForCommand()) + ".";
        return result;
    }

    if (action == "get")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: get <target>";
            return result;
        }

        auto* binding = findTarget(tokens[1]);
        if (binding == nullptr)
        {
            result.output = "ERROR: unknown target slug `" + tokens[1] + "`.";
            return result;
        }

        auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding->property);
        if (lockable == nullptr)
        {
            result.output = "ERROR: target `" + binding->slug + "` is not numeric.";
            return result;
        }

        syncBindingValue(*binding, true);
        const juce::String prevText = binding->hasPreviousValue
            ? formatConsoleValue(binding->previousValue)
            : "n/a";
        result.output = binding->slug + " = " + formatConsoleValue(binding->lastKnownValue)
                      + " (previous: " + prevText + ")";
        return result;
    }

    if (action == "toggle")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: toggle <target>";
            return result;
        }

        const juce::String targetToken = tokens[1].toLowerCase();
        if (targetToken == "hq")
        {
            const bool next = !processor.isHqEnabled();
            runSnapshotAction("toggle hq", [this, next]
            {
                if (auto* param = processor.getValueTreeState().getParameter(ChoroborosAudioProcessor::HQ_ID))
                {
                    param->beginChangeGesture();
                    param->setValueNotifyingHost(next ? 1.0f : 0.0f);
                    param->endChangeGesture();
                }
                updateActiveProfileLabel();
                updateRightTabVisibility();
                resized();
                repaint();
            });
            result.output = "HQ " + juce::String(next ? "enabled." : "disabled.");
            return result;
        }
        if (targetToken == "bypass")
        {
            const bool next = !consoleBypassActive;
            runSnapshotAction("toggle bypass", [this, next, &setMappedParameter, &setRawParameter, &refreshAllPanels]
            {
                if (next)
                {
                    if (auto* raw = processor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::MIX_ID))
                        consoleBypassStoredMixRaw = raw->load();
                    setMappedParameter(ChoroborosAudioProcessor::MIX_ID, 0.0f);
                    consoleBypassActive = true;
                }
                else
                {
                    setRawParameter(ChoroborosAudioProcessor::MIX_ID, consoleBypassStoredMixRaw);
                    consoleBypassActive = false;
                }
                refreshAllPanels();
                updateActiveProfileLabel();
                updateRightTabVisibility();
                repaint();
            });
            result.output = "Bypass " + juce::String(next ? "enabled." : "disabled.");
            return result;
        }

        ConsoleTargetBinding* binding = nullptr;
        LockableFloatPropertyComponent* lockable = nullptr;
        juce::String errorText;
        if (!resolveNumericTarget(tokens[1], binding, lockable, errorText))
        {
            result.output = errorText;
            return result;
        }

        const double minValue = lockable->getMinimumForCommand();
        const double maxValue = lockable->getMaximumForCommand();
        const double current = lockable->getCurrentValueForCommand();
        const double midpoint = (minValue + maxValue) * 0.5;
        const double toggled = current >= midpoint ? minValue : maxValue;
        juce::String outcome;
        if (!applyNumericChange(*binding, *lockable, toggled, "toggle " + binding->slug, outcome))
        {
            result.output = outcome;
            return result;
        }
        if (outcome.startsWithIgnoreCase("ERROR:"))
        {
            result.output = outcome;
            return result;
        }
        result.output = "Toggled " + binding->slug + " -> "
                      + formatConsoleValue(lockable->getCurrentValueForCommand()) + ".";
        return result;
    }

    if (action == "macro")
    {
        if (tokens.size() < 3)
        {
            result.output = "ERROR: usage: macro <rate|depth|offset|width|color|mix> <0..100>";
            return result;
        }

        const juce::String macroName = tokens[1].toLowerCase();
        juce::String paramId;
        if (macroName == "rate") paramId = ChoroborosAudioProcessor::RATE_ID;
        else if (macroName == "depth") paramId = ChoroborosAudioProcessor::DEPTH_ID;
        else if (macroName == "offset") paramId = ChoroborosAudioProcessor::OFFSET_ID;
        else if (macroName == "width") paramId = ChoroborosAudioProcessor::WIDTH_ID;
        else if (macroName == "color") paramId = ChoroborosAudioProcessor::COLOR_ID;
        else if (macroName == "mix") paramId = ChoroborosAudioProcessor::MIX_ID;
        else
        {
            result.output = "ERROR: unknown macro `" + macroName + "`.";
            return result;
        }

        double percent = 0.0;
        if (!parseConsoleDouble(tokens[2], percent))
        {
            result.output = "ERROR: macro value must be numeric (0..100).";
            return result;
        }
        percent = juce::jlimit(0.0, 100.0, percent);

        runSnapshotAction("macro " + macroName, [&, percent]
        {
            setMacroPercent(paramId, percent);
            refreshAllPanels();
            updateActiveProfileLabel();
            updateRightTabVisibility();
            repaint();
        });

        if (auto* raw = processor.getValueTreeState().getRawParameterValue(paramId))
        {
            const double mapped = processor.mapParameterValue(paramId, raw->load());
            result.output = "Macro " + macroName + " set to " + formatConsoleValue(percent, 2)
                          + "% (mapped=" + formatConsoleValue(mapped) + ").";
        }
        else
        {
            result.output = "Macro " + macroName + " set to " + formatConsoleValue(percent, 2) + "%.";
        }
        return result;
    }

    if (action == "solo")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: solo <node>";
            return result;
        }

        const juce::String node = tokens[1].toLowerCase();
        const bool dryProbe = (node == "dry" || node == "input");
        const bool wetProbe = (node == "wet" || node == "output"
                               || node == "bbd_left" || node == "bbd_right"
                               || node == "tape_saturator" || node == "chorus");

        runSnapshotAction("solo " + node, [this, node, dryProbe, &setMappedParameter, &refreshAllPanels]
        {
            if (!consoleSoloActive)
            {
                if (auto* raw = processor.getValueTreeState().getRawParameterValue(ChoroborosAudioProcessor::MIX_ID))
                    consoleSoloStoredMixRaw = raw->load();
            }
            consoleSoloActive = true;
            consoleSoloNode = node;
            if (dryProbe)
                setMappedParameter(ChoroborosAudioProcessor::MIX_ID, 0.0f);
            else
                setMappedParameter(ChoroborosAudioProcessor::MIX_ID, 1.0f);
            refreshAllPanels();
            updateRightTabVisibility();
            repaint();
        });

        if (!dryProbe && !wetProbe)
            result.output = "Solo node `" + node + "` routed via wet probe fallback (node-level solo routing is not exposed in current DSP graph).";
        else
            result.output = "Solo probe active: " + node + ".";
        return result;
    }

    if (action == "unsolo")
    {
        if (!consoleSoloActive)
        {
            result.output = "No active solo probe.";
            return result;
        }

        runSnapshotAction("unsolo", [this, &setRawParameter, &refreshAllPanels]
        {
            setRawParameter(ChoroborosAudioProcessor::MIX_ID, consoleSoloStoredMixRaw);
            consoleSoloActive = false;
            consoleSoloNode.clear();
            refreshAllPanels();
            updateRightTabVisibility();
            repaint();
        });
        result.output = "Solo probe disabled; restored previous mix.";
        return result;
    }

    if (action == "sweep")
    {
        if (tokens.size() < 5)
        {
            result.output = "ERROR: usage: sweep <target> <start> <end> <time_ms>";
            return result;
        }

        ConsoleTargetBinding* binding = nullptr;
        LockableFloatPropertyComponent* lockable = nullptr;
        juce::String errorText;
        if (!resolveNumericTarget(tokens[1], binding, lockable, errorText))
        {
            result.output = errorText;
            return result;
        }
        if (lockable->isLocked())
        {
            result.output = "ERROR: target `" + binding->slug + "` is locked. Use `unlock " + binding->slug + "` first.";
            return result;
        }

        double startValue = 0.0;
        double endValue = 0.0;
        double timeMs = 0.0;
        if (!parseConsoleDouble(tokens[2], startValue)
            || !parseConsoleDouble(tokens[3], endValue)
            || !parseConsoleDouble(tokens[4], timeMs))
        {
            result.output = "ERROR: sweep arguments must be numeric.";
            return result;
        }
        timeMs = juce::jlimit(20.0, 600000.0, timeMs);

        for (int i = static_cast<int>(consoleSweeps.size()) - 1; i >= 0; --i)
        {
            if (consoleSweeps[static_cast<size_t>(i)].slug == binding->slug)
                consoleSweeps.erase(consoleSweeps.begin() + i);
        }

        const double preSweepValue = lockable->getCurrentValueForCommand();
        lockable->setValueFromCommand(startValue);
        binding->previousValue = preSweepValue;
        binding->hasPreviousValue = true;
        binding->lastKnownValue = lockable->getCurrentValueForCommand();

        ConsoleSweepState sweep;
        sweep.lockable = lockable;
        sweep.slug = binding->slug;
        sweep.startValue = lockable->getCurrentValueForCommand();
        sweep.endValue = endValue;
        sweep.preSweepValue = preSweepValue;
        sweep.totalTicks = juce::jmax(1, static_cast<int>(std::round((timeMs / 1000.0) * static_cast<double>(kDevPanelTimerHz))));
        sweep.tick = 0;
        consoleSweeps.push_back(sweep);

        ConsoleAction historyAction;
        historyAction.label = "sweep " + binding->slug;
        historyAction.undo = [lockable, preSweepValue]() { lockable->setValueFromCommand(preSweepValue); };
        historyAction.redo = [lockable, endValue]() { lockable->setValueFromCommand(endValue); };
        pushAction(std::move(historyAction));

        result.output = "Sweep started for " + binding->slug + ": "
                      + formatConsoleValue(sweep.startValue) + " -> "
                      + formatConsoleValue(endValue) + " over "
                      + formatConsoleValue(timeMs, 1) + " ms.";
        return result;
    }

    if (action == "reset")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: reset <target|all>";
            return result;
        }

        if (tokens[1].equalsIgnoreCase("all"))
        {
            runSnapshotAction("reset all", [this, &refreshAllPanels]
            {
                processor.resetToFactoryDefaults();
                editor.resetLayoutToFactoryDefaults();
                consoleBypassActive = false;
                consoleSoloActive = false;
                consoleSoloNode.clear();
                refreshAllPanels();
                updateActiveProfileLabel();
                updateRightTabVisibility();
                resized();
                repaint();
            });
            result.output = "Reset all tuning, internals, and layout values to factory defaults.";
            return result;
        }

        auto* binding = findTarget(tokens[1]);
        if (binding == nullptr)
        {
            result.output = "ERROR: unknown target slug `" + tokens[1] + "`.";
            return result;
        }

        auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding->property);
        if (lockable == nullptr)
        {
            result.output = "ERROR: target `" + binding->slug + "` is not writable.";
            return result;
        }
        if (lockable->isLocked())
        {
            result.output = "ERROR: target `" + binding->slug + "` is locked. Use `unlock " + binding->slug + "` first.";
            return result;
        }

        if (!ensureFactoryValueCache())
        {
            result.output = "ERROR: unable to resolve factory values for target reset.";
            return result;
        }

        const auto factoryIt = consoleFactoryValues.find(binding->slug.toStdString());
        if (factoryIt == consoleFactoryValues.end())
        {
            result.output = "ERROR: no cached factory value for `" + binding->slug + "`.";
            return result;
        }

        const double oldValue = lockable->getCurrentValueForCommand();
        const double factoryValue = factoryIt->second;
        lockable->setValueFromCommand(factoryValue);
        const double newValue = lockable->getCurrentValueForCommand();
        if (std::abs(newValue - oldValue) <= kConsoleValueEpsilon)
        {
            result.output = binding->slug + " already at factory value (" + formatConsoleValue(newValue) + ").";
            return result;
        }

        binding->previousValue = oldValue;
        binding->hasPreviousValue = true;
        binding->lastKnownValue = newValue;
        lockable->refresh();

        ConsoleAction historyAction;
        historyAction.label = "reset " + binding->slug;
        historyAction.undo = [lockable, oldValue]() { lockable->setValueFromCommand(oldValue); };
        historyAction.redo = [lockable, newValue]() { lockable->setValueFromCommand(newValue); };
        pushAction(std::move(historyAction));

        result.output = "Reset " + binding->slug + " to factory value " + formatConsoleValue(newValue) + ".";
        return result;
    }

    if (action == "lock" || action == "unlock")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: " + action + " <target>";
            return result;
        }

        auto* binding = findTarget(tokens[1]);
        if (binding == nullptr)
        {
            result.output = "ERROR: unknown target slug `" + tokens[1] + "`.";
            return result;
        }

        auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding->property);
        if (lockable == nullptr)
        {
            result.output = "ERROR: target `" + binding->slug + "` has no lock state.";
            return result;
        }

        const bool shouldLock = (action == "lock");
        const bool oldLock = lockable->isLocked();
        if (oldLock == shouldLock)
        {
            result.output = binding->slug + (shouldLock ? " already locked." : " already unlocked.");
            return result;
        }

        lockable->setLocked(shouldLock);
        lockable->refresh();

        ConsoleAction historyAction;
        historyAction.label = action + " " + binding->slug;
        historyAction.undo = [lockable, oldLock]() { lockable->setLocked(oldLock); };
        historyAction.redo = [lockable, shouldLock]() { lockable->setLocked(shouldLock); };
        pushAction(std::move(historyAction));

        result.output = binding->slug + (shouldLock ? " locked." : " unlocked.");
        return result;
    }

    if (action == "undo" || action == "redo")
    {
        cancelConsoleSweeps();
        int count = 1;
        if (tokens.size() >= 2)
            count = juce::jlimit(1, 64, tokens[1].getIntValue());
        juce::StringArray messages;
        for (int i = 0; i < count; ++i)
        {
            if (action == "undo")
            {
                if (consoleUndoStack.empty())
                    break;
                ConsoleAction entry = std::move(consoleUndoStack.back());
                consoleUndoStack.pop_back();
                if (entry.undo)
                    entry.undo();
                consoleRedoStack.push_back(std::move(entry));
                messages.add("undid");
            }
            else
            {
                if (consoleRedoStack.empty())
                    break;
                ConsoleAction entry = std::move(consoleRedoStack.back());
                consoleRedoStack.pop_back();
                if (entry.redo)
                    entry.redo();
                consoleUndoStack.push_back(std::move(entry));
                messages.add("redid");
            }
        }

        if (messages.isEmpty())
        {
            result.output = (action == "undo")
                ? "Nothing to undo."
                : "Nothing to redo.";
        }
        else
        {
            syncAllBindings(true);
            result.output = (action == "undo")
                ? ("Undid " + juce::String(messages.size()) + " action(s).")
                : ("Redid " + juce::String(messages.size()) + " action(s).");
        }
        return result;
    }

    if (action == "history")
    {
        juce::StringArray lines;
        lines.add("Recent console commands:");
        const int commandCount = juce::jmin(20, consoleCommandHistory.size());
        for (int i = 0; i < commandCount; ++i)
            lines.add(juce::String(i + 1) + ". " + consoleCommandHistory[i]);

        lines.add("");
        lines.add("Recent touches:");
        const int touchCount = juce::jmin(20, recentTouchHistory.size());
        for (int i = 0; i < touchCount; ++i)
            lines.add(juce::String(i + 1) + ". " + recentTouchHistory[i]);

        if (commandCount == 0 && touchCount == 0)
            lines.add("No history yet.");

        result.output = lines.joinIntoString("\n");
        return result;
    }

    if (action == "watch" || action == "unwatch")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: " + action + " <target>";
            return result;
        }

        ConsoleTargetBinding* binding = nullptr;
        LockableFloatPropertyComponent* lockable = nullptr;
        juce::String errorText;
        if (!resolveNumericTarget(tokens[1], binding, lockable, errorText))
        {
            result.output = errorText;
            return result;
        }
        juce::ignoreUnused(lockable);

        if (action == "watch")
        {
            if (!consoleWatchSlugs.contains(binding->slug))
                consoleWatchSlugs.add(binding->slug);
            result.output = "Watching `" + binding->slug + "`.";
        }
        else
        {
            consoleWatchSlugs.removeString(binding->slug);
            result.output = "Stopped watching `" + binding->slug + "`.";
        }
        return result;
    }

    if (action == "fx")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: fx <0|1|2>";
            return result;
        }

        const juce::String presetToken = tokens[1].trim();
        if (!(presetToken == "0" || presetToken == "1" || presetToken == "2"))
        {
            result.output = "ERROR: usage: fx <0|1|2>";
            return result;
        }
        const int preset = presetToken.getIntValue();

        runSnapshotAction("fx " + juce::String(preset), [this, preset] { applyValueFxPreset(preset); });
        result.output = "Applied FX preset " + juce::String(preset) + ".";
        return result;
    }

    if (action == "dump")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: dump <color>";
            return result;
        }

        const int engineIndex = parseEngineIndexToken(tokens[1]);
        if (engineIndex < 0)
        {
            result.output = "ERROR: usage: dump <green|blue|red|purple|black>";
            return result;
        }

        juce::StringArray lines;
        lines.add("Engine " + engineNameForIndex(engineIndex).toUpperCase() + " NQ:");
        forEachRuntimeTuningValue(processor.getEngineDspInternals(engineIndex, false),
                                  [&lines](const char* key, float value)
                                  {
                                      lines.add("  " + juce::String(key) + " = " + formatConsoleValue(value));
                                  });
        lines.add("");
        lines.add("Engine " + engineNameForIndex(engineIndex).toUpperCase() + " HQ:");
        forEachRuntimeTuningValue(processor.getEngineDspInternals(engineIndex, true),
                                  [&lines](const char* key, float value)
                                  {
                                      lines.add("  " + juce::String(key) + " = " + formatConsoleValue(value));
                                  });
        result.output = lines.joinIntoString("\n");
        return result;
    }

    if (action == "diff" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("factory"))
    {
        if (!ensureFactoryValueCache())
        {
            result.output = "ERROR: unable to compute factory diff.";
            return result;
        }

        const int activeEngine = juce::jlimit(0, 4, processor.getCurrentEngineColorIndex());
        juce::StringArray lines;
        lines.add("Diff vs factory (active engine: " + engineNameForIndex(activeEngine) + "):");

        int diffCount = 0;
        for (auto& binding : consoleTargets)
        {
            auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property);
            if (lockable == nullptr)
                continue;
            if (binding.engineScope >= 0 && binding.engineScope != activeEngine)
                continue;

            const auto it = consoleFactoryValues.find(binding.slug.toStdString());
            if (it == consoleFactoryValues.end())
                continue;
            const double current = lockable->getCurrentValueForCommand();
            const double factory = it->second;
            if (std::abs(current - factory) <= kConsoleValueEpsilon)
                continue;

            lines.add("  " + binding.slug + ": current=" + formatConsoleValue(current)
                      + ", factory=" + formatConsoleValue(factory)
                      + ", delta=" + formatConsoleValue(current - factory));
            ++diffCount;
        }

        if (diffCount == 0)
            lines.add("  No active-engine deltas from factory.");
        result.output = lines.joinIntoString("\n");
        return result;
    }

    if (action == "search")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: search <term>";
            return result;
        }

        const juce::String term = parsedCommand.fromFirstOccurrenceOf(" ", false, false).trim().toLowerCase();
        juce::StringArray queryTerms;
        queryTerms.addTokens(term, " \t", "");
        queryTerms.removeEmptyStrings();
        if (queryTerms.isEmpty())
        {
            result.output = "ERROR: usage: search <term>";
            return result;
        }

        juce::StringArray lines;
        lines.add("Search matches:");
        int matchCount = 0;
        for (auto& binding : consoleTargets)
        {
            auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding.property);
            if (lockable == nullptr)
                continue;

            juce::String haystack = binding.slug + " " + binding.displayName + " " + binding.property->getTooltip();
            haystack = haystack.toLowerCase();
            bool allMatched = true;
            for (const auto& query : queryTerms)
            {
                if (!haystack.contains(query))
                {
                    allMatched = false;
                    break;
                }
            }
            if (!allMatched)
                continue;

            lines.add("  " + binding.slug + " = " + formatConsoleValue(lockable->getCurrentValueForCommand())
                      + "  [" + binding.displayName + "]");
            ++matchCount;
            if (matchCount >= 80)
                break;
        }

        if (matchCount == 0)
            lines.add("  No matches.");
        result.output = lines.joinIntoString("\n");
        return result;
    }

    if (action == "stats")
    {
        const auto& telemetry = processor.getLiveTelemetry();
        const auto& analyzerCfg = processor.getAnalyzerRuntimeConfig();
        juce::StringArray lines;
        lines.add("Live telemetry:");
        lines.add("  process_ms_last=" + formatConsoleValue(telemetry.lastProcessMs.load(), 3)
                  + ", process_ms_peak=" + formatConsoleValue(telemetry.maxProcessMs.load(), 3));
        lines.add("  process_blocks=" + juce::String(static_cast<long long>(telemetry.processBlockCount.load()))
                  + ", param_writes=" + juce::String(static_cast<long long>(telemetry.parameterWriteCount.load())));
        lines.add("  engine_switches=" + juce::String(static_cast<long long>(telemetry.engineSwitchCount.load()))
                  + ", hq_toggles=" + juce::String(static_cast<long long>(telemetry.hqToggleCount.load())));
        lines.add("  input_peak_l=" + formatConsoleValue(telemetry.inputPeakL.load(), 6)
                  + ", input_peak_r=" + formatConsoleValue(telemetry.inputPeakR.load(), 6));
        lines.add("  output_peak_l=" + formatConsoleValue(telemetry.outputPeakL.load(), 6)
                  + ", output_peak_r=" + formatConsoleValue(telemetry.outputPeakR.load(), 6));
        lines.add("  engine=" + engineNameForIndex(processor.getCurrentEngineColorIndex())
                  + ", hq=" + juce::String(processor.isHqEnabled() ? "on" : "off")
                  + ", analyzer_refresh_hz=" + juce::String(analyzerCfg.refreshHz.load()));
        result.output = lines.joinIntoString("\n");
        return result;
    }

    if (action == "list")
    {
        if (tokens.size() < 2)
        {
            result.output = "ERROR: usage: list <color|globals>";
            return result;
        }

        const juce::String mode = tokens[1].toLowerCase();
        int selectedEngine = -1;
        const bool globalsOnly = mode == "globals";
        if (!globalsOnly)
            selectedEngine = parseEngineIndexToken(mode);

        if (!globalsOnly && selectedEngine < 0)
        {
            result.output = "ERROR: usage: list <green|blue|red|purple|black|globals>";
            return result;
        }

        bool includeGlobals = false;
        bool fullOutput = false;
        for (int i = 2; i < tokens.size(); ++i)
        {
            const juce::String option = tokens[i].toLowerCase();
            if (option == "all")
                includeGlobals = true;
            else if (option == "full")
                fullOutput = true;
            else
            {
                result.output = "ERROR: unknown list option `" + tokens[i] + "`. Use `all` and/or `full`.";
                return result;
            }
        }

        const std::string cacheKey = [&]()
        {
            std::string key = globalsOnly ? "globals" : ("engine:" + std::to_string(selectedEngine));
            if (includeGlobals)
                key += ":all";
            key += fullOutput ? ":full" : ":dedup";
            return key;
        }();

        if (const auto cacheIt = consoleListOutputCache.find(cacheKey); cacheIt != consoleListOutputCache.end())
        {
            result.output = cacheIt->second;
            return result;
        }

        std::vector<const ConsoleTargetBinding*> sorted;
        sorted.reserve(consoleTargets.size());
        for (const auto& binding : consoleTargets)
        {
            if (globalsOnly)
            {
                if (binding.engineScope >= 0)
                    continue;
            }
            else
            {
                if (binding.engineScope == selectedEngine)
                {
                    // engine-scoped entry
                }
                else if (includeGlobals && binding.engineScope < 0)
                {
                    // global entry included explicitly
                }
                else
                    continue;
            }
            sorted.push_back(&binding);
        }
        std::sort(sorted.begin(), sorted.end(), [](const ConsoleTargetBinding* a, const ConsoleTargetBinding* b)
        {
            return a->slug < b->slug;
        });

        juce::StringArray lines;
        if (fullOutput)
        {
            lines.add(globalsOnly
                ? "Global parameter slugs (full):"
                : ("Parameter slugs for " + engineNameForIndex(selectedEngine)
                   + (includeGlobals ? " + globals (full):" : " (engine-scoped, full):")));
            for (const auto* binding : sorted)
                lines.add("  " + binding->slug + "  [" + binding->displayName + "]");
            if (sorted.empty())
                lines.add("  (none)");
            else
                lines.add("Total: " + juce::String(static_cast<int>(sorted.size())) + " slugs.");
        }
        else
        {
            struct CollapsedEntry
            {
                juce::String canonicalSlug;
                juce::String displayName;
                int variants = 0;
            };

            std::unordered_map<std::string, size_t> indexByCanonical;
            std::vector<CollapsedEntry> collapsed;
            collapsed.reserve(sorted.size());

            for (const auto* binding : sorted)
            {
                const juce::String canonical = canonicalSlugForList(binding->slug);
                const std::string key = canonical.toStdString();
                const auto it = indexByCanonical.find(key);
                if (it == indexByCanonical.end())
                {
                    indexByCanonical.emplace(key, collapsed.size());
                    collapsed.push_back({ canonical, binding->displayName, 1 });
                }
                else
                {
                    auto& entry = collapsed[it->second];
                    ++entry.variants;
                    if (binding->displayName.length() < entry.displayName.length())
                        entry.displayName = binding->displayName;
                }
            }

            std::sort(collapsed.begin(), collapsed.end(), [](const CollapsedEntry& a, const CollapsedEntry& b)
            {
                return a.canonicalSlug < b.canonicalSlug;
            });

            lines.add(globalsOnly
                ? "Global parameter slugs (deduped):"
                : ("Parameter slugs for " + engineNameForIndex(selectedEngine)
                   + (includeGlobals ? " + globals (deduped):" : " (engine-scoped, deduped):")));

            constexpr int kDefaultListDisplayCap = 180;
            const int visibleCount = juce::jmin(kDefaultListDisplayCap, static_cast<int>(collapsed.size()));
            for (int i = 0; i < visibleCount; ++i)
            {
                const auto& entry = collapsed[static_cast<size_t>(i)];
                juce::String line = "  " + entry.canonicalSlug + "  [" + entry.displayName + "]";
                if (entry.variants > 1)
                    line << "  (" << juce::String(entry.variants) << " variants)";
                lines.add(line);
            }

            if (collapsed.empty())
                lines.add("  (none)");
            else
                lines.add("Total: " + juce::String(static_cast<int>(collapsed.size())) + " unique slugs.");

            if (static_cast<int>(collapsed.size()) > visibleCount)
            {
                lines.add("Showing first " + juce::String(visibleCount)
                          + " entries; use `list " + (globalsOnly ? juce::String("globals") : engineNameForIndex(selectedEngine))
                          + " full` for every variant slug.");
            }

            if (!globalsOnly)
                lines.add("Hint: use `list " + engineNameForIndex(selectedEngine) + " full` for every variant slug.");
        }
        result.output = lines.joinIntoString("\n");
        consoleListOutputCache[cacheKey] = result.output;
        return result;
    }

    if (action == "export" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("script"))
    {
        std::vector<const ConsoleTargetBinding*> sorted;
        sorted.reserve(consoleTargets.size());
        for (const auto& binding : consoleTargets)
        {
            if (dynamic_cast<LockableFloatPropertyComponent*>(binding.property) != nullptr)
                sorted.push_back(&binding);
        }
        std::sort(sorted.begin(), sorted.end(), [](const ConsoleTargetBinding* a, const ConsoleTargetBinding* b)
        {
            return a->slug < b->slug;
        });

        juce::StringArray scriptLines;
        scriptLines.add("# Choroboros DevPanel Console Export");
        scriptLines.add("engine " + engineNameForIndex(processor.getCurrentEngineColorIndex()));
        scriptLines.add("hq " + juce::String(processor.isHqEnabled() ? "on" : "off"));
        for (const auto* binding : sorted)
        {
            auto* lockable = dynamic_cast<LockableFloatPropertyComponent*>(binding->property);
            if (lockable == nullptr)
                continue;
            scriptLines.add("set " + binding->slug + " " + formatConsoleValue(lockable->getCurrentValueForCommand(), 8));
        }
        juce::SystemClipboard::copyTextToClipboard(scriptLines.joinIntoString("\n"));
        result.output = "Exported " + juce::String(static_cast<int>(sorted.size()))
                      + " set commands to clipboard.";
        return result;
    }

    if (action == "import" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("script"))
    {
        juce::String scriptText;
        const juce::String scriptArg = parsedCommand.fromFirstOccurrenceOf("import script", false, true).trim();
        if (scriptArg.isNotEmpty())
        {
            const juce::File scriptFile = juce::File(scriptArg.unquoted());
            if (!scriptFile.existsAsFile())
            {
                result.output = "ERROR: import script file not found: " + scriptFile.getFullPathName();
                return result;
            }
            scriptText = scriptFile.loadFileAsString().trim();
        }
        else
        {
            scriptText = juce::SystemClipboard::getTextFromClipboard().trim();
        }

        if (scriptText.isEmpty())
        {
            result.output = "ERROR: no import script found in clipboard.";
            return result;
        }

        juce::StringArray lines;
        lines.addLines(scriptText);
        int applied = 0;
        int failed = 0;
        juce::StringArray failLines;

        for (int i = 0; i < lines.size(); ++i)
        {
            const juce::String line = lines[i].trim();
            if (line.isEmpty() || line.startsWithChar('#') || line.startsWith("//"))
                continue;
            if (line.startsWithIgnoreCase("import script"))
            {
                ++failed;
                failLines.add("line " + juce::String(i + 1) + ": recursive import script command ignored");
                continue;
            }

            const auto nested = executeConsoleCommand(line);
            if (nested.output.startsWithIgnoreCase("ERROR:"))
            {
                ++failed;
                failLines.add("line " + juce::String(i + 1) + ": " + nested.output);
            }
            else
            {
                ++applied;
            }
        }

        juce::StringArray summary;
        summary.add("Import complete: applied=" + juce::String(applied) + ", failed=" + juce::String(failed) + ".");
        const int maxErrors = juce::jmin(10, failLines.size());
        for (int i = 0; i < maxErrors; ++i)
            summary.add("  " + failLines[i]);
        result.output = summary.joinIntoString("\n");
        return result;
    }

    result.output = "ERROR: unknown command `" + action + "`. Type `help` for available commands.";
    return result;
}
