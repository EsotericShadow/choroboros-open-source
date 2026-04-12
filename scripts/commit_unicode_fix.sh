#!/bin/bash
cd "$(dirname "$0")/.." || exit 1

rm -f .git/index.lock .git/HEAD.lock .git/objects/maintenance.lock

git add Source/UI/DevPanelRuntime.cpp Source/Plugin/PluginEditor.cpp
git commit -m "Replace non-ASCII characters with ASCII equivalents

Em dashes, curly apostrophes, arrows, degree symbols, and bullets
were rendering as garbled text in the plugin UI. Replaced with
plain ASCII: -- for dashes, -> for arrows, ' for apostrophes,
deg for degree symbols.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"

rm -f .git/index.lock .git/HEAD.lock
git push origin main

echo ""
echo "Done. Run 'git log --oneline -3' to verify."
