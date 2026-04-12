#!/bin/bash
# Run this from the choroboros-open-source directory
cd "$(dirname "$0")/.." || exit 1

# Clean up any stale lock files
rm -f .git/index.lock .git/HEAD.lock .git/objects/maintenance.lock

# Commit 3: UI/DevPanel improvements
git add Source/UI/DevPanel.cpp Source/UI/DevPanel.h Source/UI/DevPanelRuntime.cpp Source/Plugin/PluginEditor.cpp Source/Plugin/PluginEditor.h
git commit -m "Add tooltip toggle and fix DevPanel tab layout

DevPanel: add tooltip enable/disable setting in Accessibility
PluginEditor: expose setTooltipsEnabled for main UI tooltip control
DevPanelRuntime: fix tab button width calc and apply tooltip preference
DevPanel window widened from 900 to 1100px

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"

# Push all 3 commits
git push origin main

echo ""
echo "Done! Run 'git log --oneline -5' to verify."
