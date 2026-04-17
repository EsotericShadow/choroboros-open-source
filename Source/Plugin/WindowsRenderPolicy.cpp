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

#include "WindowsRenderPolicy.h"
#include "PluginProcessor.h"

namespace
{
#if JUCE_WINDOWS
int findPreferredSoftwareRenderingEngineIndex(const juce::StringArray& engines)
{
    for (int i = 0; i < engines.size(); ++i)
        if (engines[i].equalsIgnoreCase("GDI"))
            return i;

    for (int i = 0; i < engines.size(); ++i)
        if (engines[i].containsIgnoreCase("software"))
            return i;

    for (int i = 0; i < engines.size(); ++i)
        if (engines[i].containsIgnoreCase("gdi"))
            return i;

    return -1;
}

juce::String getRendererName(const juce::StringArray& engines, int index)
{
    return juce::isPositiveAndBelow(index, engines.size()) ? engines[index] : juce::String("unknown");
}
#endif
} // namespace

namespace choroboros::windows
{
void applyPreferredRenderer(juce::Component& component,
                            const juce::String& context,
                            const ChoroborosAudioProcessor* processor)
{
#if JUCE_WINDOWS
    if (auto* peer = component.getPeer())
        applyPreferredRenderer(*peer, context, processor);
#else
    juce::ignoreUnused(component, context, processor);
#endif
}

void applyPreferredRenderer(juce::ComponentPeer& peer,
                            const juce::String& context,
                            const ChoroborosAudioProcessor* processor)
{
#if JUCE_WINDOWS
    const auto engines = peer.getAvailableRenderingEngines();
    const auto preferredIndex = findPreferredSoftwareRenderingEngineIndex(engines);

    if (preferredIndex < 0)
    {
        if (processor != nullptr)
            processor->logLoadTraceEvent("windows_render_policy_missing_software_renderer", 0.0,
                                         "context=" + context + ";available=" + engines.joinIntoString("|"));
        return;
    }

    const auto beforeIndex = peer.getCurrentRenderingEngine();
    if (beforeIndex != preferredIndex)
        peer.setCurrentRenderingEngine(preferredIndex);

    if (processor != nullptr)
    {
        const auto afterIndex = peer.getCurrentRenderingEngine();
        processor->logLoadTraceEvent("windows_render_policy_applied", 0.0,
                                     "context=" + context
                                     + ";before=" + getRendererName(engines, beforeIndex)
                                     + ";after=" + getRendererName(engines, afterIndex)
                                     + ";available=" + engines.joinIntoString("|"));
    }
#else
    juce::ignoreUnused(peer, context, processor);
#endif
}
} // namespace choroboros::windows
