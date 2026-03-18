# TODO: AAX Plugin — Path to Pro Tools Approval

Choroboros currently builds VST3, AU, and Standalone. This document outlines the steps to add AAX format and reach commercial Pro Tools distribution.

**Key facts:**
- JUCE **bundles** the AAX SDK — no separate download needed for building
- Unsigned AAX plugins can be tested with **Pro Tools Developer** (free, from Avid)
- Commercial distribution requires **PACE code-signing** (iLok USB key, NDA, Avid approval)

---

## Part 1: Manual — You Must Do These

### 1.1 Avid Developer Account Setup

- [ ] **Create Developer Account** — Log in to Avid.com → "Linked Avid Accounts" → "Create Account" next to "Developer Account"
- [ ] **Accept AAX SDK license** — Go to [developer.avid.com/aax](https://developer.avid.com/aax) → "Download Evaluation Toolkit" → accept click-through license (takes you to developer downloads)
- [ ] **Create iLok account** — [ilok.com](https://www.ilok.com) (required for Pro Tools and later for signing)

### 1.2 Pro Tools Developer (for testing unsigned plugins)

- [ ] **Download Pro Tools Developer** — From Avid developer downloads (after accepting SDK license)
- [ ] **Obtain Pro Tools Developer license** — Avid provides a free license for approved developers; may require email to audiosdk@avid.com if not auto-granted
- [ ] **Install Pro Tools Developer** on your Mac
- [ ] **Install iLok License Manager** — Required to activate Pro Tools license (iLok Cloud or USB key)

### 1.3 Commercial Signing (when ready to distribute)

- [ ] **Email audiosdk@avid.com** — Request commercialization info: PACE EDEN Tools access, signing process
- [ ] **Obtain iLok USB key** — Physical key required for PACE code-signing (or explore Cloud 2 Cloud for CI)
- [ ] **Sign PACE NDA** — PACE will send NDA; sign and return
- [ ] **Receive PACE EDEN Tools** — Avid/PACE provide free EDEN license for approved developers (normally ~$500/year)
- [ ] **Read PACE documentation** — ~80 pages of confidential signing instructions
- [ ] **Code-sign the .aaxplugin** — Using PACE EDEN Tools with your iLok (process is NDA-covered)

### 1.4 Post-Signing Verification

- [ ] **Test signed plugin in commercial Pro Tools** — Verify it loads; if it doesn’t, signing failed
- [ ] **Install to correct path** — `/Library/Application Support/Avid/Audio/Plug-Ins` (macOS)

---

## Part 2: Orchestrated — AI Agents Can Do These

### 2.1 Add AAX to CMake Build

- [ ] **Edit CMakeLists.txt** — Add `AAX` to `FORMATS` in `juce_add_plugin`:
  ```cmake
  FORMATS AU VST3 AAX Standalone
  ```
- [ ] **Set AAX category** — Choroboros is a chorus (modulation effect). Add to `juce_add_plugin`:
  ```cmake
  AAX_CATEGORY AAX_ePlugInCategory_Modulation
  ```
- [ ] **Verify JUCE bundled SDK** — JUCE includes AAX SDK at `JUCE/modules/juce_audio_plugin_client/AAX/SDK`; no `juce_set_aax_sdk_path` needed unless using external SDK

### 2.2 Build & Fix Compilation

- [ ] **Run CMake configure** — `cmake -B build -S .`
- [ ] **Build AAX target** — `cmake --build build --target Choroboros_AAX`
- [ ] **Fix any compile/link errors** — Resolve architecture, missing symbols, or JUCE version quirks
- [ ] **Add post-build copy** (optional) — Copy `.aaxplugin` to `/Library/Application Support/Avid/Audio/Plug-Ins` for quick testing

### 2.3 Regression & Sanity Checks

- [ ] **Run regression tests** — Ensure `ChoroborosRegressionTests` still pass
- [ ] **Verify VST3/AU unchanged** — Build all formats, confirm no regressions

### 2.4 Documentation & Scripts

- [ ] **Update DISTRIBUTION.md** — Add AAX install path and instructions
- [ ] **Update package.sh** — Include AAX in release package when present
- [ ] **Add AAX to README** — Installation section for AAX users
- [ ] **Create docs/AAX_BUILD.md** — Short build instructions for AAX

### 2.5 Packaging (for distribution)

- [ ] **Include .aaxplugin in release ZIP** — After PACE signing, add to distribution package
- [ ] **Update INSTALL.txt** in package — AAX install path and Pro Tools rescan steps

---

## Sequence Overview

| Phase | Manual | Orchestrated |
|-------|--------|--------------|
| **1. Build** | Create Dev Account, accept SDK license, iLok account | Add AAX to CMake, build, fix errors |
| **2. Test** | Install Pro Tools Developer, get license, test unsigned plugin | (Optional) post-build copy script |
| **3. Sign** | Email audiosdk@avid.com, iLok key, NDA, EDEN Tools, sign plugin | — |
| **4. Ship** | Verify in commercial Pro Tools | Update docs, package script |

---

## References

- [AAX SDK](https://developer.avid.com/aax)
- [Avid Quick Start](https://developer.avid.com/cert/quickStart.html)
- [JUCE CMake API — FORMATS, AAX_CATEGORY](https://docs.juce.com/master/CMake%20API.html)
- Contact: **audiosdk@avid.com** (commercialization, PACE access)
