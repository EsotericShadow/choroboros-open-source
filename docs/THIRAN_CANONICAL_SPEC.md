# Blue HQ / `thiran` core — canonical specification (ship vs R&D)

This document ties [`ChorusCoreThiran`](../Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp) to the reference library and **separates what ships in the audio thread** from what remains **regression / research**.

## 0. Ship audio path (authoritative)

**Classical Thiran chorus (reference model):**

1. **Smoothed total delay** `T` (samples): centre (block-constant inner smoother) + LFO depth; **one-pole** slew on `T` (~**24 ms** τ) for stability.
2. **Integer floor** `⌊T⌋` drives a **circular delay line** read — **one tap per branch**, offset `max(0, ⌊T⌋ − N)` samples **before** the current write index (**no Lagrange** on the line).
3. **Thiran design delay** \(D\) from Laakso §3.4.4: **`D = (N − ½) + f`** with `f` the fractional part of `T` clamped to \((0,1)\), then **clamped** to the §3.4.4 **interior** used by `computeCoefficients` (same band as golden tests).
4. **DFII-style** `N`th-order allpass (`processAllpass`) on that tap; **`a_k`** move by **one-pole smoothing** (~**14 ms**) toward the analytic target (probe **3** forces full recompute).
5. On **`⌊T⌋` hop**: **dual** allpass instances, **state copy** to the incoming branch, **sin²/cos²** blend over ~**6 ms** (probe **4** = instant hop); **deferred hop** if a floor change arrives mid-xfade.

Implementation is **fresh** in [`ChorusCoreThiran.cpp`](../Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp) — not a copy of `legacy_inactive/` (that path is kept for archaeology only).

## 1. Canonical literature spine (what is “normative”)

| Topic | Normative reference in the dump |
|--------|-----------------------------------|
| Closed-form maximally flat allpass FD \(a_k\) | **Laakso §3.4.3** (Eq. 3.133 family) = **JOS** “Thiran Allpass Interpolators” product = **Thiran 1971** |
| **Operating band for \(D\)** (low MSE / stability context) | **Laakso §3.4.4**: use **\(N-\tfrac12 \le D \le N+\tfrac12\)** with practical **interior margin** (see §3) |
| Bulk motion vs fractional object | **Laakso et al. 1996 tutorial** (“Splitting the unit delay”) + **McGill / comp.dsp** themes summarized in the library §§4, 11 |
| Perceptual bandwidth of flat group delay | **JOS** figures — [§13](THIRAN_REFERENCE_LIBRARY.md) (`docs/assets/`) |
| DFII + time-varying coefficients | **Perera 2024** (engineering) + in-repo **HIL** evidence |
| **Not** blocking v1 | **Välimäki FDWF Ch.4** junction bands ([library §16](THIRAN_REFERENCE_LIBRARY.md)) — different geometry |

## 2. Transfer function and realization (ship)

**Allpass:** \(H(z) = z^{-N} A(z^{-1}) / A(z)\) with \(A(z) = 1 + a_1 z^{-1} + \cdots + a_N z^{-N}\), \(a_0 \equiv 1\).

**Realization:** `ChorusCoreThiran::processAllpass` — DFII transposed–style recursion (same structure as the literature-facing notes in [THIRAN_REFERENCE_LIBRARY.md](THIRAN_REFERENCE_LIBRARY.md)).

**Code:** `computeCoefficients` implements Laakso Eq. 3.133 for \(N=5\); **`computeCoefficientsForTests`** is the regression hook.

**Coefficient construction (Laakso Eq. 3.133):** for integer order \(N\) and **design delay** \(D\) (samples, DC group-delay target of the Thiran allpass block),

\[
a_k = (-1)^k \binom{N}{k} \prod_{n=0}^{N} \frac{D - N + n}{D - N + k + n}, \quad k = 1,\ldots,N,\quad a_0 = 1.
\]

Implemented in `ChorusCoreThiran::computeCoefficients` using `double` for the product; output `float` \(a_k\).

## 3. Laakso §3.4.4 operating band (**for `computeCoefficients` / tests**)

**Theory:** small average FD error when \(D\) lies in the **centred** band of width one sample around \(N\):

\[
N - \tfrac12 \;\le\; D \;\le\; N + \tfrac12
\]

(with endpoint singularities avoided in practice).

**Shipped interior:** clamp \(D\) to

\[
D \in [\,N - \tfrac12 + \varepsilon,\; N + \tfrac12 - \varepsilon\,], \quad \varepsilon = 0.01\text{ samples}.
\]

**Rationale:** keeps \(D\) away from **\(D = N \pm \tfrac12\)** pole-on-boundary degeneracies for any future or offline use of `computeCoefficients`.

## 4. Archived / other code

- **`legacy_inactive/ChorusCoreThiranLegacyInactive`**: older experiment; **not** the ship implementation.
- **Lagrange + quantised-`D_q` split** (prior 2026 attempt): superseded by §0 integer-line + Thiran.

## 5. R&D (not shipped)

- **ICASSP 2006 root displacement** ([library §14](THIRAN_REFERENCE_LIBRARY.md)).
- **Välimäki truncated Thiran** ([library §15](THIRAN_REFERENCE_LIBRARY.md)).

## 6. Regression tests

See [`Tests/ChoroborosRegressionTests.cpp`](../Tests/ChoroborosRegressionTests.cpp) — **`testThiranCanonicalGoldenCoefficients`**: **golden vectors** for \(N=5\) at several \(D\) values in the §3 band (literals from an independent numeric reference). `ChorusCoreThiran::computeCoefficientsForTests` must match within **float tolerance**.

Additional Thiran harness tests (Phase 2): companion-form identity and **Välimäki–Laakso–MacKenzie (ICMC 1995)** cold-start state (Eq. (9) family) vs `processAllpassForTests`; see §8–9.

## 7. Dev panel / telemetry

**`D` trace (ms):** design delay `thiranDForCoeffs` in ms. **Crossfade (0–1):** integer-hop blend progress during **sin²/cos²** window.

## 8. Phase 0 — `processAllpass` state semantics (frozen)

**State vector:** `AllpassInstance::state` holds \(N=5\) samples \([s_0,\ldots,s_{N-1}]\) updated **in place** once per call. **Output** is computed **before** overwriting \(s_0\) (see `thiranProcessAllpassOneStep` in [`ChorusCoreThiran.cpp`](../Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp)).

**Per-sample contract:**

1. \(y[n] = a_N\,x[n] + s_0[n]\) (using clamped Thiran coefficients for the current block).
2. \(s_i[n+1]\) is formed from \(x[n]\), \(y[n]\), and **pre-update** \(s_j[n]\) as in the source (DFII-style chain).
3. Coefficients `a[k]` may differ from sample to sample (smoothing toward \(D\), probe snap, etc.); **no** automatic state rewrite is applied today — that is the gap **V&L Eq. (9)** addresses for **abrupt** \(F_1 \to F_2\) changes.

**Public test hook:** `ChorusCoreThiran::processAllpassForTests` mirrors `processAllpass` for headless regression without touching delay-line topology.

## 9. V&L transient elimination (Phase 1–3)

Normative mapping of ICMC 1995 §3 / Eq. (5)–(9) to this realization, plus isolated tests, lives in [`THIRAN_VL_STATE_PORT.md`](THIRAN_VL_STATE_PORT.md).

**Shipped (Phase 3):** dev **probe 3** (`thiran_reduction_probe == 3`) performs instant `computeCoefficients` on the tracking branch, then **`alignThiranAllpassStateValimakiLaakso`** so DFII state matches a **cold start** of the **new** \(a_k\) on the **same** allpass input prefix (ring length **512**, double accumulation). The ring is **cleared** on each **integer hop** (`beginIntegerHop`) because the allpass input stream switches taps. Smooth fractional tracking (**default path**) is unchanged.

---

*Maintainers: if literature reconciliation changes §3 margins, update golden literals and HIL in the same PR.*
