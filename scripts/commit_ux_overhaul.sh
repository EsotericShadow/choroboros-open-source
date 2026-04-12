#!/bin/bash
set -e
cd "$(dirname "$0")/.."

rm -f .git/index.lock .git/HEAD.lock

git add Source/UI/DevPanelSupport.h
git add Source/UI/DevPanelTabBuilders.cpp
git add Source/UI/DevPanel.cpp
git add Source/UI/DevPanelRuntime.cpp
git add docs/developer/MODULATION_UX_OVERHAUL.md
git add docs/developer/MODULATION_UX_AGENT_PROMPT.md

git commit -m "Modulation tab intuitiveness overhaul: self-documenting visuals

- Add subtitles to scope cards (Delay modulation over one LFO cycle,
  L vs R phase relationship)
- Add Y-axis depth label to oscilloscope
- Add L/R axis labels and shape hints (wide/mono) to Stereo Field
- Rename Modulation Depth -> Measured Swing, Phase Difference ->
  Effective Phase Spread, Stereo Correlation -> Stereo Coherence
- Add descriptive section headers (live values from DSP, derived
  from offset + width)
- Update workbench subtitle to reference real-time scope feedback
- Add plain-English tooltips for all renamed readouts
- Update modulation tutorial steps for new terminology and visuals
- Improve legend text from L/R to L ch/R ch

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"

rm -f .git/index.lock .git/HEAD.lock

git push origin main

echo "Done -- UX intuitiveness overhaul committed and pushed."
