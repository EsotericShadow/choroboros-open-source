#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

branch="codex/revised-audit-plans"

if git show-ref --verify --quiet "refs/heads/$branch"; then
    git switch "$branch"
else
    git switch -c "$branch"
fi

# Stage deletion of the old patch-level plans, but only if git knows about them
# or they still exist in the working tree.
for path in \
    docs/developer/PLAN_1_PRESET_ROUNDTRIP.md \
    docs/developer/PLAN_2_DSP_LOCK_DISCIPLINE.md \
    docs/developer/PLAN_3_REGRESSION_TESTS.md \
    docs/developer/PLAN_4_DEBUG_LOGGING.md \
    docs/developer/PLAN_5_FEEDBACK_OPTOUT.md \
    scripts/commit_audit_plans.sh
do
    if git ls-files --error-unmatch "$path" >/dev/null 2>&1 || [ -e "$path" ]; then
        git add -A -- "$path"
    fi
done

# Stage the revised architectural plans and this script only.
git add -- \
    docs/developer/PLAN_0_STATE_DOMAIN_BOUNDARIES.md \
    docs/developer/PLAN_1_CANONICAL_STATE_LAYER.md \
    docs/developer/PLAN_2_AUDIO_THREAD_OWNERSHIP.md \
    docs/developer/PLAN_3_TESTABLE_SERVICES.md \
    docs/developer/PLAN_4_DELETE_DEBUG_IO.md \
    docs/developer/PLAN_5_CONSENT_SERVICE.md \
    scripts/commit_revised_plans.sh

if git diff --cached --quiet; then
    echo "No staged plan changes to commit."
    exit 0
fi

git commit -m "Replace patch-level audit plans with architectural fix plans

Old plans tried to fix symptoms locally. The revised plans define state
domains, a canonical SoundState layer, audio-thread ownership for DSP
mutation, headless service extraction for testing, deletion of debug
file I/O, and a real consent boundary for analytics."

git push -u origin "$branch"

echo "Done -- revised plans committed and pushed to $branch."
