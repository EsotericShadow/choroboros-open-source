# Documentation Audit — Choroboros v2.04

Audited against source code on 2026-03-17. Each finding is cross-referenced with the specific source file that proves the mismatch.

---

## HIGH SEVERITY — Will mislead users

### ~~1. README.md — Rate range~~ RETRACTED

**Original claim:** Rate range was wrong (docs say 0.01–20 Hz, raw APVTS max is 10.0 Hz).
**Correction:** The raw APVTS range (`RATE_MAX = 10.0f`) is NOT the user-facing range. There is a `mapTunedValue` tuning layer that remaps the raw range to the display range. The UI tooltip confirms "0.01 Hz to 20 Hz." The README was correct; the audit was wrong. **No change needed.**
**Lesson:** Always trace through the full parameter mapping chain — don't stop at `createParameterLayout()`.

---

### 2. USER_MANUAL.md §3.3 — Offset parameter is completely wrong

**Docs say:** Offset is described as "Center Delay" time, with ranges in milliseconds: "0–5 ms = flanger territory," "10–25 ms = true chorus," "30–60 ms = slapback."
**Code says:** Offset is **LFO phase offset in degrees** (0–180°). `PluginProcessor.h`: range is `0.0f, 180.0f`, default `90.0f`. `ChorusDSPProcess.cpp:315–316`: the value is converted from degrees to radians (`phaseOffsetDeg * pi / 180`) and used to set the stereo LFO phase relationship, not a delay time.
**Fix:** Rewrite §3.3 entirely. Offset controls the LFO phase offset between left and right channels (0–180°). At 0° the channels sweep in sync (narrower stereo); at 180° they sweep in opposition (widest stereo image). Default is 90°. This is *not* a delay time control.

---

### 3. USER_MANUAL.md §3.5 — Color parameter description is misleading

**Docs say:** Color is described as a universal filter/EQ control: "0% = dark (heavy low-pass)," "50% = neutral (flat)," "100% = bright (extreme high-pass + high-shelf boost)."
**Code says:** Color is **engine-specific** and means completely different things per engine. Per `ENGINE_CONTENT_CORRECTED.md` and the actual DSP code:
- Green: Bloom (thickness + damping, no saturation)
- Blue: Focus (clarity + presence, no saturation)
- Red NQ: Saturation drive
- Red HQ: Tape tone + drive
- Purple: Warp/orbit shape
- Black: Modulation intensity / ensemble spread

The manual's "dark/neutral/bright" framing only loosely applies to Green/Blue and is outright wrong for Red (saturation), Purple (warp), and Black (mod intensity).
**Fix:** Rewrite §3.5 to explain per-engine Color behavior, or at minimum add a table of what Color does per engine. The README already has this right.

---

### 4. USER_MANUAL.md §4.2 — Blue engine NQ algorithm is wrong

**Docs say:** "Blue uses insanely high-fidelity 5th-order Lagrange math combined with steep 8-pole IIR tracking filters."
**Code says:** Blue NQ uses **Cubic interpolation** (`ChorusCoreCubic`), Blue HQ uses **Thiran allpass** (`ChorusCoreThiran`). 5th-order Lagrange is Green HQ, not Blue. No "8-pole IIR tracking filters" appear in the Blue core code.
**Fix:** Correct to: NQ uses cubic interpolation for clean, transparent chorus; HQ uses 5th-order Thiran allpass for phase-accurate stereo imaging.

---

### 5. USER_MANUAL.md §5 — Black HQ algorithm is wrong

**Docs say:** "3-Voice Tap Ensemble array"
**Code says:** `ChorusCoreLinearEnsemble.h:24` — "linear interpolation with **dual-tap** ensemble blend." The class has two delay lines (`delayLineA` and `delayLineB`), not three.
**Fix:** Change to "2-Voice (Dual-Tap) Ensemble" or "Dual-Tap Linear Ensemble."

---

### 6. USER_MANUAL.md §4.1 — Green engine "Envelope Follower" claim

**Docs say:** "Green is the only engine with an internal dynamic envelope follower. It listens to how loud you play."
**Code says:** No envelope follower exists in the Green engine source code (`Cores/green_engine_classic/`). The `envelope` references in the codebase are all in the output peak catcher (`ChorusDSPProcess.cpp:187–197`), which applies to *all* engines equally. The Dev Panel has a tutorial topic called "envelope" but it's a general DSP concept tutorial, not a Green-specific feature.
**Fix:** Remove the envelope follower claim from §4.1. If this was a planned feature that was never implemented, note that. If it was removed at some point, update accordingly.

---

### 7. README.md — Preset order doesn't match code

**Docs say:** Presets listed as: 1. Classic, 2. Modern, 3. Vintage, 4. Psychedelic, 5. Core, 6. Duck, 7. Ouroboros
**Code says:** `PluginProcessor.cpp:1065–1201`: index 0 = Classic (Green), index 1 = **Vintage (Red)**, index 2 = **Modern (Blue)**, index 3 = Psychedelic, index 4 = Core, index 5 = Duck, index 6 = Ouroboros.
**Fix:** Swap positions 2 and 3 in the README preset list to match code order: Classic, Vintage, Modern, Psychedelic, Core, Duck, Ouroboros.

---

### 8. README.md — Preset HQ states are missing/wrong

**Docs say:** No HQ states listed for the first 5 presets. Duck described as "Purple HQ" (correct), Ouroboros as "Blue HQ" (correct).
**Code says:**
- Classic (Green): **NQ** (HQ = 0.0f)
- Vintage (Red): **HQ** (HQ = 1.0f)
- Modern (Blue): **HQ** (HQ = 1.0f)
- Psychedelic (Purple): **NQ** (HQ = 0.0f)
- Core (Black): **HQ** (HQ = 1.0f)
**Fix:** Add HQ/NQ state to each preset listing.

---

### 9. README.md — Contact email mismatch

**Docs say (Support section):** `Greenalderson@gmail.com`
**Code says (AboutDialog.cpp):** `info@kaizenstrategic.ai`. The EULA, changelog, and feedback dialog all use `info@kaizenstrategic.ai`.
**Fix:** Update the README contact email to `info@kaizenstrategic.ai` (or keep both if the Gmail is intentionally listed for a different purpose).

---

### 10. USER_MANUAL.md §2/§4 — Engine selector UI location is wrong

**Docs say:** "clicking the glowing colored orb at the top center of the UI" and §7 says "hover over the tiny text reading DEV."
**Code says (CHANGELOG.md v2.04-dev):** Engine selector moved to top-left (Y=5). DEV button replaced with icon in TopBarDrawer (animated drawer, top-right). The "glowing orb" and "tiny DEV text" descriptions are stale.
**Fix:** Update §4 and §7 to reflect the current UI layout: engine selector is in the header bar (top area), and the Dev Panel opens via an icon in the animated drawer (top-right).

---

## MEDIUM SEVERITY — Incomplete but not actively wrong

### 11. USER_MANUAL.md §6 — Default Offset value is wrong

**Docs say:** "factory state… Offset at 15ms"
**Code says:** Default offset is **90°** (`PluginProcessor.cpp:2307`). The "15ms" value doesn't appear anywhere in the parameter system.
**Fix:** Change to "Offset at 90°."

---

### 12. README.md — DAW compatibility understates AAX

**Docs say (System Requirements):** "Any DAW that supports VST3 or AU plugins"
**Code says:** AAX is a supported format (CMakeLists.txt enables it, installers include it).
**Fix:** Change to "Any DAW that supports VST3, AU, or AAX plugins."

---

### 13. USER_MANUAL.md §1 — Claims AAX is available

**Docs say:** "Choroboros comes compiled as an AU, VST3, AAX, and Standalone application for macOS and Windows."
**Code says:** This is actually correct per the build config, but it's worth noting that the README's Windows format list says only "VST3, Standalone (Windows)" while the manual claims AAX on Windows too. One of these is wrong — verify whether Windows AAX builds are actually produced and shipped.
**Needs verification:** Is AAX built for Windows, or only macOS?

---

### 14. USER_MANUAL.md §3.4 — Width range is understated

**Docs say:** Width goes to "100% (Anti-Phase)" as the maximum.
**Code says:** Width range is 0–200% (`PluginProcessor.cpp:2313`, max = 2.0f). The README correctly says "0–200%."
**Fix:** The manual should note that Width goes to 200%, not 100%. At 100% the LFOs are in quadrature (90° apart); at 200% they go further.

---

### 15. KNOWN_ISSUES.md — Missing v2.04 UI changes

**Docs say:** "Click the DEV button (top-left of the plugin) to open."
**Code says:** DEV button is now an icon in a TopBarDrawer (animated, top-right per changelog). The text "DEV" was replaced with a PNG icon.
**Fix:** Update the Dev Panel access description.

---

## LOW SEVERITY — Cosmetic or minor

### 16. USER_MANUAL.md §7 — "119 internal float parameters"

**Docs say:** "exposes all 119 internal float parameters"
**Needs verification:** This number may have changed with v2.04 additions (preset manager, session logging, TopBarDrawer state, etc.). Consider removing the specific count or verifying it.

---

### 17. USER_MANUAL.md — Missing features from v2.04

The manual doesn't cover any of the v2.04 additions: preset browser UI, TopBarDrawer, session logging, crash reporter, send-to-developer feedback, icon buttons, Thiran output lowpass, or the revised gain staging. These are all documented in the CHANGELOG but not in the user-facing manual.

---

### 18. README.md — "Formats: VST3, AU, AAX, Standalone (macOS); VST3, Standalone (Windows)"

This line in Technical Details should be checked: does Windows get AAX builds? If yes, add it. If no, the line is correct as-is but the USER_MANUAL (finding #13) needs updating.

---

## VERIFIED CORRECT — No changes needed

- README.md engine descriptions (Green/Blue/Red/Purple/Black) and algorithms ✓
- README.md Color semantics per engine ✓
- README.md preset parameter values for Classic, Psychedelic, Core, Duck, Ouroboros ✓
- README.md installation paths (macOS VST3, AU, AAX, Standalone; Windows VST3, Standalone) ✓
- README.md system requirements (macOS 10.13+, Windows 10+) ✓
- README.md version "2.04", company "Kaizen DSP", copyright "Kaizen Strategic AI Inc." ✓
- README.md rate sync subdivisions (Straight/Triplet/Dotted — no Swing) ✓
- KNOWN_ISSUES.md HQ vs NQ algorithm table ✓
- KNOWN_ISSUES.md system requirements ✓
- ENGINE_CONTENT_CORRECTED.md — fully aligned with code ✓
- TECHNICAL_SPECS_CORRECTIONS.md — all corrections are accurate ✓
