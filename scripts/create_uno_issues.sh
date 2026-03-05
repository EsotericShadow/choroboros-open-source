#!/bin/bash
# Create 6 GitHub issues from Uno Rosengren's feedback and close the 2 fixed issues.
# Requires: gh auth login (or GH_TOKEN set)

set -e
cd "$(dirname "$0")/.."

echo "Closing fixed issues #1 and #2..."
gh issue close 1 --comment "Fixed in v2.02.1: Engine selection parameter now reports isMetaParameter() = true."
gh issue close 2 --comment "Fixed in v2.02.1: Binary distribution moved to GitHub Releases."

echo ""
echo "Creating 6 issues from Uno Rosengren feedback..."

gh issue create --title "Knob sensitivity too high" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** Knobs feel too sensitive; difficult to pinpoint exact values. Sliding across screen for 0%–100% is fine, but precision is hard.

**Workaround:** Manual input (typed values).

**Suggestion:** Make knobs slightly less fast; doesn't feel like most plugins.

**Category:** UI / UX"

gh issue create --title "Knob low-speed dropout" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** When moving knobs too slowly, input stops registering sometimes.

**Expected:** Should not happen; all movement should register.

**Category:** UI / UX" \
  --label "bug"

gh issue create --title "HQ/NQ button – off-kilter hit area" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** Can click to deactivate but must click *above* the button to activate. Click target for \"activate\" is misaligned.

**Category:** UI / UX"

gh issue create --title "HQ/NQ button – drag asymmetry" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** Can hold left-click and drag up to activate HQ, but not the opposite (drag down to deactivate). Should be able to hold click and switch back and forth continuously.

**Expected:** Bidirectional drag: up → activate, down → deactivate, both while holding.

**Category:** UI / UX"

gh issue create --title "HQ vs NQ spread – drastic difference" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** Noticeable difference in spread between HQ and NQ; HQ has much wider spread. Feels drastic for something labeled as a fidelity setting.

**Question:** Intentional for design/practical reasons?

**Category:** DSP / Design / Documentation"

gh issue create --title "Latency when turning knobs during playback" \
  --body "**Source:** Uno Rosengren (2026-02-26, v2.02.1)

**Description:** Delay when turning knobs during playback. Makes it harder to tune until it fits the instrument/track. Some lag when making changes during playback (optimization).

**Context:** User notes other paid 3rd-party plugins have same issue.

**Category:** Performance / UX"

echo ""
echo "Done. 6 issues created, 2 closed."
