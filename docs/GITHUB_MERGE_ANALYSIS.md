# GitHub Merge Analysis – Regression Prevention

## Summary

| Aspect | Remote (origin/main) | Local (main) |
|--------|---------------------|--------------|
| **Files** | 3,907 | 156 |
| **JUCE** | Vendored (in repo) | Gitignored (symlink) |
| **Version** | 1.0.1 | 2.01-beta |
| **Engines** | 4 colors, 8 algorithms | 5 colors, 10 algorithms |
| **Black engine** | ❌ Missing | ✅ Present |
| **DevPanel** | ❌ Missing | ✅ Present |
| **DefaultsPersistence** | ❌ Missing | ✅ Present |
| **Asset naming** | `*_panel_background.png` | `*_backpanel.png` |
| **Spritesheets** | ❌ None | ✅ Many (knobs, mix, etc.) |
| **GitHub Actions** | ✅ 3 workflows | ❌ None |
| **Docker** | ✅ Linux build | ❌ None |
| **GPL compliance** | ✅ Docs | ⚠️ LICENSE only |

---

## What We'd LOSE by Force-Push (Replace Remote)

- **GitHub Actions** – build-all-platforms, build-linux, build-windows
- **Docker** – Linux build support
- **GPL compliance docs** – GPL_COMPLIANCE_*.md, GITHUB_ACTIONS_*.md
- **Vendored JUCE** – clone-and-build would stop working (local has JUCE gitignored)

---

## What We'd LOSE by Merge (Remote into Local)

- Nothing critical. Local is the canonical source; remote adds CI/CD and docs we want to keep.

---

## Regression Risks

### 1. Build Failure After Push

**Risk:** Local repo has `JUCE/` in `.gitignore`. Remote has JUCE vendored. If we push local as-is, `cmake -B build` will fail for anyone who clones (no JUCE).

**Options:**
- **A)** Add JUCE as git submodule: `git submodule add https://github.com/juce-framework/JUCE.git JUCE`
- **B)** Remove JUCE from `.gitignore` and vendor JUCE (copy from `/Users/main/JUCE` into repo) – large (~3.7k files)
- **C)** Keep JUCE gitignored, add `docs/BUILD.md` with: "Clone JUCE into project root: `git clone https://github.com/juce-framework/JUCE.git`"

### 2. GitHub Actions Would Break

Remote workflows assume:
- `add_subdirectory(JUCE)` – JUCE in repo
- Old CMake structure (no black engine, no DefaultsPersistence)
- Old asset paths (`*_panel_background.png`)

If we merge, we must update workflows to match local structure and either include JUCE or use a JUCE submodule.

### 3. Asset Path Mismatch

Remote uses `*_panel_background.png`; local uses `*_backpanel.png`. Code references `*_backpanel.png`. No regression if we keep local.

---

## Recommended Merge Strategy

1. **Create merge branch:** `git checkout -b merge-from-remote`
2. **Merge remote:** `git pull origin main --allow-unrelated-histories`
3. **Resolve conflicts** – keep local for: Source/, Assets/, CMakeLists.txt, .gitignore
4. **Keep from remote:** `.github/workflows/`, `Dockerfile.linux`, GPL docs
5. **Update workflows** – point to new structure, add JUCE submodule or document build steps
6. **Add JUCE submodule** (recommended) so CI and developers can build without manual setup

---

## Files to Keep from Remote (No Local Equivalent)

```
.github/workflows/build-all-platforms.yml
.github/workflows/build-linux.yml
.github/workflows/build-windows.yml
Dockerfile.linux
Dockerfile.linux-test
.dockerignore
GPL_COMPLIANCE_*.md
GITHUB_ACTIONS_*.md
GITHUB_BILLING_FIX.md
```

---

## Files Where Local Wins (Newer)

- All of `Source/`
- All of `Assets/`
- `CMakeLists.txt`
- `.gitignore`
- `README.md` (update to 5 engines, 10 algorithms)

---

## Action Plan (Safe Merge)

### Step 1: Add JUCE as submodule (before merge)

```bash
# Remove symlink if present
rm -f JUCE  # or: rm JUCE if it's a symlink

# Add JUCE submodule (pins a specific version for reproducible builds)
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git submodule update --init --recursive

# Remove JUCE/ from .gitignore (submodule is tracked)
```

### Step 2: Merge remote

```bash
git pull origin main --allow-unrelated-histories
# Resolve conflicts: keep local for Source/, Assets/, CMakeLists.txt
# Keep remote for .github/, Dockerfile*, GPL_*.md
```

### Step 3: Update GitHub Actions

- Workflows reference old paths; update to match local structure
- Ensure JUCE submodule is checked out in CI: `git submodule update --init --recursive`

### Step 4: Verify build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## See Also

- **docs/GITHUB_MERGE_GUARDRAILS.md** – Deep analysis, BinaryData verification, regression test notes, CI requirements
- **scripts/verify_merge_guardrails.sh** – Pre-merge verification script
