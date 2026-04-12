#!/bin/bash
set -e
cd "$(dirname "$0")/.."

rm -f .git/index.lock .git/HEAD.lock

git add Source/UI/DevPanelSupport.h
git add Source/UI/DevPanelTabBuilders.cpp
git add Source/UI/DevPanel.cpp
git add Source/UI/DevPanelRuntime.cpp
git add Source/Plugin/PluginEditor.cpp
git add docs/developer/MODULATION_UX_OVERHAUL.md
git add docs/developer/MODULATION_UX_AGENT_PROMPT.md

git commit -m "Modulation tab UX overhaul + restore degree symbol

UX intuitiveness pass:
- Add subtitles to scope cards explaining what each shows
- Add Y-axis depth label to oscilloscope, L/R axis labels to Stereo Field
- Add dim shape hints (wide/mono) to Stereo Field
- Rename confusing readouts: Measured Swing, Effective Phase Spread,
  Stereo Coherence
- Add descriptive section headers and enriched tooltips
- Update workbench subtitle and tutorial steps

Degree symbol restoration:
- Replace all 'deg' display text with proper degree symbol
- Parser still accepts both 'deg' text and degree symbol input
- Consistent with main plugin UI which already uses degree symbol

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"

rm -f .git/index.lock .git/HEAD.lock

git push origin main

echo "Done -- UX overhaul and degree symbol fix committed and pushed."
