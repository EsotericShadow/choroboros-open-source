#!/bin/bash
cd "$(dirname "$0")/.." || exit 1

rm -f .git/index.lock .git/HEAD.lock .git/objects/maintenance.lock

git add Source/UI/DevPanelSupport.h Source/UI/DevPanelTabBuilders.cpp Source/UI/DevPanelRuntime.cpp
git commit -m "Redesign Modulation tab with dual oscilloscope and Lissajous scope

Replace separate L/R sparkline cards with a single overlaid dual
waveform oscilloscope showing both channels with phase markers.
Add Lissajous XY stereo field display for instant stereo correlation
feedback. Restructure layout: scope (primary), Lissajous + trajectory
(secondary), workbench (action). Enrich inspector from 4 to 8
readouts in Modulation and Stereo sections.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"

rm -f .git/index.lock .git/HEAD.lock
git push origin main

echo ""
echo "Done. Run 'git log --oneline -3' to verify."
