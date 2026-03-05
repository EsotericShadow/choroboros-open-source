# Dev Panel – Complete Contents

Full inventory of all controls, sections, and tabs in the Dev Panel.

**How to open:** Click the **DEV** button in the top-left corner of the plugin. Hover for tooltip.

---

## Top Bar (Buttons)

| Button | Purpose |
|--------|---------|
| **Copy JSON** | Copies all current Dev panel values as JSON |
| **Set Current as Defaults** | Saves tuning, internals, and layout as startup defaults |
| **Lock** | Locks/unlocks all controls for safe editing |
| **FX Off** | Applies off preset to value glow/reflection FX |
| **FX Subtle** | Applies subtle value FX preset |
| **FX Medium** | Applies medium-strength value FX preset |

---

## Left Column (Always Visible)

### Parameter Mapping
Map UI values into DSP ranges (min/max/curve).

- **Rate** – Min, Max, Curve, Lock
- **Depth** – Min, Max, Curve, Lock
- **Offset** – Min, Max, Curve, Lock
- **Width** – Min, Max, Curve, Lock
- **Color** – Min, Max, Curve, Lock
- **Mix** – Min, Max, Curve, Lock

### UI Response
Change slider feel without touching DSP.

- **Rate** – UI Skew, Lock
- **Depth** – UI Skew, Lock
- **Offset** – UI Skew, Lock
- **Width** – UI Skew, Lock
- **Color** – UI Skew, Lock
- **Mix** – UI Skew, Lock

---

## Right Column (Tabbed)

### Engine Tab Controls (when Engine tab selected)
- **Filter** – Textbox to filter engine internals sections by name
- **Clear** – Clears filter
- **Show Advanced** – Toggle for core vs advanced internals

### Tab 0: Overview
**Audio Overview** – Live derived state for the active engine/mode.

- **Active Parameters**
  - Engine + Mode
  - Rate (raw -> mapped)
  - Depth (raw -> mapped)
  - Offset (raw -> mapped)
  - Width (raw -> mapped)
  - Color (raw -> mapped)
  - Mix (raw -> mapped)
- **Derived State**
  - HPF Effective
  - LPF Effective
  - Centre Delay (base + scale*depth)
  - Red NQ BBD Min Delay @ Max Clock
  - Red Saturation Drive Target
- **Signal Flow** – Signal chain diagram with stage badges
- **Visual Feedback** – Delay Trajectory (ms) sparkline

### Tab 1: Mod (Modulation)
**Modulation** – LFO phase and delay trajectory instrumentation.

- **LFO Readouts**
  - LFO Rate
  - Stereo Offset
  - Depth
  - Stereo Correlation (est.)
- **Motion Visuals**
  - LFO Left sparkline
  - LFO Right sparkline
  - Delay Trajectory sparkline

### Tab 2: Tone
**Tone / Dynamics** – Spectrum overlays and nonlinear transfer behavior.

- **Tone Readouts**
  - HPF / LPF
  - Pre-Emphasis Gain
  - Compressor
  - Tonal Tilt (est.)
- **Tone Visuals**
  - Spectrum + HP/LP Overlay
  - Saturation Transfer Curve

### Tab 3: Engine
**Engine** – Engine-relevant internals with core/advanced disclosure and filtering.

- **Active Engine**
  - Engine Character
  - Color Semantics
  - Applicability
- **Engine Macro Derived**
  - Macro A
  - Macro B
  - Macro C

**DSP Internals (Per Engine + HQ)** – Per-engine profiles (Green/Blue/Red/Purple/Black, NQ/HQ).

Each engine profile has:
- **Timing + Motion** – Rate Smooth, Depth Smooth, Depth Rate Limit, Centre Smooth, Centre Base, Centre Scale, Color Smooth, Width Smooth
- **Filtering** (or **Filtering + Emphasis** for Red) – HPF Cutoff, HPF Q, LPF Cutoff, LPF Q, PreEmph Freq/Q/Gain/Level Smooth/Quiet Thresh/Max Amount (Red only)
- **Compressor** – Attack, Release, Threshold, Ratio
- **Saturation** (Red only) – Saturation Drive Scale
- **Green Bloom** (Green only) – Exponent, Depth Scale, Centre Offset, Cutoff Max/Min, Wet Blend, Gain
- **Blue Focus** (Blue only) – Exponent, HP Min/Max, LP Max/Min, Presence Freq Min/Max, Presence Q Min/Max, Presence Gain Max, Wet Blend, Output Gain
- **Purple Warp** (Purple NQ only) – Warp A/B, K Base/Scale, Delay Smooth
- **Purple Orbit** (Purple HQ only) – Eccentricity, Theta Base/Scale, Theta2 Ratio, Eccentricity2 Ratio, Mix1, Stereo Theta Offset, Delay Smooth
- **Black Intensity** (Black NQ only) – Depth Base/Scale, Delay Glide
- **Black Ensemble** (Black HQ only) – Tap2 Mix Base/Scale, Tap2 Depth Base/Scale, Tap2 Delay Offset Base/Scale

**BBD Profiles (Red NQ only)** – Shown when Red Normal is active.

- **Red NQ - Delay + Depth** – BBD Delay Smooth/Min/Max, Centre Base/Scale, Depth
- **Red NQ - Filter** – Filter Smooth, Min/Max Hz, Scale
- **Red NQ - Clock** – Clock Smooth, Min Hz, Max Ratio
- **Red NQ - Structure** – BBD Stages, Filter Max Ratio

**Tape Profiles (Red HQ only)** – Shown when Red HQ is active.

- **Red HQ - Delay + Tone** – Tape Delay Smooth, Centre Base/Scale, Tone Max/Min, Tone Smooth
- **Red HQ - Motion + Modulation** – LFO Ratio/Smooth, Ratio Smooth, Phase Damp, Wow/Flutter Freq Base/Spread, Wow/Flutter Depth Base/Spread
- **Red HQ - Drive + Output** – Drive Scale, Ratio Min/Max, Wet Gain, Hermite Tension

### Tab 4: Layout
**Layout** – Live placement + sizing of UI elements (grouped by engine color).

- **Global (Mix Knob Y, Color Value X)**
- **Green Engine** – Main Knob Size, Knob Top Y, Rate/Depth/Offset/Width/Color/Mix Center X, etc.
- **Blue Engine**
- **Red Engine**
- **Purple Engine**
- **Black Engine**
- **Global Value Typography**
- **Global Value FX - Main Knobs**
- **Global Value FX - Color**
- **Global Value FX - Mix**
- **Value Flip - Main Knobs**
- **Value Flip - Color**
- **Value Flip - Mix**
- **Top Buttons (About/Help/Feedback)**
- **Engine Selector (Combo + Popup)**
- **HQ Flip Switch**
- **Global Knob Response**

### Tab 5: Validate
**Validation** – Quick wiring checks: UI raw -> mapped -> snapshot -> DSP effective.

- **Wiring Checks**
  - Color Zero -> Saturation Gate
  - Profile -> Active Runtime Sync
  - BBD Delay Feasibility (Red NQ)
  - Engine Applicability Guard
- **UI -> Mapped -> Runtime** – Rate, Depth, Offset, Width, Color, Mix traces
- **UI -> Mapped -> Snapshot -> Effective** – Trace Matrix
- **Recent Activity** – Recent touches with timestamps
