#!/bin/bash
set -e
cd "$(dirname "$0")/.."

rm -f .git/index.lock .git/HEAD.lock

git add Source/UI/DevPanelTabBuilders.cpp
git commit -m "Fix scope cards: register in liveReadoutProperties for timer refresh

Scope cards must appear in liveReadoutProperties so refreshVisibleLiveReadouts()
calls refresh() (same path as makeSparkline). They also stay in
modulationVisualizerProperties for analyzer LFO demand."

rm -f .git/index.lock .git/HEAD.lock

git push origin main

echo "Done — scope refresh fix committed and pushed."
