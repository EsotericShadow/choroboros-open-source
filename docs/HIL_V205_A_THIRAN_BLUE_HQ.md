# HIL-A — Blue HQ Thiran buzzing and distortion

## Agent references (repo-local)
- **DSP pitfalls / RT / UB:** workspace skill bundle **`assets/skills/deep-dsp-quirks.skill`** (ZIP: extract or `unzip -p … deep-dsp-quirks/SKILL.md`). Covers denormals (S2), fast-math / NaN (S1), float→int casts (C6), buffer init (C4), thread/allocation issues (D*), etc. **Repo `.claude/`** here is only `settings.local.json` — skills live under **`assets/skills/`**, not under `.claude`.

## Question
Is residual buzzing **core-local** to `ChorusCoreThiran`, driven by **shared HQ/NQ core switching** (`ChorusDSP::switchCore` / crossfade buffers), or by **interaction** between the fractional-delay path and outer DSP?

**Shipping Blue HQ Thiran (2026):** the live core is **`ChorusCoreThiran`** — **integer delay line** + **5th-order Thiran allpass** (Laakso §3.4.3–3.4.4) + **dual-path sin²/cos²** on **⌊T⌋** hops (see [`THIRAN_CANONICAL_SPEC.md`](THIRAN_CANONICAL_SPEC.md) §0). The attempt log below is **historical** for earlier experiments (including `legacy_inactive/`); it is **not** a line-by-line spec of the current ship file.

---

## Attempt log — **legacy inactive core only** (do not re-apply without re-reading outcomes)

The following experiments refer to **`ChorusCoreThiranLegacyInactive`**, not the shipping hardened core.

| ID | Change | Intended effect | Actual outcome | Verdict |
|----|--------|-----------------|----------------|---------|
| A1 | Per-sample full `computeCoefficients` on active allpass | Track fractional `D` exactly | DFII-T fights coeffs → zipper / droney roughness | **Reverted** → use smoothed `a[]` toward target |
| A2 | 1 ms → **6 ms** sin²/cos² integer crossfade + state copy on hop | Mask coeff/tap discontinuity | Much lower boundary THD vs 1 ms | **Keep** |
| A3 | **Mid-xfade abort** (`xfadeCounter=0`) + same-sample **chained** integer crossings | Catch up when delay jumps | Hard blend jumps → **bitcrush / heavy alias-like motion** | **Reverted** |
| A4 | Per-sample host **centre ms** into Thiran’s inner centre smoother | Finer centre tracking | Stacked two modulation paths → **non-chorus / crush** | **Reverted** — Thiran uses **block-constant** centre |
| A5 | One-pole **`smoothCoeffsTowardD`** on fade-in branch, **including during** integer xfade (when `trackAp.intDelay == floor(delay)`) | Avoid frozen `frac` for whole ~6 ms xfade on growing path | Reduces droning zipper vs skipping smooth during xfade | **Keep** |
| A6 | Thiran-only **τ:** delay smooth **24 ms**, coeff **14 ms**, centre **5 ms**, wet LP **~9.5 kHz** | Tame HF grain under Color / Focus | Subjective | **Keep** until re-tuned |
| A7 | **Mid-xfade abort** when `floor(smoothedDelay) != fadeInAp.intDelay` | Fix wrong tap + coeff freeze | **Bitcrushing waves** | **Reverted** |
| A8 | **Mid-xfade fade-in realignment** (snap `intDelay` + coeffs on fade-in only) | Second integer hop during blend | User: still harsh; extra snaps | **Reverted** |
| A9 | Thiran **mono LFO** (`lfoBuffer` only for L and R) | Sync integer hops L/R | User: **long pulsing “alarm” crush, left-only** (regression) | **Reverted** |
| A10 | Coeff τ **14 ms → 4 ms** | Faster coeff tracking | Combined with A9 user still unhappy; **reverted** with A9 | **Reverted** |
| A11 | **Delay line: read taps then write** `buffer[writePos]` (was write then read) | Strictly causal bulk read — avoid any fold-in of current `in` with ambiguous index math / crossfade branches | **In trial** | |
| A12 | **Prepare-only tuning:** wet LP **~8.8 kHz**; delay τ **30 ms**; integer xfade **4.5 ms** | Mitigate deferred second hop / HF hash | User: **worse**, steady “chrrrr” — **reverted** to 9.5 kHz / 24 ms / **6 ms** | **Reverted** |
| A13 | **During integer xfade**, clamp `smoothedDelay` to **\[fadeIn.intDelay, fadeIn+1−ε\]** (bounded by `maxDelay`) while `xfadeCounter` > 0 | Floor stays on the frozen fade-in tap so `smoothCoeffsTowardD` runs and reads stay coherent (not A7 mid-abort) | User: **low choppy droning** at low settings — **reverted** (fought LFO) | **Reverted** |
| A14 | **Deferred hop** `pendingIntHopTarget` (latest floor while `xfadeCounter`>0) drained when xfade completes; **intra-xfade** coeff path uses fade-in `intDelay` + `jlimit` frac vs `smoothedDelay` | Catch up multi-hop without A13 pin; keep `smoothCoeffsTowardD` active during blend | Pending beta / Reaper HQ listen | **In trial** |

**Open tension (legacy only):** **A14** queues a floor hop during xfade and applies on completion; **intra-xfade** fractional coeffs align to the frozen fade-in tap (clamped frac). Chained hops faster than xfade length still **stack** in `pendingIntHopTarget`. **Shipping (2026):** V&L **cold-start** state alignment runs on **probe 3** coeff snaps only (see [`THIRAN_VL_STATE_PORT.md`](THIRAN_VL_STATE_PORT.md) Phase 3); applying the same on every smoothed fractional step remains optional R&D.

---

## Live paths — **shipping** `ChorusCoreThiran` (Blue HQ / `thiran` token)

**Reference-model Thiran chorus** (see [`THIRAN_CANONICAL_SPEC.md`](THIRAN_CANONICAL_SPEC.md) §0): **integer** delay-line tap (no Lagrange) → **5th-order Thiran allpass** with **Laakso §3.4.3** \(a_k\) and **§3.4.4** interior **\(D\)** mapped from **`T − ⌊T⌋`** via **`D = (N−½)+f`**; **dual allpass** + **sin²/cos²** ~**6 ms** on **⌊T⌋** hops; **one-pole** delay slew (~**24 ms**) and coeff tracking (~**14 ms**); wet **~9.5 kHz** one-pole (probe **1** bypass).

**Implementation** is **not** copied from `legacy_inactive/` — fresh code in `ChorusCoreThiran.{h,cpp}`.

### 1) Core (`ChorusCoreThiran::processDelay`)
- **LFO:** L = `lfoBuffer`, R = `cosBuffer`.
- **Inner centre:** block-constant **`currentCentreDelayMs`** into **`smoothedCentreDelay`** (~**5 ms** τ); probe **13** snaps per sample.
- **Guard:** **N+2** samples.

### 2) Shared HQ/NQ switching
- **Warmup** + **sin/cos** between wet cores.

### 3) Interaction surfaces
- **Probe 14:** LFO depth block-constant. **Probe 5:** Blue Focus wet bypass.

## Assessment (current ship)
- Artifacts are dominated by **integer-hop rate**, **coeff smoothing vs hop length**, and **recursive FD under LFO** — use probes and HIL captures; compare to **canonical spec** and literature expectations (JOS HF group-delay roll-off).

## Implementation status
**Shipping:** reference topology in `ChorusCoreThiran.{h,cpp}`; goldens on **`computeCoefficientsForTests`**. **Legacy inactive:** archaeology only.

---

## Reduction probe (dev console)

**Goal:** Hear which single subsystem correlates with the artifact by **removing exactly one** processing stage at a time (not cumulative).

**Commands:** `probe <0..18>` applies one bypass/experiment; **`probe list`** prints all mode numbers, **signal-order** lines, descriptions, and a short note on **masking vs fixing** (no audio change). Values live in `RuntimeTuning::thiranReductionProbe` on the **currently selected engine colour and HQ/NQ slot** (except **`probe 0`** clears **both** NQ and HQ for that colour). Tuning is pushed immediately via `publishActiveRuntimeTuningSnapshotNow`. `dump <color>` lists `thiran_reduction_probe` (use a **space**: `dump blue`).

**Per-slot storage:** For modes **1–14**, only the **active** HQ or NQ row is updated. **`probe 0`** clears **both** rows for the current colour.

**Reading results:** The mode that **most** reduces the buzz implicates that block for a real fix. If **no** mode removes it, the cause is likely **inside** the delay core + LFO topology (not separable with these switches), **host / oversampling / buffer-size** interaction, or **auditory masking** (a bypass changes level or spectrum so the buzz is less obvious without removing the underlying mechanism). Compare at **matched loudness** (e.g. slight output trim) when unsure.

| Mode | What is bypassed | Notes |
|------|------------------|--------|
| **0** | *(none)* | Normal; clears both NQ+HQ probe for current colour |
| **1** | Thiran **wet output** one-pole (~9.5 kHz) | Raw allpass sum before output LP |
| **2** | Thiran **delay-line slew** bypass | `smoothedDelay = target` each sample |
| **3** | Thiran **fractional coeff** snap | Full `computeCoefficients` when tracking branch matches; **V&L (ICMC 1995) cold-start** state on recent allpass-input ring (cleared each integer hop) |
| **4** | Thiran **integer** sin²/cos² crossfade bypass | Instant hop (`xfadeCounter = 0`) |
| **5** | Blue **Focus** wet (`processBlueFocusWet`) | Skips presence/tilt on wet; Thiran unchanged |
| **6** | **`processPreEmphasis`** before delay core | Dynamic wet-input shaping off |
| **7** | **Output LPF** after chorus | In `ChorusDSP::process` |
| **8** | **Stereo width** (`processWidth`) | M/S after chorus |
| **9** | **Safety lookahead limiter** | After width; may clip when bypassed |
| **10** | **Input HPF** | Before `processChorus`; full band into chorus |
| **11** | **Post-mix wet peak catch** (`processOutputPeakCatch`) | After `dryWet.mixWetSamples` |
| **12** | **Output trim** multiply (`processOutputTrim`) | `smoothedOutputTrimGain` still advances via `skip` |
| **13** | Thiran **inner** centre snap to block target | Per-sample `smoothedCentreDelay =` block ms |
| **14** | **LFO depth** block-constant | `ChorusDSPProcess`: skips `oscVolume` `SmoothedValue` `multiplyBy` on sin/cos buffers (depth = `currentDepth * 0.5f` for the block) |
| **15** | **EXP:** zero incoming branch IIR state on integer hop | Replaces `newActive.state = oldActive.state` with zero-state handoff |
| **16** | **EXP:** freeze coeff updates inside integer bucket | No intra-bucket `smoothCoeffsTowardD`; recompute only when tracked `intDelay` bucket changes |
| **17** | **EXP:** pin integer-delay bucket (no hop scheduler/xfade) | Disables integer retargeting (`beginIntegerHop`/pending) and keeps one active branch/tap bucket while still modulating fractional `D` within that pinned bucket |
| **18** | **EXP:** hop hysteresis + minimum dwell (diagnostic) | Keeps normal hop/xfade path but only accepts bucket changes after boundary hysteresis and a minimum dwell interval |

**Workflow:** `probe list` for the cheat sheet. Try **10 → 12** if outer-chain suspects remain; **14** isolates LFO depth smoothing (**2–4, 13** are legacy-only on hardened). If the goal is **core DSP** rather than more listening triage, go straight to **Phase B** below; use **`probe 0`** between A/Bs and when finished.

**Lab note (Apr 2026):** Probes **1–4** and **13** isolate the Thiran core; **14** LFO depth; **5–12** outer chain. **Phase B** still applies for spectral evidence vs **Rate/Depth**.

---

## Phase B — Core DSP design + measurement (active)

**Intent:** Stop expanding the probe matrix for listening-only triage. Align engineering with literature: time-varying **recursive** fractional delay at **LFO rate** needs either **state-consistent coefficient updates** (Välimäki–Laakso–class ideas), a **less aggressive** fractional mechanism (lower order, FIR/Lagrange on the modulated tap), or a **topology** that feeds Thiran a **slower** effective delay law.

### B0 — Code anchors (where work lands)

| Area | File / symbol | Notes |
|------|----------------|--------|
| Thiran core | `ChorusCoreThiran.cpp` — `processDelay`, `processAllpass`, `thiranDFromTotalDelay` | Integer line + dual AP + smoothing (see §0). |
| Coefficients | Same — `computeCoefficients`, `computeCoefficientsForTests` | Golden tests lock **`a[]`**. |
| Legacy | `legacy_inactive/ChorusCoreThiranLegacyInactive.cpp` | Older experiment; not ship. |
| Centre vs depth, LFO, block centre | `ChorusDSPProcess.cpp` — `processChorusParameters`, `processChorusLFO` | Shared with all cores; depth law / smoothing experiments. |

### B1 — Measurement (do before large rewrites)

Goal: separate **rate‑locked spectral lines** (modulation IM / zipper harmonics) from **broad hash**, and catch **buffer-size** dependence.

1. **Host / buffer matrix (same preset, same level)**  
   - Two hosts minimum (e.g. Live + Logic, or + Reaper).  
   - Block sizes **32 / 64 / 128 / 256 / 512** (and 96/192 if the host allows).  
   - Note **oversampling** on/off if the host adds plugin OS.  
   - **Log:** host, buffer, OS, sample rate, engine Blue HQ, representative **Rate** and **Depth** (and Color if relevant).

2. **Rate vs spectrum**  
   - Slow **Rate** sweep while watching a **high‑res FFT** (plugin analyzer or external).  
   - Mark whether peaks **track** \(f_\mathrm{LFO}\) and **integer multiples** (suggests modulation‑locked behaviour in the FD path) vs **fixed** spectral tilt (more EQ/masking).

3. **Depth at fixed Rate**  
   - Repeat at **low / mid / high** Depth; note whether grain **scales** ~linearly with depth (consistent with **\(D(t)\)** amplitude) or **saturates** (limiter / soft clip elsewhere).

4. **Wet null (optional)**  
   - Mix **100% wet**, match level roughly to dry, invert polarity against **dry** (or use a nulling bus). Residual often highlights **non‑linear / time‑varying** components without room wash.

5. **Capture**  
   - Short **48 kHz / 24‑bit** renders of **sine** and **bright** sources per matrix cell; file names include buffer + host. Avoid relying on memory alone for A/B after DSP changes.

**Pass/fail for “host artefact”:** If the same **subjective** zipper appears across hosts and scales with **Rate/Depth** but **not** with buffer size, weight toward **core DSP**. If it **strongly** depends on buffer or one host, weight toward **scheduling / denormals / channel strip** and still fix core if obvious on anechoic renders.

### B2 — DSP design options (recommended order)

1. **Slower effective delay law (topology)**  
   - Idea: Thiran sees **slower** variation in the quantity that maps to **fractional \(D\)** (e.g. more **centre** smoothing, less **depth**‑driven fractional swing, or explicit **two‑time‑constant** sum — **without** repeating A4’s failed “per‑sample centre into inner smoother” without a full redesign).  
   - **Pros:** Can preserve **5th‑order** character when quiescent. **Cons:** Changes **chorus width/feel**; must respect A4 lessons.

2. **Lower Thiran order (e.g. 3 or 1)**  
   - **Pros:** Fastest experiment matching JUCE’s warning (1st‑order Thiran exists for a reason). **Cons:** HF phase error vs today’s HQ target; may need **compensating** EQ or acceptance of softer high band.

3. **Slower effective delay law** (split topology before Thiran)  
   - Literature option when recursive FD is still too rough at chorus rates.  
   - **Cons:** changes width/feel; needs its own design note.

4. **Välimäki–Laakso‑class state handling on fractional steps**  
   - Apply a **state update** when **`a[]`** move (not only output crossfade on **integer** hops). This is the **heaviest** but most principled fix for zipper from **coefficient trajectories**.  
   - **Pros:** Keeps high‑order IIR when \(D\) is slow enough. **Cons:** Paper‑accurate implementation + validation effort; must not reintroduce A1‑style “full recompute per sample” instability.

### B3 — Acceptance criteria (for closing Phase B)

- **Subjective:** zipper / “bitcrush motion” **gone or clearly reduced** on the **same** beta preset matrix used in B1, without A7/A8/A9‑class regressions.  
- **Objective (optional):** reduced **modulation‑locked** sidebands in wet‑null captures vs baseline branch; document host/buffer cells where improvement holds.

### B4 — Non‑goals (until Phase B stabilizes)

- Further **probe** modes for listening‑only triage.  
- **Factory JSON** retune as the **primary** lever (still allowed **after** architecture stabilizes).  
- Re‑trying **A7 / A8 / A9** without a new design doc referencing this section.

---

## External references (Thiran / fractional delay — what “successful” practice usually does)

Primary literature and teaching sources (URLs are stable canonical copies):

| Source | URL | Relevant takeaway for Choroboros |
|--------|-----|----------------------------------|
| J. O. Smith III — *Physical Audio Signal Processing*, §Thiran allpass interpolators | [ccrma.stanford.edu/~jos/pasp/Thiran_Allpass_Interpolators.html](https://ccrma.stanford.edu/~jos/pasp/Thiran_Allpass_Interpolators.html) | Thiran gives **maximally flat group delay at DC**; mean group delay of an \(N\)th-order stable allpass is **\(N\)** samples. Approximation quality is best **near DC**; **high-frequency phase/delay error** grows — fast chorus modulation sweeps bands where the design is **least** accurate → grain / roughness can be **structural**, not only a bug. |
| Välimäki, Laakso, MacKenzie — *Elimination of transients in time-varying allpass fractional delay filters* (ICMC 1995) | [ICMC 1995 full text (UMich)](https://quod.lib.umich.edu/i/icmc/bbp2372.1995.096/--elimination-of-transients-in-time-varying-allpass-fractional?rgn=main;view=fulltext) | Time-varying **recursive** allpass FD: coeff changes excite transients; published fix is **state-variable update**. **Ship** path uses **smoothed** \(a_k\), **integer-hop** sin²/cos² blend, and **delay slew** — related mitigations, **not** the full V&L recipe. |
| Välimäki — PhD thesis (1995), esp. Ch. 3–4 on FD / waveguides | [Aalto legacy publications index](http://users.spa.aalto.fi/vpv/publications/vesa_phd.html) | Same school: **waveguide** applications often keep delay **slowly varying** or use **dedicated** transient handling. |
| Välimäki — *Simple design of fractional delay allpass filters* (EUSIPCO 2000) | [EUSIPCO00 PDF (Aalto)](http://users.spa.aalto.fi/vpv/publications/EUSIPCO00-FD-AP-Design.pdf) | Discusses **truncated / wideband** Thiran-derived allpass designs — different trade (bandwidth vs order) than HQ 5th-order + audio-rate \(D\) slew. |
| Laakso et al. — *Splitting the unit delay* (IEEE SPM, 1996) | Often cited as **tutorial** on FD filters (FIR vs IIR, tuning vs modulation). | Industry default for **fast-modulated delay** is often **Lagrange / Farrow FIR** or **low-order** allpass; **high-order Thiran + fast \(D\)** is an aggressive combination. |
| JUCE `DelayLineInterpolationTypes::Thiran` | [docs.juce.com — Thiran struct](https://docs.juce.com/master/structjuce_1_1dsp_1_1DelayLineInterpolationTypes_1_1Thiran.html) | JUCE ships **1st-order** Thiran for `DelayLine` and warns about **fast modulation**. Choroboros uses **5th-order** + **hop crossfade** + **smoothed** delay/coeffs — still a demanding path; HIL and probes matter. |

### Contrast table: “textbook” Thiran use vs **shipping** `ChorusCoreThiran`

| Dimension | Typical successful / conservative use | `ChorusCoreThiran` (**current ship**) |
|-----------|----------------------------------------|------------------------------|
| **Modulation rate** | Slow delay tweaks (tuning, pitch shift, waveguide bore length) | **LFO-rate** on smoothed total delay \(T\) |
| **Order** | Low order when \(D\) moves quickly (JUCE: 1st Thiran) | **ORDER = 5** Thiran allpass on **integer** tap |
| **Time-varying \(D\)** | Coefficient update + **state correction** (V&L); or **FIR** fractional | **Smoothed** \(a_k\) toward analytic \(D\); **sin²/cos²** on **⌊T⌋** hop; **not** V&L full recipe |
| **Where accuracy matters** | Thiran design optimizes **DC / low** phase delay | Chorus + Color/Focus emphasize **mid/high** |
| **Output** | Often no extra HF emphasis on the FD path | Blue **Focus** wet **re-boosts** highs |

### Implication (not a prescription)

If residual grain remains, treat it as **recursive FD + LFO** stress: **hop / xfade / coeff τ**, **wet LP**, **Focus**, **host buffer / denormals** — and compare captures to **Phase B** criteria.

