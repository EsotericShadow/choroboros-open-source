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

#include "ConsoleEngine.h"
#include "PluginProcessor.h"
#include "../DSP/ChorusDSP.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>

namespace
{
constexpr double kConsoleValueEpsilon = 1.0e-6;

juce::String engineNameForIndex(int engineIndex)
{
    static const juce::String names[] { "green", "blue", "red", "purple", "black" };
    return names[juce::jlimit(0, 4, engineIndex)];
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
} // anonymous namespace

ConsoleEngine::ConsoleEngine(ChoroborosAudioProcessor& processor)
    : processor_(processor)
{
}

ConsoleEngine::Result ConsoleEngine::execute(const juce::String& commandLine)
{
    Result result;
    const juce::String trimmed = commandLine.trim();
    if (trimmed.isEmpty())
        return result;

    // Expand aliases (up to 8 levels deep to prevent infinite loops)
    juce::String parsedCommand = trimmed;
    {
        std::unordered_set<std::string> visitedAliases;
        for (int depth = 0; depth < 8; ++depth)
        {
            const juce::String head = parsedCommand.upToFirstOccurrenceOf(" ", false, false).trim().toLowerCase();
            if (head.isEmpty() || head == "alias")
                break;
            const auto it = aliases_.find(head.toStdString());
            if (it == aliases_.end())
                break;

            if (!visitedAliases.insert(head.toStdString()).second)
            {
                result.output = "ERROR: alias loop detected while expanding `" + head + "`.";
                return result;
            }

            const juce::String tail = parsedCommand.fromFirstOccurrenceOf(" ", false, false).trim();
            parsedCommand = juce::String(it->second) + (tail.isNotEmpty() ? (" " + tail) : "");
        }
    }

    // Add to history
    const juce::String timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S");
    commandHistory_.insert(0, timestamp + "  " + trimmed);
    while (commandHistory_.size() > kMaxHistorySize)
        commandHistory_.remove(commandHistory_.size() - 1);

    // Parse tokens
    juce::StringArray tokens;
    tokens.addTokens(parsedCommand, " \t", "\"'");
    tokens.trim();
    tokens.removeEmptyStrings();
    if (tokens.isEmpty())
        return result;

    const juce::String action = tokens[0].toLowerCase();

    // Dispatch to command handlers
    if (action == "help")
        return executeHelpCommand();

    if (action == "clear")
        return executeClearCommand();

    if (action == "alias")
        return executeAliasCommand(tokens);

    if (action == "cp" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("json"))
        return executeCpJsonCommand(tokens);

    if (action == "save" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("defaults"))
        return executeSaveDefaultsCommand(tokens);

    if (action == "core")
        return executeCoreCommand(tokens);

    if (action == "slot")
        return executeSlotCommand(tokens);

    if (action == "engine")
        return executeEngineCommand(tokens);

    if (action == "hq")
        return executeHqCommand(tokens);

    if (action == "get")
        return executeGetCommand(tokens, parsedCommand);

    if (action == "set")
        return executeSetCommand(tokens, parsedCommand);

    if (action == "add")
        return executeAddCommand(tokens, parsedCommand);

    if (action == "sub")
        return executeSubCommand(tokens, parsedCommand);

    if (action == "toggle")
        return executeToggleCommand(tokens);

    if (action == "macro")
        return executeMacroCommand(tokens);

    if (action == "reset")
        return executeResetCommand(tokens);

    if (action == "history")
        return executeHistoryCommand();

    if (action == "dump")
        return executeDumpCommand(tokens);

    if (action == "diff" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("factory"))
        return executeDiffFactoryCommand();

    if (action == "search")
        return executeSearchCommand(tokens);

    if (action == "stats")
        return executeStatsCommand();

    if (action == "list")
        return executeListCommand(tokens);

    if (action == "export" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("script"))
        return executeExportScriptCommand();

    if (action == "import" && tokens.size() >= 2 && tokens[1].equalsIgnoreCase("script"))
        return executeImportScriptCommand(parsedCommand);

    // Commands that stay UI-side and must be deferred
    // view, tutorial, watch, unwatch, sweep, undo, redo, lock, unlock, bypass, solo, unsolo, fx
    if (action == "view" || action == "tutorial" || action == "watch" || action == "unwatch"
        || action == "sweep" || action == "undo" || action == "redo" || action == "lock"
        || action == "unlock" || action == "bypass" || action == "solo" || action == "unsolo" || action == "fx")
    {
        result.output = "NOTE: command `" + action + "` is UI-session scoped and must be handled by DevPanel adapter.";
        result.success = false;
        return result;
    }

    result.output = "ERROR: unknown command `" + action + "`. Use `help` for available commands.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeHelpCommand()
{
    Result result;
    result.output =
        "Headless Commands (Tier 1):\n"
        "Engine & Core:\n"
        "  engine <green|blue|red|purple|black>\n"
        "  hq <on|off>\n"
        "  slot show\n"
        "  slot set <engine> <nq|hq> <core_id>\n"
        "  core list\n"
        "  core show <id>\n"
        "Parameters:\n"
        "  set <slug> <value>\n"
        "  get <slug>\n"
        "  add <slug> <delta>\n"
        "  sub <slug> <delta>\n"
        "  reset <slug>\n"
        "  reset all\n"
        "  toggle <slug>\n"
        "  macro <rate|depth|offset|width|color|mix> <0..100>\n"
        "Queries:\n"
        "  history\n"
        "  stats\n"
        "  search <term>\n"
        "  list <color|globals> [full]\n"
        "  dump <color>\n"
        "  diff factory\n"
        "  export script\n"
        "  import script\n"
        "Utility:\n"
        "  alias <name> <command>\n"
        "  cp json\n"
        "  save defaults\n"
        "  clear\n"
        "\n"
        "UI-Session Commands (DevPanel Adapter):\n"
        "  view <tab> | tutorial | watch | unwatch | sweep | undo | redo\n"
        "  lock | unlock | bypass | solo | unsolo | fx\n"
        "\nUse headless tests to verify command behavior without UI construction.";
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeClearCommand()
{
    Result result;
    result.clearOutput = true;
    result.output = "Console cleared.";
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeAliasCommand(const juce::StringArray& tokens)
{
    Result result;
    if (tokens.size() < 3)
    {
        result.output = "ERROR: usage: alias <name> <command>";
        result.success = false;
        return result;
    }

    const juce::String aliasName = slugifyParameterName(tokens[1]);
    if (aliasName.isEmpty())
    {
        result.output = "ERROR: alias name is invalid.";
        result.success = false;
        return result;
    }
    if (aliasName == "alias")
    {
        result.output = "ERROR: alias name cannot be `alias`.";
        result.success = false;
        return result;
    }

    const juce::String fullCommand = juce::String::join(
        juce::StringArray(tokens.begin() + 2, tokens.end()), " ");

    if (fullCommand.isEmpty())
    {
        result.output = "ERROR: alias command body cannot be empty.";
        result.success = false;
        return result;
    }

    aliases_[aliasName.toStdString()] = fullCommand.toStdString();
    result.output = "Alias `" + aliasName + "` => `" + fullCommand + "`";
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeCpJsonCommand(const juce::StringArray& tokens)
{
    Result result;
    // TODO: Implement copy processor state to JSON
    result.output = "NOTE: cp json command deferred to DevPanel adapter.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeSaveDefaultsCommand(const juce::StringArray& tokens)
{
    Result result;
    // TODO: Implement save current state as user defaults
    result.output = "NOTE: save defaults command deferred to DevPanel adapter.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeCoreCommand(const juce::StringArray& tokens)
{
    Result result;
    if (tokens.size() < 2)
    {
        result.output = "ERROR: usage: core <list|show>";
        result.success = false;
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
            result.success = false;
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
            result.success = false;
            return result;
        }

        const auto& descriptor = choroboros::descriptorForCore(coreId);
        const auto assignments = choroboros::findAssignmentsForCore(processor_.getCoreAssignments(), coreId);
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
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeSlotCommand(const juce::StringArray& tokens)
{
    Result result;
    if (tokens.size() < 2)
    {
        result.output = "ERROR: usage: slot <show|set>";
        result.success = false;
        return result;
    }

    const juce::String op = tokens[1].toLowerCase();
    if (op == "show")
    {
        const auto& assignments = processor_.getCoreAssignments();
        const int activeEngine = juce::jlimit(0, 4, processor_.getCurrentEngineColorIndex());
        const bool activeHq = processor_.isHqEnabled();
        juce::StringArray lines;
        lines.add("Slot assignments (modular cores " + juce::String(processor_.isModularCoresEnabled() ? "on" : "off") + "):");
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
        const auto dupes = processor_.getDuplicateAssignmentWarnings();
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
            result.success = false;
            return result;
        }

        const int engineIndex = parseEngineIndexToken(tokens[2]);
        if (engineIndex < 0)
        {
            result.output = "ERROR: invalid engine. Use: green, blue, red, purple, black.";
            result.success = false;
            return result;
        }

        bool hqEnabled = false;
        if (!tryParseMode(tokens[3], hqEnabled))
        {
            result.output = "ERROR: invalid mode. Use nq or hq.";
            result.success = false;
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
            result.success = false;
            return result;
        }

        const bool success = processor_.setCoreAssignment(engineIndex, hqEnabled, coreId);
        if (!success)
        {
            result.output = "ERROR: failed to set core assignment.";
            result.success = false;
            return result;
        }

        result.output = "Assigned " + engineNameForIndex(engineIndex) + " "
                       + juce::String(hqEnabled ? "hq" : "nq")
                       + " -> " + juce::String(choroboros::coreIdToToken(coreId));
        return result;
    }

    result.output = "ERROR: usage: slot <show|set>";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeEngineCommand(const juce::StringArray& tokens)
{
    Result result;
    if (tokens.size() < 2)
    {
        result.output = "ERROR: usage: engine <green|blue|red|purple|black>";
        result.success = false;
        return result;
    }

    const int engineIndex = parseEngineIndexToken(tokens[1]);
    if (engineIndex < 0)
    {
        result.output = "ERROR: invalid engine. Use: green, blue, red, purple, black.";
        result.success = false;
        return result;
    }

    // Set APVTS engine parameter (assume parameter ID follows naming convention)
    const juce::String paramId = "engine";  // TODO: verify actual parameter ID
    if (auto* param = processor_.getValueTreeState().getParameter(paramId))
    {
        const float normalized = static_cast<float>(engineIndex) / 4.0f;
        param->beginChangeGesture();
        param->setValueNotifyingHost(normalized);
        param->endChangeGesture();
        result.output = "Engine switched to " + engineNameForIndex(engineIndex);
        return result;
    }

    result.output = "ERROR: failed to set engine parameter.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeHqCommand(const juce::StringArray& tokens)
{
    Result result;
    if (tokens.size() < 2)
    {
        result.output = "ERROR: usage: hq <on|off>";
        result.success = false;
        return result;
    }

    bool hqEnabled = false;
    if (!tryParseOnOff(tokens[1], hqEnabled))
    {
        result.output = "ERROR: invalid mode. Use on or off.";
        result.success = false;
        return result;
    }

    const juce::String paramId = "hq";  // TODO: verify actual parameter ID
    if (auto* param = processor_.getValueTreeState().getParameter(paramId))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(hqEnabled ? 1.0f : 0.0f);
        param->endChangeGesture();
        result.output = "HQ mode " + juce::String(hqEnabled ? "on" : "off");
        return result;
    }

    result.output = "ERROR: failed to set HQ parameter.";
    result.success = false;
    return result;
}

// Stub implementations for remaining commands - to be filled in
ConsoleEngine::Result ConsoleEngine::executeGetCommand(const juce::StringArray& tokens, const juce::String& fullCommand)
{
    Result result;
    result.output = "NOTE: get command - deferred to handle parameter slug registry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeSetCommand(const juce::StringArray& tokens, const juce::String& fullCommand)
{
    Result result;
    result.output = "NOTE: set command - deferred to handle parameter slug registry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeAddCommand(const juce::StringArray& tokens, const juce::String& fullCommand)
{
    Result result;
    result.output = "NOTE: add command - deferred to handle parameter slug registry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeSubCommand(const juce::StringArray& tokens, const juce::String& fullCommand)
{
    Result result;
    result.output = "NOTE: sub command - deferred to handle parameter slug registry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeToggleCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: toggle command - deferred to handle parameter slug registry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeMacroCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: macro command - deferred to handle parameter mutation.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeResetCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: reset command - deferred to handle factory defaults.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeHistoryCommand()
{
    Result result;
    if (commandHistory_.isEmpty())
    {
        result.output = "(no history)";
        return result;
    }
    result.output = commandHistory_.joinIntoString("\n");
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeDumpCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: dump command - deferred to DSP state readout.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeDiffFactoryCommand()
{
    Result result;
    result.output = "NOTE: diff factory command - deferred to factory default comparison.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeSearchCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: search command - deferred to parameter enumeration.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeStatsCommand()
{
    Result result;
    result.output = "NOTE: stats command - deferred to diagnostics/telemetry.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeListCommand(const juce::StringArray& tokens)
{
    Result result;
    result.output = "NOTE: list command - deferred to parameter enumeration.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeExportScriptCommand()
{
    Result result;
    result.output = "NOTE: export script command - deferred to state export.";
    result.success = false;
    return result;
}

ConsoleEngine::Result ConsoleEngine::executeImportScriptCommand(const juce::String& fullCommand)
{
    Result result;
    result.output = "NOTE: import script command - deferred to state import.";
    result.success = false;
    return result;
}

juce::StringArray ConsoleEngine::getCommandHistory() const
{
    return commandHistory_;
}

juce::StringArray ConsoleEngine::getAvailableCommands() const
{
    return { "help", "core", "slot", "engine", "hq", "get", "set", "add", "sub", "toggle", "macro", "reset",
             "history", "dump", "diff", "search", "stats", "list", "export", "import", "alias", "cp", "save", "clear",
             "view", "tutorial", "watch", "unwatch", "sweep", "undo", "redo", "lock", "unlock", "bypass", "solo", "unsolo", "fx" };
}

juce::StringArray ConsoleEngine::getAvailableParameterSlugs(const juce::String& filterColor) const
{
    // TODO: Enumerate available parameter slugs from APVTS
    return {};
}

juce::String ConsoleEngine::getArchitectureNote()
{
    return "ConsoleEngine Architecture: Headless-Safe Commands\n"
           "====================================================\n"
           "This service executes only headless-safe console commands.\n"
           "It does NOT depend on UI classes (DevPanel, PluginEditor, etc.).\n"
           "\n"
           "Group A (Headless): core, slot, engine, hq, get/set/add/sub/reset, macro, etc.\n"
           "Group B (UI-side): view, tutorial, watch, unwatch, sweep, undo/redo, lock/unlock, bypass, solo, fx\n"
           "\n"
           "Deferred decisions:\n"
           "- Slug-based parameter access (set/get/add/sub/reset/toggle) needs parameter slug registry\n"
           "- Lock/unlock/toggle requires LockableFloatPropertyComponent registration (UI-side)\n"
           "- Watch HUD, sweep animation, undo/redo snapshots are UI-session state (UI-side)\n"
           "\n"
           "This split allows Tier 1 headless tests without UI construction.";
}

// Utility methods
bool ConsoleEngine::tryParseOnOff(const juce::String& token, bool& outValue) const
{
    const auto lower = token.trim().toLowerCase();
    if (lower == "on" || lower == "true" || lower == "1")
    {
        outValue = true;
        return true;
    }
    if (lower == "off" || lower == "false" || lower == "0")
    {
        outValue = false;
        return true;
    }
    return false;
}

bool ConsoleEngine::tryParseMode(const juce::String& token, bool& outHqEnabled) const
{
    const auto lower = token.trim().toLowerCase();
    if (lower == "hq")
    {
        outHqEnabled = true;
        return true;
    }
    if (lower == "nq")
    {
        outHqEnabled = false;
        return true;
    }
    return false;
}

int ConsoleEngine::tryParseEngineIndex(const juce::String& token) const
{
    return parseEngineIndexToken(token);
}

juce::String ConsoleEngine::formatValue(double value, int decimals) const
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

bool ConsoleEngine::tryParseDouble(const juce::String& token, double& outValue) const
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

juce::String ConsoleEngine::slugifyParameterName(const juce::String& input) const
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

juce::String ConsoleEngine::getParameterLabel(const juce::String& paramId) const
{
    if (auto* param = processor_.getValueTreeState().getParameter(paramId))
        return param->getLabel();
    return juce::String();
}

float ConsoleEngine::getCurrentParameterValue(const juce::String& paramId) const
{
    if (auto* param = processor_.getValueTreeState().getParameter(paramId))
        return param->getValue();
    return 0.0f;
}

void ConsoleEngine::setParameterValue(const juce::String& paramId, float mappedValue)
{
    if (auto* param = processor_.getValueTreeState().getParameter(paramId))
    {
        const auto& range = processor_.getValueTreeState().getParameterRange(paramId);
        const float normalized = juce::jlimit(0.0f, 1.0f, range.convertTo0to1(mappedValue));
        param->beginChangeGesture();
        param->setValueNotifyingHost(normalized);
        param->endChangeGesture();
    }
}

bool ConsoleEngine::findParameterBySlug(const juce::String& slug, juce::String& outParamId) const
{
    // TODO: Implement parameter slug lookup
    return false;
}
