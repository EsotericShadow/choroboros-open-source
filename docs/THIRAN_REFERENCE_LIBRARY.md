# Thiran fractional delay — reference library

This file records **external sources** supplied for the Choroboros / Blue HQ Thiran workstream. It is a **bibliography + design crib**, not a reproduction of copyrighted manuals or books. Open the linked originals for full text, figures, and proofs.

### Sources vs implementations (read this first)

- **Inside this Markdown file:** several **independent references** (MATLAB docs, JOS, Laakso textbook, McGill notes, Perera et al.). They describe **one** classical object — the **Thiran allpass fractional-delay filter** — from different angles (closed form, optimal \(D\) range, first-order limit, RF wideband use). That is **not** “multiple Thiran cores” in the product.
- **Inside the C++ tree:**  
  - **One shipped core** per `CoreId::thiran`: `ChorusCoreThiran` (Blue HQ) — **integer delay line + dual Thiran allpass** (Laakso/JOS-style), implemented **fresh** in `ChorusCoreThiran.{h,cpp}`.  
  - **One archived file** (not in CMake): `ChorusCoreThiranLegacyInactive` under `legacy_inactive/`, kept for **archaeology / diff only** — not the ship topology.

So: **full picture of the literature** = still growing as you paste more; **full picture of the codebase** = ship Thiran core + legacy snapshot + dev **probes** aligned to the ship core (`thiranReductionProbe`).

---

## 1. MATLAB Control System Toolbox — `thiran(tau, Ts)`

**Source:** MathWorks Help Center, function `thiran` (“Generate fractional delay filter based on Thiran approximation”).

**Role:** Canonical software naming and discrete-time allpass transfer function viewpoint: delay \(\tau\) (seconds), sample time \(T_s\), \(D = \tau / T_s\), order \(N = \lceil D \rceil\) when \(\tau\) is not an integer multiple of \(T_s\); pure delay \(z^{-N}\) when it is.

**Allpass form (documentation summary):**

\[
H(z)=\frac{a_N z^N + a_{N-1} z^{N-1} + \cdots + a_0}{a_0 z^N + a_1 z^{N-1} + \cdots + a_N}
\]

with \(a_0 = 1\) and \(a_k\) from the product/binomial construction (see §2–3 below — same family as audio DSP literature).

**Pointers:** Search MathWorks documentation for `thiran` and “Models with Time Delays”. Typical citation line: *The MathWorks, Inc., Control System Toolbox documentation, function `thiran`.*

---

## 2. J. O. Smith — *Physical Audio Signal Processing* (online book)

**Source:** “Thiran Allpass Interpolators” chapter section (DSPRelated / Stanford-style open book material).

**Role:** Audio-oriented statement of the **maximally flat group delay** Thiran allpass as a **recursive** counterpart to Lagrange FIR for fractional delay; gives the **\(H(z)\)** ratio form and the closed-form \(a_k\) construction in code-like notation.

**Design parameter:** Desired delay \(\Delta = N + \delta\) samples; coefficients \(a_k\) from binomials and products of \((\Delta - N + n)/(\Delta - N + k + n)\) (equivalent to Laakso Eq. (3.133) after notational alignment).

**Pointers:** Book title *Physical Audio Signal Processing*; section “Thiran Allpass Interpolators”. Use the book’s PDF/HTML for the exact \(H(z)\) diagram and group-delay figures. **Local figure bundle:** see **§13** (`docs/assets/`).

---

## 3. Laakso et al. — discrete-time modeling / fractional-delay allpass (textbook excerpt)

**Source:** Book chapter “Fractional Delay Filters”, §3.4 *Design Methods for Fractional Delay Allpass Filters*, §3.4.3 *Maximally Flat Group Delay Design* (Thiran; closed form), §3.4.4 *Choice of Optimal Range for D*.

**Key formulas (notation from excerpt):**

- Allpass: \(A(z)=z^{-N} D(z^{-1})/D(z)\) with real \(a_k\), \(|A(e^{j\omega})|=1\).
- **Thiran allpass (MF at DC),** delay parameter **\(D\)** (total group delay target in samples), order **\(N\)**:

\[
a_k = (-1)^k \binom{N}{k} \prod_{n=0}^{N} \frac{D - N + n}{D - N + k + n}, \quad k = 0,1,\ldots,N
\]

with \(a_0 = 1\) (no extra scaling). This matches the implementation in `ChorusCoreThiran::computeCoefficients` (same product structure as the legacy core).

**Stability (excerpt):** Proof context for **\(D > N\)**; text also notes experimental stability for **\(N-1 < D < N\)** and issues at **\(D = N-1\)** (pole on unit circle) and **\(D < N-1\)** (unstable).

**Design window (rule of thumb, §3.4.4, Eq. (3.137)):** For small average approximation error, operate with

\[
N - \tfrac{1}{2} \le D < N + \tfrac{1}{2}
\]

(Table 3.2 gives finer **\(D_{0,\mathrm{opt}}\)** per order; the half-sample-centered interval is the simple rule.)

**Choroboros implication:** Mapping the **modulation-driven** scalar \(D\) into this interval (rather than always \((N, N+1)\)) aligns the running point with the low MSE band described in the excerpt. Implementation choice belongs in `ChorusCoreThiran` / HIL notes.

**Pointers:** Full chapter PDF from the original publisher; secondary summary in Laakso–Välimäki–Karjalainen–Laine, *Splitting the unit delay*, *IEEE Signal Processing Magazine*, Jan. 1996 (tutorial, cited everywhere).

---

## 4. McGill University — Scavone course notes (allpass interpolation)

**Source:** Course material on “Delay Line Interpolation: Allpass Interpolation” (McGill, maintained by Gary P. Scavone).

**First-order allpass (simplest Thiran case):**

\[
H(z)=\frac{a + z^{-1}}{1 + a z^{-1}}, \qquad
\Delta \approx \frac{1-a}{1+a} \ \text{(low frequency)}, \qquad
a = \frac{1-\Delta}{1+\Delta}.
\]

**Practical \(\Delta\) range (notes):** \(\Delta\) roughly **0.3 … 1.3** samples for flatter phase delay and faster decaying impulse response when **delay is time-varying** (shorter tails than \(\Delta \to 0^+\)).

**Choroboros implication:** Explains why **first-order** allpass is a poor drop-in for **Blue HQ** (we use **order 5** Thiran for the “focus” band), but informs **probe / reduction** paths and any future **cascaded low-order** experiments.

---

## 5. Perera, Rathnasekara, Madanayake — wideband Thiran in digital beamforming

**Source:** S. M. Perera, G. Rathnasekara, A. Madanayake, “Thiran Filters for Wideband DSP-Based Multi-Beam True Time Delay RF Sensing Applications,” *Sensors* **2024**, *24*(2), 576.  
**DOI:** <https://doi.org/10.3390/s24020576> (open access).

**Topic:** True time delay (TTD) **Delay Vandermonde Matrix (DVM)** factorizations; **twiddle** stages realized with **Thiran allpass** fractional delays; FPGA RFSoC prototypes (orders 3 and 4 in the paper’s examples).

**Takeaways relevant to *audio* (by analogy, not 1:1):**

- Thiran trades **usable bandwidth** vs **complexity** vs high-order FIR fractional delay.
- Under their simulations, **approximately constant group delay** holds over a **limited normalized-frequency band** (they stress **temporal oversampling** / keeping the band of interest inside the “good” region; figures use \(|\omega| \lesssim \pi/3\) style ranges in the discussion).
- **Direct Form II** (DFII) realizations appear in the DSP/FPGA context — matches our implementation concern: **coefficient updates vs state** must be handled carefully (smoothed \(a_k\), integer-hop crossfade in `ChorusCoreThiran`).

**Choroboros implication:** Justifies **mild wet LP** and **conservative modulation** stress tests: chorus is wideband baseband, but the Thiran block is still a **narrowband-accurate** FD element at high \(\omega T\).

---

## 6. In-repo implementation pointers

| Item | Location |
|------|-----------|
| **Canonical spec (Laakso band + split delay)** | [`docs/THIRAN_CANONICAL_SPEC.md`](THIRAN_CANONICAL_SPEC.md) |
| Shipping Thiran core | `Source/Cores/blue_engine_modern/ChorusCoreThiran.{h,cpp}` |
| Archived pre-rebuild core | `Source/Cores/blue_engine_modern/legacy_inactive/` |
| HIL / listening notes | `docs/HIL_V205_A_THIRAN_BLUE_HQ.md` |
| Core token / descriptor | `Source/DSP/CoreAssignments.h` (`CoreId::thiran`) |
| Golden coefficient tests | `Tests/ChoroborosRegressionTests.cpp` (`testThiranCanonicalGoldenCoefficients`) |

---

## 7. Suggested BibTeX-style entries (copy as needed)

```bibtex
@article{Thiran1971,
  author  = {Thiran, J.-P.},
  title   = {Recursive Digital Filters with Maximally Flat Group Delay},
  journal = {IEEE Trans. Circuit Theory},
  year    = {1971},
  month   = nov
}

@article{LaaksoEtAl1996Tutorial,
  author  = {Laakso, Timo I. and V{\"a}lim{\"a}ki, Vesa and Karjalainen, Matti and Laine, Unto K.},
  title   = {Splitting the Unit Delay},
  journal = {IEEE Signal Processing Magazine},
  year    = {1996},
  volume  = {13},
  number  = {1},
  pages   = {30--60}
}

@article{Perera2024ThiranSensors,
  author  = {Perera, Sirani M. and Rathnasekara, Gayani and Madanayake, Arjuna},
  title   = {Thiran Filters for Wideband DSP-Based Multi-Beam True Time Delay RF Sensing Applications},
  journal = {Sensors},
  year    = {2024},
  volume  = {24},
  number  = {2},
  pages   = {576},
  doi     = {10.3390/s24020576}
}

@inproceedings{Hacihabiboglu2006RootDisplacement,
  author    = {Hac{\\i}habibo{\\u g}lu, H{\\\"u}seyin and G{\\\"u}nel, Banu and Kondoz, Ahmet M.},
  title     = {Interpolated Allpass Fractional-Delay Filters Using Root Displacement},
  booktitle = {Proc. IEEE Int. Conf. Acoustics, Speech and Signal Processing (ICASSP)},
  year      = {2006},
  pages     = {III--864--III--867},
  note      = {IEEE; verify page string in official metadata}
}

@misc{ValimakiTruncatedThiranFD,
  author = {V{\\\"a}lim{\\\"a}ki, Vesa},
  title  = {Simple Design of Fractional Delay Allpass Filters},
  year   = {undated in paste},
  note   = {Complete venue/page range not in paste; excerpt adjoins thesis pagination ``158''---retrieve official PDF for BibTeX}
}
```

---

## 8. GNU Octave — `thiran(tau, tsam)`

**Source:** GNU Octave Control package documentation (`thiran` function file).

**Role:** Same **family** as MATLAB §1: approximate a **continuous-time** delay \(\tau\) (seconds) at sampling interval `tsam`; returns a **discrete-time transfer function**; **integer** \(\tau/\texttt{tsam}\) reduces to pure **`z^{-N}`** delay; otherwise an **allpass** Thiran approximation (example in docs: \(\tau=1.33\), `tsam=0.5` → third-order rational \(H(z)\)).

**Choroboros implication:** Confirms the **toolbox view** (“delay in seconds ÷ sample period”) as distinct from the **audio FD view** (“total delay in samples \(D\)” directly in `computeCoefficients`). Both map to the same coefficient construction after unit conversion.

**Pointers:** Search Octave docs for `thiran` (Control toolbox / `help thiran` in Octave).

---

## 9. “What is an allpass?” — informal thread + JOS link

**Source:** User-copied **Reddit-style** Q\&A (approx. 2022; exact permalink not preserved in paste).

**Consolidated answers (for the library, not verbatim):**

- **Allpass:** \(|H(e^{j\omega})| = 1\) for all \(\omega\) of interest; **phase** (hence **group delay**) is what changes — not “just a delay” unless \(H(z)=z^{-K}\).
- **vs comb:** Comb-like **feedforward-only** or **feedback-only** structures create **peaks/notches** in magnitude; classic **first-order AP** combines paths so **pole–zero magnitude cancellation** yields flat \(|H|\).
- **“Frequency knob” AP vs delay-style AP:** Same **family** of circuits — parameterization differs (e.g. SVF/biquad allpass path vs explicit delay \(z^{-m}\) with coefficient); not automatically “tiny delay in ms” vs “cutoff”: read the **specific** topology.

**Curated link (from thread):** Julius O. Smith, *Physical Audio Signal Processing*, “Allpass from Two Combs”:  
<https://ccrma.stanford.edu/~jos/pasp/Allpass_Two_Combs.html>

**Optional multimedia (cited in thread):** Dan Worrall videos on allpass / phase (YouTube titles only in paste — search “Dan Worrall allpass” if needed).

**Choroboros implication:** Explains **why** Thiran sits in a **chorus delay path** (phase / fractional delay) while magnitude stays neutral; also why **coefficient–state** issues show up as **color** rather than “EQ bumps.”

---

## 10. DSP Stack Exchange — maximally flat 2nd-order allpass; Matt L. answer

**Source:** Stack Exchange *Signal Processing* thread (Dec 2024 paste; OP: second-order allpass \(G(z)=z^{-2}K(z^{-1})/K(z)\), **maximally flat** phase/group delay at DC, solve for \(a,b\) in \(K(z)=1+az^{-1}+bz^{-2}\)).

**Matt L. answer — key ideas:**

- Brute-force **derivative matching** at \(\omega=0\) is instructive but **laborious**; **Thiran’s closed form** is the scalable solution (arbitrary order).
- Links **all-pole** Thiran construction to **allpass** \(H(z)=z^{-N}A(z^{-1})/A(z)\): DC group delay of allpass = **\(2\tau + N\)** when \(\tau\) is the all-pole MF parameter → substitute **\(\tau \mapsto (D-N)/2\)** style reparameterization (see answer) to get the **standard \(a_k\)** product formula (consistent with Laakso Eq. (3.133) / our code).
- **Recurrence** (I. Selesnick, cited in answer) for efficient coefficient build; small-order closed forms match direct derivation.
- **Max-flat group delay \(\Leftrightarrow\) max-flat phase delay** at DC in the sense given in the answer (derivative relations at \(\omega=0\)).

**Also cited:** R. Bristow-Johnson partial brute-force phase path in same thread (in progress / commentary).

**Pointers:** Search DSP.SE for “maximally flat group delay second order allpass Thiran” (Dec 2024).

---

## 11. comp.dsp — “Is anybody familiar with the Thiran allpass filter?” (Dec 2003)

**Source:** Classic **Usenet / comp.dsp** thread (quoted in paste: Alberto, **Rick Lyons**, Siddharth Mathur, Matt Timmermans, Clay Turner, Jim Adamthwaite, etc.).

**Practical wisdom recorded:**

- **Rick Lyons:** Very **high Thiran order** in **IIR** is numerically fraught (“like a 16th-order IIR”); prefer **bulk integer delay** + **low-order** Thiran for the **fractional** part. **Asymmetry:** for a given target, **negative** fractional offset vs positive can give **flatter** group-delay behavior in the Thiran construction (thread-specific tip — verify in your \(D\) parameterization). **Higher \(F_s\)** keeps more of the audio band in the region where Thiran delay is flat.
- **Siddharth Mathur / Alberto:** For **time-varying** fractional delay (slider moves), **FIR fractional delay** (Lagrange, windowed sinc, etc.) avoids **IIR stability / coefficient-update** pain; points to **fdtools** and *Splitting the unit delay*.
- **Clay Turner:** **Taylor / Farrow-style** decompositions and polyphase splits as another variable-delay toolkit.
- **Outcome (Alberto):** Moved to **127-tap LS windowed sinc** FIR for the application in the thread.

**Choroboros implication:** Historical **validation** of the current product direction: **Lagrange (or similar) for the moving read** + **Thiran for a bounded, controlled \(D\)** is exactly the “don’t ask order-128 Thiran to do all the work” philosophy, echoed independently by Lyons and by Alberto’s eventual FIR choice.

**Pointers:** Search Google Groups or archives for `comp.dsp` Thiran Alberto December 2003.

---

## 12. Beamforming paper snippet (references block)

**Source:** Fragment pasted from an unrelated **IEEE-style** paper intro + bibliography (radar/phased arrays; cites Fulton et al., Rotman \& Tur, **Laakso SP Mag 1996**, **Thiran 1971**, Harris windows, etc.).

**Role here:** Shows **same citation stack** (Thiran + Laakso tutorial) recurring across **audio FD**, **control-toolbox discretization**, and **array/wideband** literature — not a separate theory, different **constraints** (bandwidth, oversampling, coefficient update rate).

---

## 13. J. O. Smith (CCRMA) — “Thiran Allpass Interpolators” (figures archived in-repo)

**Source:** *Music 420* lecture overheads / *Physical Audio Signal Processing* web chapter, **CCRMA, Stanford** (often bundled with `Interpolation.pdf`). Copyright Julius O. Smith III; cite the official site when publishing.

**Why these figures matter:** They make **visual** what the algebra only implies: Thiran gives **excellent** group-delay tracking at **DC** and low \(\omega T\), but **order and operating \(\Delta\)** decide how far toward **Nyquist** that flatness survives — directly relevant to **chorus** (wideband source + moving \(\Delta\)).

### 13a — Formula page (transfer function + \(a_k\))

**File:** `docs/assets/thiran_allpass_interpolators_jos_formulas.png`  
(duplicate/alternate scan: `docs/assets/thiran_allpass_interpolators_jos_ccrma.png`)

**Summary:** \(H(z)=z^{-N}A(z^{-1})/A(z)\); \(\Delta=N+\delta\);  
\(a_k = (-1)^k \binom{N}{k} \prod_{n=0}^{N} \frac{\Delta-N+n}{\Delta-N+k+n}\); \(a_0=1\); mean group delay \(=N\); “only closed-form allpass FD of arbitrary order”; tuning / low-frequency pitch accuracy.

### 13b — Group delay vs frequency (orders 1, 2, 3, 5, 10, 20)

**File:** `docs/assets/thiran_group_delay_orders_1_2_3_5_10_20_del_order_plus_0_3.png`

**What the plot shows (read from JOS caption):** Family of **group delay** curves for Thiran allpass interpolators with **desired delay \(=\) order \(+\,0.3\)** samples (“easy zone”). Horizontal axis: **normalized rad/sample** (Nyquist \(\approx \pi\)); vertical: **group delay (samples)**. All curves match the **fractional part (0.3)** at \(\omega=0\); **low orders** peel away from 0.3 almost immediately with \(\omega\); **orders 10 and 20** stay near 0.3 across a **large** fraction of \([0,\pi]\) before diving (order 20 roughly flat until \(\sim 2\) rad/sample in the description).

**Choroboros implication:** Explains the **product** choice of **order-5** Thiran for Blue HQ (compromise of complexity vs bandwidth of “good” delay), and why **HF hash / zipper** under heavy modulation is **expected** when the ear cares about frequencies where the curve is no longer flat — not necessarily a bug in one line of C++, but **physics of the approximation**.

---

## 14. Hacıhabiboğlu, Günel, Kondoz — root displacement between two Thiran FD allpasses

**Source:** H. Hacıhabiboğlu, B. Günel, A. M. Kondoz, “Interpolated Allpass Fractional-Delay Filters Using Root Displacement,” *Proc. IEEE ICASSP*, 2006, pp. III-864–867 (IEEE Xplore; user paste from licensed PDF).

**Idea:** Given **two** \(N\)th-order Thiran allpass FD filters for delays \(D_1, D_2\), build an **intermediate** allpass for \(D_i\in(D_1,D_2)\) by **interpolating denominator poles** in the complex plane: sort positive half-plane poles by angle, displacement \(\vec{\nu}_k=c_{2,k}-c_{1,k}\), then \(c_{\mathrm{int},k}=(1-\rho)c_{1,k}+\rho c_{2,k}\), \(\rho\in[0,1]\); reconstruct second-order sections; **stability** retained if endpoints stable. **Cost:** \(\mathcal{O}(N)\) multiplies per update if poles precomputed (**~\(4N\)** stated) vs recomputing full Thiran coefficients (\(\mathcal{O}(N^2)\) mults/divs in paper’s count).

**Caveat (authors):** Interpolated filters are **not** maximally flat; **FRE** (phase vs ideal \(e^{-j\omega D_i}\)) trades off vs bracket width; better when \(|D_2-D_1|\) is small.

**Application in paper:** Variable fractional delay in a **digital waveguide string** (glissando); higher harmonics imperfect under Thiran bandwidth limits.

**Choroboros implication:** Alternative to **quantised-\(D\) + crossfade** for smooth delay motion: pole-space interpolation between **two** precomputed Thiran designs (heavier setup, different error profile).

---

## 15. Välimäki — truncated Thiran allpass (wideband FD, closed form)

**Source:** V. Välimäki, “Simple Design of Fractional Delay Allpass Filters,” conference paper (paste includes pp. 158+; cites Laakso SP Mag 1996, Lang–Laakso 1994, Fettweis 1972, Thiran 1971).

**Method:** Start from **Thiran/Fettweis** closed-form \(a_k\) for **prototype order** \(M\), then **implement** a lower **order** \(N<M\) by using only the **first \(N\)** coefficients (truncation / “prototype order” vs “implementation order”). **Not** MF at DC anymore, but often **wider** useful bandwidth than full Thiran of order \(N\); numerically benign even for very large \(M\).

**Design aids:** Empirical formulas for **normalized bandwidth** \(B\) and **peak frequency-response error** \(E_{\max}\) vs \(N,M\) (paper Eqs. (5)–(8) in paste).

**Stability (paper):** Truncated designs reported stable for **\(d>-1\)** in the \(D=N+d\) convention used there (align with Laakso/Fettweis stability discussion when porting).

**Choroboros implication:** Another knob if we ever need **wider HF delay accuracy** without leaving the allpass-only path (different trade than Lagrange + Thiran cascade).

---

## 16. Välimäki — thesis/book ch. 4: fractional-delay **waveguide** filters (FDWF)

**Source:** Excerpt from *Discrete-Time Modeling of Acoustic Tubes Using Fractional Delay Filters* (Ch. 4: “Fractional Delay Waveguide Filters”), §§4.5–4.8 in user paste.

**Engineering content:**

- **Variable delay line** with **first-order allpass** (Figs. 4.30–4.31): shared delays, difference eq. (4.36)/(4.38); **deinterpolator** as **transpose** (Fig. 4.32); interpolator vs deinterpolator **coefficients not simply identical** (complementary delay \(d\) vs \(1-d\) in FIR world; allpass needs separate design per role).
- **FIR vs allpass in FDWFs:** FIR taps relate directly to impulse response / junction geometry; **allpass** gives **unity magnitude** and often **lower complexity** than FIR for comparable low/mid-band error in their examples.
- **Allpass FD junction** (Fig. 4.33c): combine delays so each side uses **\(D=2d\)** and \(\bar D=2\bar d\) with \(d+\bar d=1\); first-order Thiran blocks approximate **double** fractional delays in \([0,2]\).
- **Thiran vs FIR at junction:** MF Thiran error **zero** when junction at **\(d=0.5\)** (unlike FIR where \(d=0.5\) was worst-case in their comparison narrative).
- **Broken structure** (Fig. 4.36b): sum of two allpasses is **not** allpass → can break passivity / stability in feedback loops — pedagogical “don’t commute naïvely” warning.

**Delay-range wording (check against §3):** The excerpt states an “easy rule of thumb” to keep the Thiran delay parameter in **[\(N-\tfrac12\),\,N+\tfrac32\)]** for the **junction** discussion (first-order case tied to \(d\in[0.25,0.75]\) for Fig. 4.33c). Elsewhere (§3 / our §3) the **MSE-optimal** operating band is often quoted as **\([N-\tfrac12,\,N+\tfrac12)\)**. Treat these as **different design contexts** (waveguide double-delay vs generic FD tab); reconcile with the primary text before changing `ChorusCoreThiran` limits.

**Choroboros implication:** Historical precedent for **first-order Thiran in waveguides**; our **Blue HQ order-5** path is a different cost/quality point on the same FD–allpass ladder.

---

*Maintainers: append new pasted sources as new numbered sections; keep long quotes out of the repo and link to originals instead.*
