# BBD DSP Reference Documentation

Technical references for the proprietary BBD implementation. All formulas verified from primary sources.

---

## 1. Table III – Third-Order Lagrange BLEP Residual (Välimäki et al. 2012)

**Source:** Välimäki, Pekonen, Nam. "Perceptually informed synthesis of bandlimited classical waveforms using integrated polynomial interpolation." *J. Acoust. Soc. Am.* 131(1), 974–986 (2012).  
**Used by:** DAFx 2025 (Gabrielli et al.) for BBD anti-aliasing.

For third-order (N=3) Lagrange: variable change **d = D − (N−1)/2 = D − 1**, with d ∈ [0, 1).

### BLEP residual polynomials β(d), d ∈ [0, 1)

| Span (samples) | BLEP residual polynomial |
|----------------|---------------------------|
| −2T … −T       | d⁴/24 − d²/12 |
| −T … 0         | −d⁴/8 + d³/6 + d²/2 − 1/24 |
| 0 … T          | d⁴/8 − d³/3 − d²/4 + d − 1/2 |
| T … 2T         | −d⁴/24 + d³/6 − d²/6 + 1/24 |

**Note:** The simple "t² − 2t + 1" formula is from first-order polyBLEP (Table I), not Table III. DAFx 2025 uses third-order Lagrange (Table III) above.

---

## 2. Bilinear Transform – Cutoff Prewarp

**Source:** JOS, CCRMA. *Introduction to Digital Filters*, Example: Second-Order Butterworth Lowpass.

To match digital cutoff frequency ω_c (rad/s) at sample rate f_s:

**c = cot(ω_c T / 2)**  where T = 1/f_s

Equivalently: **c = 1 / tan(π · f_c / f_s)**

**Incorrect:** c = 2/T (generic mapping; does not preserve cutoff).

**Frequency warping:** ω_a = c · tan(ω_d · T / 2) maps digital ω_d to analog ω_a.

---

## 3. Butterworth Pole Positions (JOS, CCRMA)

**Source:** [Butterworth Lowpass Poles and Zeros](https://ccrma.stanford.edu/~jos/filters/Butterworth_Lowpass_Poles_Zeros.html)

For Nth-order normalized (ω_c = 1 rad/s):

- θ_k = (2k + 1)π / (2N), k = 0, 1, …, N−1
- s_k = σ_k + jω_k = −sin(θ_k) + j·cos(θ_k)

Left-half-plane poles only. Scale by ω_a for desired cutoff (after prewarp).

---

## 4. Pole-to-Biquad Conversion (Bilinear Transform)

**Source:** Parks & Burrus, *Digital Filter Design*; JOS filter design examples.

### First-order section (real pole at s = −a)

H_a(s) = 1/(s + a). With s = c(1−z⁻¹)/(1+z⁻¹):

H_d(z) = (1 + z⁻¹) / ((c + a) + (a − c)z⁻¹)

Normalize by (c + a):

- b0 = 1/(c + a), b1 = 1/(c + a)
- a1 = (a − c)/(c + a), a2 = 0

### Second-order section (complex pole pair p = −σ ± jω)

H_a(s) = 1 / ((s + σ)² + ω²) = 1 / (s² + 2σs + (σ² + ω²))

Substitute s = c(1−z⁻¹)/(1+z⁻¹), expand, normalize denominator to 1 + a1·z⁻¹ + a2·z⁻².

**Result (after algebra):**

Let K = c² + 2σc + (σ² + ω²). Then:

- b0 = 1/K, b1 = 2/K, b2 = 1/K  (numerator (1+z⁻¹)² from zeros at s=∞)
- a1 = 2(c² − (σ² + ω²)) / K
- a2 = (c² − 2σc + (σ² + ω²)) / K

**Reference:** JOS Example Second-Order Butterworth works through the algebra for a = e^(jπ/4).

---

## 5. JUCE Butterworth Q Values (verification)

For odd-order Butterworth, JUCE uses:
- First-order section at cutoff
- Biquads with Q = 1 / (2·cos((i+1)π/N)), i = 0, …, (N/2)−1

For 5th order: Q₁ = 1/(2·cos(π/5)), Q₂ = 1/(2·cos(2π/5)).

Equivalent to pole-placement formulation.

---

## 6. Raffel Waveshaping (echo.cpp)

```
bbdOut = bbdOut - (1/166)·bbdOut² - (1/32)·bbdOut³ + 1/16
```

---

## 7. Raffel Input Interpolation (echo.cpp)

When writing at fractional tick:
```
delta = (currTime - clockTime) / T
delayin = delta * bbdIn + (1 - delta) * prevInVal
```
