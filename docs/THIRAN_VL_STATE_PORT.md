# Välimäki–Laakso–MacKenzie (ICMC 1995) state port — Thiran DFII (`ChorusCoreThiran`)

This document maps the published **state-variable transient elimination** to the **exact** `processAllpass` / `processAllpassForTests` recursion in [`ChorusCoreThiran.cpp`](../Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp). **Phase 2** regression proves the companion form; **Phase 3** (below) wires the cold-start alignment into the audio thread for **probe 3** only.

## Primary source

Välimäki, V., Laakso, T. I., & MacKenzie, J., *Elimination of Transients in Time-Varying Allpass Fractional Delay Filters with Application to Digital Waveguide Modeling*, **Proc. ICMC 1995** (Banff), pp. 327–334. Full text (UMich ICMC archive):  
https://quod.lib.umich.edu/i/icmc/bbp2372.1995.096/--elimination-of-transients-in-time-varying-allpass-fractional?rgn=main;view=fulltext  

Cross-check summary in [`THIRAN_REFERENCE_LIBRARY.md`](THIRAN_REFERENCE_LIBRARY.md) if present.

## Paper structure used here (section / equation numbers)

The paper puts the **direct-form II** allpass in **state–space** form (their **Eq. (5a)–(5c)**):

- \(v(n+1) = F\,v(n) + q\,x(n)\), \(y(n) = g^\top v(n) + g_0\,x(n)\) (Eq. (5a) family).
- Coefficient jump at sample \(C\): past evolution with \(F_1\), then \(F_2\) after the change.
- **Eq. (6)–(8)** expand \(v(n)\) across the jump and isolate the **transient term** proportional to the **mismatch** between the state you carry forward and the state a **pure-\(F_2\)** filter would have after the same input prefix.
- **Eq. (9)** (cold-start alignment): under the paper’s standing assumption that an **initial-condition transient has decayed**, the **post-change** state at \(C\) can be set to the state a filter with **only** \(F_2\) would have accumulated from **\(n=0\)** with the **same** input samples \(x(0),\ldots,x(C-1)\) and **zero** initial state — i.e. the **controllability** sum / recursion

\[
v(C) \;=\; \sum_{k=0}^{C-1} F_2^{\,C-1-k}\, q\, x(k)
\]

equivalently the **iterative** form used in regression (`v \leftarrow 0`; then for \(k=0..C-1\): \(v \leftarrow F_2 v + q\,x(k)\)).

That is the **operational** content wired into **Phase 2** tests: we do **not** re-derive the allpass topology from the paper; we **match their state-update goal** to **our** \(F,q\) for **our** DFII node.

## Mapping: paper \(F,q\) ↔ Choroboros `ap.state`

**Shipped state vector** \(v = [s_0,\ldots,s_{N-1}]^\top\) is `std::array<float, N>` with **\(N=5\)**, updated in `thiranProcessAllpassOneStep` / `processAllpass`.

**Output** (same sample, before state overwrite):

\[
y(n) = a_N\,x(n) + s_0(n).
\]

**State update** (indexing \(a_0\equiv 1\), \(a_1..a_N\) are `a[1]`..`a[N]` in code):

\[
\begin{aligned}
s_i(n+1) &= (a_{N-i} - a_{i+1} a_N)\,x(n) - a_{i+1}\,s_0(n) + s_{i+1}(n), \quad i=0..N-2,\\
s_{N-1}(n+1) &= (a_0 - a_N^2)\,x(n) - a_N\,s_0(n).
\end{aligned}
\]

**Companion form** used in tests (double precision):

- \((F v)_0 = -a_1 v_0 + v_1\), …, \((F v)_{N-2} = -a_{N-1} v_0 + v_{N-1}\), \((F v)_{N-1} = -a_N v_0\).
- \(q_0 = a_{N-1} - a_1 a_N\), \(q_1 = a_{N-2} - a_2 a_N\), … through the pattern ending in \(q_{N-1} = a_0 - a_N^2\).

**Important:** The paper’s printed **\(F\)** layout in Eq. (5c) follows **their** DFII ordering of \(v_i\). If a verbatim row/column order differs from the above, the **numeric** identity checked in **`testThiranAllpassCompanionFormMatchesProcessStep`** is authoritative for this repo: **one step** of `processAllpassForTests` \(\equiv\) \(v' = F v + q x\) for the **same** \(a_k\).

## When V&L applies vs other ship mechanisms

| Event | Primary lever today | V&L (future) |
|--------|---------------------|--------------|
| **`a_k` smoothing** (~14 ms) toward new \(D\) | Continuous coefficient motion | Optional; reduces need if \(\Delta a\) is small |
| **Integer `⌊T⌋` hop** | Dual AP + sin²/cos² + state **copy** | Different geometry (whole branch), not the same as Eq. (9) on a single AP |
| **Instant coefficient jump** (e.g. probe / lab) | Discontinuity in \(a_k\) **and** inherited `state` | **Eq. (9)** targets **IIR state vs coefficient** mismatch for **one** recursion |

## Practical notes (from the paper’s §3.2)

- **Exact** Eq. (9) needs the **full** prefix \(x(0..C-1)\); long histories are handled in the paper via an **efficient parallel tail** (read §3.2 in the PDF). Phase 2 uses **moderate \(C\)** and bounded inputs so the recursion is exact to **double** tolerance.
- V&L addresses **state–coefficient** transients; it does **not** replace correct **Thiran \(a_k\)** formulas (still Laakso §3.4.3 / §3.4.4).

## Phase 2 code pointers

- `ChorusCoreThiran::processAllpassForTests` — public, bit-identical to `processAllpass`.
- `Tests/ChoroborosRegressionTests.cpp` — `testThiranAllpassCompanionFormMatchesProcessStep`, `testThiranVLStateMatchesColdStartF2Trajectory`, `testThiranNaiveJumpOutputDiffersFromVLAtStep`.

## Phase 3 — audio thread (shipped)

When **`thiran_reduction_probe == 3`** (fractional coeff **snap**), after `computeCoefficients` on the fade-in / tracking branch, `ChorusCoreThiran::alignThiranAllpassStateValimakiLaakso` overwrites `trackAp.state` using Eq. (9) on the **last up to 512** samples of **allpass input** (`activeDelayed` into `processAllpass` for `curActive`), accumulated in **double**. The per-channel ring is reset in **`beginIntegerHop`** so we do not mix tap histories across integer delay changes.

**Not** applied on the default **one-pole `smoothCoeffsTowardD`** path (continuous small steps); **not** applied on integer hop beyond clearing the ring (dual-path crossfade + state copy remain the primary tool there).
