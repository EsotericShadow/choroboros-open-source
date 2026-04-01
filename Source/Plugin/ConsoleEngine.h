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

#include <juce_core/juce_core.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ChoroborosAudioProcessor;
class ChorusDSP;

/** Headless console command execution engine.
 *
 * This service encapsulates all headless-safe console command logic.
 * It does NOT depend on UI classes (DevPanel, PluginEditor, CommandConsolePropertyComponent).
 * It operates on narrow processor interfaces (parameter access, preset state).
 *
 * Responsibilities:
 * - Command parsing and dispatch for headless-safe commands
 * - Engine selection, HQ mode, slot assignment, core listing
 * - Parameter queries and mutations (set, get, add, sub, reset, macro)
 * - Help, history, stats, diagnostics
 * - Alias registry
 * - Command history (in-memory)
 *
 * NOT Responsible For:
 * - DevPanel tab/view switching (stays in DevPanel)
 * - Tutorial management (stays in DevPanel)
 * - Lock/unlock/toggle of UI properties (requires LockableFloatPropertyComponent registry)
 * - Watch HUD state (stays in DevPanel)
 * - Sweep animation (stays in DevPanel)
 * - Undo/redo with full UI snapshot (stays in DevPanel)
 * - Layout mode switching (stays in DevPanel)
 * - Console widget rendering (stays in DevPanel)
 */
class ConsoleEngine final
{
public:
    struct Result
    {
        bool success = true;
        juce::String output;
        bool clearOutput = false;  // If true, UI should clear console before displaying this output
    };

    /** Construct a headless console engine bound to a processor.
     * No UI construction happens here. */
    explicit ConsoleEngine(ChoroborosAudioProcessor& processor);

    /** Execute a command string. Returns result with success/output.
     * Safe to call from any thread. */
    Result execute(const juce::String& commandLine);

    /** Get command history (most recent first). */
    juce::StringArray getCommandHistory() const;

    /** Get available command names (for autocomplete). */
    juce::StringArray getAvailableCommands() const;

    /** Get a list of parameter slugs for which the processor has APVTS parameters.
     * Can be filtered by color (e.g., "blue") or left empty for globals. */
    juce::StringArray getAvailableParameterSlugs(const juce::String& filterColor = juce::String()) const;

    /** Create a developer note explaining command ownership split and what went where. */
    static juce::String getArchitectureNote();

private:
    ChoroborosAudioProcessor& processor_;
    juce::StringArray commandHistory_;
    std::unordered_map<std::string, std::string> aliases_;

    // Maximum command history size
    static constexpr int kMaxHistorySize = 240;

    // Helper methods for command execution
    Result executeHelpCommand();
    Result executeClearCommand();
    Result executeAliasCommand(const juce::StringArray& tokens);
    Result executeCpJsonCommand(const juce::StringArray& tokens);
    Result executeSaveDefaultsCommand(const juce::StringArray& tokens);
    Result executeCoreCommand(const juce::StringArray& tokens);
    Result executeSlotCommand(const juce::StringArray& tokens);
    Result executeEngineCommand(const juce::StringArray& tokens);
    Result executeHqCommand(const juce::StringArray& tokens);
    Result executeGetCommand(const juce::StringArray& tokens, const juce::String& fullCommand);
    Result executeSetCommand(const juce::StringArray& tokens, const juce::String& fullCommand);
    Result executeAddCommand(const juce::StringArray& tokens, const juce::String& fullCommand);
    Result executeSubCommand(const juce::StringArray& tokens, const juce::String& fullCommand);
    Result executeToggleCommand(const juce::StringArray& tokens);
    Result executeMacroCommand(const juce::StringArray& tokens);
    Result executeResetCommand(const juce::StringArray& tokens);
    Result executeHistoryCommand();
    Result executeDumpCommand(const juce::StringArray& tokens);
    Result executeDiffFactoryCommand();
    Result executeSearchCommand(const juce::StringArray& tokens);
    Result executeStatsCommand();
    Result executeListCommand(const juce::StringArray& tokens);
    Result executeExportScriptCommand();
    Result executeImportScriptCommand(const juce::String& fullCommand);

    // Utility helpers
    bool tryParseOnOff(const juce::String& token, bool& outValue) const;
    bool tryParseMode(const juce::String& token, bool& outHqEnabled) const;
    int tryParseEngineIndex(const juce::String& token) const;
    juce::String formatValue(double value, int decimals = 6) const;
    bool tryParseDouble(const juce::String& token, double& outValue) const;
    juce::String slugifyParameterName(const juce::String& input) const;

    // Parameter access helpers
    juce::String getParameterLabel(const juce::String& paramId) const;
    float getCurrentParameterValue(const juce::String& paramId) const;
    void setParameterValue(const juce::String& paramId, float mappedValue);
    bool findParameterBySlug(const juce::String& slug, juce::String& outParamId) const;
};
