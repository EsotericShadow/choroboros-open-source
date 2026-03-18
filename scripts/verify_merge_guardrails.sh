#!/bin/bash
# Verify merge guardrails before pushing to GitHub.
# Run from project root: ./scripts/verify_merge_guardrails.sh

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

FAIL=0

echo "=== Choroboros Merge Guardrails ==="
echo

# 1. Assets in CMakeLists exist on disk
echo "[1/5] Checking CMakeLists assets exist..."
ASSETS=$(sed -n '/juce_add_binary_data/,/^)/p' CMakeLists.txt | grep -oE '(Assets/[^ ]+\.(png|ttf)|EULA\.md)' || true)
for path in $ASSETS; do
    if [[ ! -f "$path" ]]; then
        echo "  FAIL: Missing asset: $path"
        FAIL=1
    fi
done
[[ $FAIL -eq 0 ]] && echo "  OK"

# 2. Regression tests build and pass
echo "[2/5] Building regression tests..."
if ! cmake -B build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1; then
    echo "  FAIL: CMake configure failed"
    FAIL=1
elif ! cmake --build build --target ChoroborosRegressionTests >/dev/null 2>&1; then
    echo "  FAIL: Build failed"
    FAIL=1
else
    echo "  OK"
fi

# 3. Run regression tests
echo "[3/5] Running regression tests..."
if [[ -x ./build/ChoroborosRegressionTests ]]; then
    if ! ./build/ChoroborosRegressionTests 2>&1 | grep -q "PASS: All regression tests passed"; then
        echo "  FAIL: Regression tests did not pass"
        FAIL=1
    else
        echo "  OK"
    fi
else
    echo "  SKIP: ChoroborosRegressionTests not built"
fi

# 4. JUCE present (required for build)
echo "[4/5] Checking JUCE..."
if [[ ! -d JUCE ]] && [[ ! -L JUCE ]]; then
    echo "  WARN: JUCE/ not found. Clone or add submodule before merge."
elif [[ -L JUCE ]]; then
    echo "  WARN: JUCE is a symlink. Consider submodule for GitHub."
else
    echo "  OK"
fi

# 5. DISTRIBUTION.md for CI
echo "[5/5] Checking DISTRIBUTION.md..."
if [[ ! -f DISTRIBUTION.md ]]; then
    if [[ -f docs/archive/DISTRIBUTION.md ]]; then
        echo "  WARN: DISTRIBUTION.md at docs/archive/ only. CI expects at root."
    else
        echo "  WARN: DISTRIBUTION.md not found"
    fi
else
    echo "  OK"
fi

echo
if [[ $FAIL -eq 1 ]]; then
    echo "=== FAILED: Fix issues before merge ==="
    exit 1
fi
echo "=== All guardrails passed ==="
