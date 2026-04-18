# Changelog

All notable changes to Choroboros are documented here.

## [2.05] - 2026-04-11

Sample-rate invariance structural fix — all 5 hardcoded DSP constants now derived from physical time constants.

### Fixed
- **Color macro (Green / Blue / Red):** Red HQ tape tone LP now opens (brighter) as Color increases, with drive still rising with Color; factory `internalsRedHQ` uses a wider audible tone sweep and stronger `tapeDriveScale`. Green and Blue bloom/focus internals and `engineParamProfiles` default Color values were strengthened so the slider is clearly audible at typical mix/depth. Red NQ post-chorus saturation drive scale slightly increased in `internalsRed`.
- **Sample-rate invariance (all engines):** Five DSP constants were hardcoded as raw pole values or sample counts tuned at 48 kHz. At other sample rates (44.1k, 96k, 192k) they silently degraded — smoothing speeds halved/doubled, crossfade durations shrank, and the Tape varispeed spring tightened. All five are now computed from physical time constants (seconds/ms) in prepare() or at the consumption site, making behavior identical at any sample rate.
  - **Thiran delay smoothing** (Blue HQ): 14ms time constant, was hardcoded α=0.9985 (halved at 96k).
  - **Thiran crossfade length** (Blue HQ): 6ms sin²/cos² crossfade, was hardcoded 48 samples (shrank to 0.25ms at 192k, leaking 1.5% THD transients).
  - **Pre-emphasis level follower** (all engines): 208ms time constant, was hardcoded per-block α=0.95 (4× slower at 96k).
  - **Tape tone cutoff smoother** (Red HQ): 0.25ms time constant, was hardcoded α=0.08 (2× faster at 96k).
  - **Tape phase damping** (Red HQ): Per-second retention 0.6188, was hardcoded per-sample 0.99999 (spring 4× stronger at 192k). Field renamed `tapePhaseDampingPerSec` for clean semantic break.
- **Thiran broadband fuzz/distortion** (Blue HQ): Removed per-sample coefficient updates that caused DFII-T state–coefficient mismatch transients (Välimäki & Laakso). Extended crossfade from 1ms to 6ms, reducing THD from 1.47% to 0.14%. State-copy architecture on boundary crossings.
- **Width text input parsing:** Typing "1" set width to 100% and "2" to 200%. parseWidthValue() now always divides by 100 to match the display format. Both PluginEditor.cpp copies fixed.

### Changed
- **Factory defaults JSON parity (maintainers):** `Assets/defaults_factory_mac.json`, `json_defaults_dump.json`, `linux/linux_factory_defaults.json`, and `windows/windows_factory_defaults.json` must stay byte-identical; `scripts/verify_factory_json_sync.sh` and CMake target `verify_factory_json_sync` (Unix) enforce it before release. Documented in `docs/FACTORY_DEFAULTS_JSON_SYNC.md`; `windows/RELEASE_CHECKLIST_HARD.md` is tracked again. Linux factory JSON is included in the repo for the same payload as macOS/Windows embedded defaults.
- **DevPanel slider labels:** Pre-emphasis level smoother shows "PreEmph Level τ (s)" with range 0.001–2.0s. Tape tone smoother shows "Tape Tone τ (ms)" with range 0.01–10.0ms. Tape phase damping shows "Tape Phase Damp/s" with range 0.01–1.0.
- **Factory defaults updated:** All three JSON factory files and PluginProcessor per-engine defaults converted from raw coefficients to physical time constants.
- **Green engine UI (factory):** Layout defaults `mainKnobSizeGreen` 134, `knobTopYGreen` 85; refreshed green main-knob filmstrip PNGs (rate, depth, offset, width off/on pairs).
- **GitHub Actions (Windows AAX):** Windows x64 workflows build `Choroboros_AAX`, include the `.aaxplugin` in the zip when present, and optionally PACE-sign via `windows/sign_aax_pace_windows.ps1` when repository variable `WINDOWS_AAX_SIGNING_ENABLED` is `true` (see `docs/GITHUB_ACTIONS_WINDOWS_AAX.md`).
- **Shared interpolation utility:** New `InterpolationUtils.h` with `readCubicInterp()` deduplicating cubic interpolation across Blue, Purple, and Black cores.
- **Tape core robustness:** Per-channel smoothedDepth with NaN guard in Red HQ. Per-channel colour smoothing in Black HQ.

### Earlier v2.05 release work (2026-04-10)

35 commits, 158 files changed. Major architectural modernization focused on stability, audio thread safety, and extensibility.

### Added
- **KZN custom engine import system:** Full .kzn binary format library (libkzn) with Ed25519 signature verification. ChoroborosKznImporter supports drag-and-drop import. CustomEngineManager provides factory + custom engine model with free-tier gating (single custom slot, white visual identity, commercial cores rejected in free build). PluginEditor wired for drag-and-drop; PluginProcessor handles custom engine activation.
- **PresetState with engine identity:** Added `engineColorIndex` (0–4 factory) and `customEngineId` (UUID string) fields to PresetState. JSON serialize/deserialize, legacy APVTS XML migration, and missing-engine fallback (invalid customEngineId clears to factory).
- **Canonical preset state layer (Phase 1):** PresetState routes host state, user presets, and factory presets through a canonical layer. ApplyContext descriptor for preset application context. DspConfig publish/consume double-buffered DSP configuration at block boundaries. DspConfigManager for DSP parameter routing. PresetManager integration. Tier 1+2 regression verified.
- **Lock-free runtime tuning (Phase 2):** TuningConfigManager replaces dspLock on the audio path with double-buffered publish/consume. Precomputed snapshot-based application eliminates blocking on the audio thread.
- **Headless console service (Phase 3):** ConsoleEngine for headless-safe command parsing (no JUCE UI thread assumptions). Tiered regression harness (Tier 1, Tier 1+2 smoke tests).
- **Consent service (Phase 5):** ConsentService with explicit consent matrix for feedback/analytics. FeedbackDialog integration. prepareForShutdown() pattern matching SessionLog.
- **DPI / HiDPI scaling support:** `setScaleFactor()` override stores host-reported DPI scale factor. JUCE 8.0.12's VST3 wrapper applies the scaling transform automatically. `getUiScale()` returns `kBaseUiScale * dpiScale` so all layout helpers account for both the 0.91x base scale and the host's DPI factor. HQ lit overlay cache invalidated on scale change.
- **Modulation tab redesign:** Replaced L/R sparklines with dual waveform oscilloscope overlay and Lissajous XY stereo phase plot. New scope subtitles, legends, and hints. Updated readout names: "Measured Swing", "Effective Phase Spread", "Stereo Coherence". DevPanel widened from 900 to 1100px.
- **Accessibility: tooltip toggle:** Enable/disable tooltips in DevPanel Accessibility panel. setTooltipsEnabled() exposed in PluginEditor for main UI control.
- **Validation matrix:** VST3 strictness L3/L5/L7/L10, L5 x 3 random seeds. AU L5 + L10, auval aufx ChBr KzDp. Edge block 1/8192 and 96k/192k sample rates. All runs exit 0.
- **Optional sanitizers and profiling:** CMake CHOROBOROS_ENABLE_ASAN/TSAN with per-target flags. Optional Melatonin Inspector (Cmd+I) and Perfetto tracing. TRACE_DSP hooks in processBlock (guarded, default off).
- **macOS release orchestration:** New release_macos_signed_installer.sh — single entry point for universal build, bundle sign, pkg build, notarize. Version sync check (CMake project() vs installer_config vs distribution.xml). Flags: --no-universal-build, --to-signed-pkg, --notarize-only.
- **External SSD (T7) build support:** CHOROBOROS_CMAKE_BUILD_DIR for external disk cmake -B paths. build_on_external_ssd wrapper. clean_choroboros_build_artifacts.sh with dry run vs --yes. Keeps juceaide/objects off internal SSD.
- **CI: macOS Universal job:** Build workflow now includes macOS Universal (VST3, AU, Standalone) matching Release coverage. Runs on v2.05-dev branch.

### Changed
- **Equal-power crossfade and output trim:** Switched DryWetMixer from linear to balanced sin/cos mixing (eliminates 3 dB volume dip at 50% wet). Added smoothed output trim parameter (+/-12 dB) for per-engine gain compensation.
- **Engine core interpolation and smoothing:** Green (Lagrange 3rd/5th) improved smoothing coefficients. Blue (Cubic and Thiran) coefficient interpolation fixes. Red (Tape) crossfade and drive refinements. Black (Linear Ensemble) delay smoothing improvements. Eliminates zippering artifacts across all 5 engines.
- **`kUiScale` renamed to `kBaseUiScale`:** Clarifies base scale factor (0.91f) vs runtime DPI scale. All references updated across both editor files and .cursorrules.
- **UI string polish:** Replaced "deg" abbreviations with degree symbol. Replaced non-ASCII text (em dashes, curly quotes, arrows, math symbols) with ASCII equivalents for JUCE font rendering.
- **Dev Panel tutorials rewritten:** Orientation-first walkthrough replacing concept-first approach. 13-topic index (orientation, core, primers, deep-dive tasks).
- **Dialog destruction safety:** delete-this replaced with SafePointer + MessageManager::callAsync in About/Feedback/Help dialogs. Non-blocking theme decode via polling in paint().
- **Debug I/O removal (Phase 4):** Removed debug file logging and trace disk writes. Clean shutdown paths.
- **CMakeLists JUCE resolution:** Uses `./JUCE`, optional sibling `../JUCE`, or FetchContent JUCE 8.0.12 only (removed an extra optional sibling-repo JUCE path so the open-source tree stays self-contained).
- **Installer improvements:** installer_config.sh sourced by all sign/build/notarize scripts. Fixed wrong bundle paths that broke signing, pkgbuild, and notarization. Stage payloads with ditto, strip xattrs, chmod postinstall before pkgbuild. Detect invalid notarization and fetch log.
- **Build artifact retention:** 30 days down to 3 days (build) / 1 day (release).
- **Binary size 400 MB to <120 MB:** All engine spritesheets reprocessed and optimized (not just Black).
- **Version bump:** Project version 2.0.41 to 2.0.50, version string "Beta v2.04.1" to "Beta v2.05".

### Fixed
- **Three freeze-on-close vectors (beyond D3D11):** (1) DevPanelWindow: hide + removeFromDesktop before reset to prevent Win32 HWND message cascade under DLL loader lock. (2) themePrewarmThread: replaced callAsync with mutex queue + join pattern to prevent detached threads outliving DLL unload. (3) FeedbackCollector: prepareForShutdown() moves blocking disk I/O out of destructor chain.
- **Equal-power crossfade 3 dB dip:** 50% wet mix produced a volume dip because linear mixing doesn't account for phase offset from stereo modulation. Fixed with balanced sin/cos amplitude curves (industry standard).
- **Engine core zippering across all 5 engines:** Per-sample interpolation smoothing and coefficient fixes across Green, Blue, Red, and Black cores. Reported as "DSP anomalies" and "scratchy" by multiple testers.
- **Preset browser cycling bug:** Preset index reset to -1 by parameter callbacks during preset load. loadInProgress_ guard prevents false invalidations in invalidatePreset().
- **Text entry '1' = 100% bug:** Boundary check `value > 1.0f` didn't catch exactly 1.0. Fixed to `value >= 1.0f && value <= 100.0f` in Depth, Color, Mix, and Width parsers. Width parser also clamps via juce::jlimit. Offset parser improved to accept both "deg" and degree symbol suffixes.
- **Color slider "sounds reversed" after engine switch:** Re-sync slider value after skew factor change + applyTuningToUI() on engine switch.
- **Timer race condition on shutdown:** stopTimer() doesn't block on a currently-executing timerCallback(). Added std::atomic<bool> isShuttingDown with release/acquire ordering. Flag set before stopTimer() in destructor and releaseResources(); cleared in prepareToPlay().
- **Logic Pro editor resource leak:** Logic Pro (AU) hides editor without destroying it, leaving background threads running. visibilityChanged() override stops theme prewarm thread when editor becomes invisible.
- **Non-ASCII rendering in JUCE fonts:** em dashes, curly quotes, arrows, and math symbols rendered as garbled text. Replaced with ASCII equivalents.

### Release packaging and external assets (2026-04-09 through 2026-04-17)

Tracked in git on `release_macos_signed_installer.sh`, `build_macos_universal.sh`, `installer/build_installer.sh`, `installer/distribution.xml`, `installer/sign_and_notarize.sh`, `installer/resources/*.html`, and `windows/package_windows_release.ps1` (among others).

- **Stop shipping duplicate heavy art inside every format binary:** Universal macOS release (`scripts/build_macos_universal.sh`, default `CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK=OFF`) and **Windows Release** configures from `windows/build_windows_x64.ps1` / `build_windows_x86.ps1` omit heavy fallback; CMake `juce_add_binary_data(ChoroborosBinaryData)` then embeds only the small “core” set (fonts, toolbar icons, factory JSON, etc.). Large backpanels, knob/mix spritesheets, slider thumbs, and the HQ switch sheet are supplied by the **shared versioned asset pack** installed beside the plug-ins on macOS (`Choroboros-Resources.pkg` / `Library/Application Support/.../Assets/<version>`). Debug Windows builds keep fallback **ON** (CMake default) for convenience. Any configure may override `CHOROBOROS_ALLOW_EMBEDDED_ASSET_FALLBACK` explicitly.
- **Name-based embedded fallback for dev:** `EmbeddedAssetFallback` resolves heavy art through `BinaryData::getNamedResource(...)` (JUCE basename keys such as `green_1_off_png`) instead of direct `BinaryData::` symbol references, so the heavy file list can be omitted from release links without breaking optional dev fallback.
- **Installer resource package and signing fixes:** Staged asset pack matches runtime `AssetLocator` / `AssetRepository` expectations; component `pkgbuild` produces `Choroboros-Resources.pkg` plus per-format packages. Notarization reuses the built `distribution.xml` so a signed product cannot accidentally diverge from an unsigned tree (e.g. AAX strip vs include). Installer HTML and Windows release zips aligned with the beta dual-license story (official binary = EULA; public source = GPLv3), including shipping `EULA.md` beside `LICENSE`/`COPYING` in release archives where applicable.
- **Observed artefact sizes (unsigned installer, macOS universal Release, AAX omitted when unsigned):** universal zip for plug-ins alone on the order of **~31M**; combined unsigned product installer on the order of **~223M** (shared **~192M** resource component plus slim VST3/AU/Standalone components), versus prior **~1.6G**-class installers when each format still embedded the full art payload **and** the resource pack.

### UI: lit knob variants, filmstrip opacity, and HQ composite blending (2026-03-31 through 2026-04-07)

Implementation lives primarily in `CustomLookAndFeel.cpp` / `CustomLookAndFeel.h` (`ThemeAssetPack` and `drawRotarySlider`), `PluginEditorSetup.cpp` (knob IDs and layout), `SmoothedSlider` (visual value equals parameter value for filmstrip sampling), `AnimatedToggleButton` (HQ switch), and `PluginEditor` (HQ animation state fed into the look-and-feel). Related polish landed in git as **“Polish switch smoothness and DSP transitions”** (2026-03-31, `285b04c`) touching `AnimatedToggleButton`, `SmoothedSlider`, `PluginEditor`, and DSP glue.

- **Per-knob “off” vs lit “on” filmstrip assets:** Rate, Depth, Offset, and Width each load a primary spritesheet plus a matching **`*_on`** sheet when present. The mixer knob continues to use its own larger mix spritesheet grid.
- **Adjacent-frame opacity crossfade along the knob arc:** For the main arc, `selectFilmstripFrameBlend` picks the two neighbouring frames and draws them with complementary alpha (`drawBlendedFilmstrip` / `drawBlendedFilmstripWithin`) so motion between discrete frames stays smooth. `Graphics::setOpacity` layers those passes; optional `keepBaseOpaque` keeps the base frame fully opaque while only the incoming frame fades.
- **HQ toggle: composite layer between off and on knob art:** When HQ animation is active and both off and on sheets exist, `drawCrossfadedFilmstripPair` selects the current frame index (snapped from the blend), extracts matching rectangles from **off** and **on** sheets, and **alpha-blends RGBA in premultiplied space** into a temporary ARGB image before drawing once (correct handling of semi-transparent pixels). Otherwise the code falls back to drawing off and on sheets with separate overall alphas driven by HQ state and animation progress (`setHqAnimationState`).
- **HQ switch control:** `AnimatedToggleButton` drives a vertical filmstrip from the shared switch spritesheet, with **sub-frame interpolation** between cached scaled frames (`g.setOpacity` over base and next frame) and timer-based animation toward the target on/off frame; animation progress is exposed for the knob HQ crossfade.
- **Theme asset caching:** Per-engine decoded `ThemeAssetPack` (including the knob sheets above) is cached under a mutex for reuse across editors; images are loaded as **software bitmaps** via `AssetRepository` where required for consistent compositing (see also Windows teardown path that forces software UI images).

---

## [2.04-dev] - 2026-03-16

### Added
- **PresetManager:** New preset management system wrapping factory programs and user presets (XML files on disk). Supports browsing, save, delete, and smart state invalidation — `currentIndex = -1` sentinel means "no preset active". `loadInProgress_` guard prevents re-entrant invalidation when a preset load changes parameter values as a side-effect.
- **TopHeaderBar:** Branded header bar at top of plugin window. Contains Kaizen DSP SVG wordmark logo (brightness-to-alpha pixel conversion strips baked-in black background), preset browser (prev/next chevrons, combo dropdown, save/delete buttons), engine selector slot, and accent-colour-aware custom LookAndFeel with dark gradient background and thin accent separator line.
- **Preset browser UI:** ComboBox dropdown with "Load a preset" placeholder text on init. Selecting a preset shows its name; switching engine colour invalidates the active preset and resets the dropdown to placeholder. Next/prev buttons wrap around; pressing next/prev from no-preset-active state starts from first/last preset respectively.
- **Engine selector in header bar:** Engine colour ComboBox relocated from main content area into the header bar, laid out as part of the centred cluster: `|logo| space |‹|›|preset combo|+|−| gap |engine selector| space |drawer|`. Header bar accepts an externally-owned ComboBox via `setEngineSelector()` and handles styling and layout.
- **Session event log:** Lightweight ring buffer (last 64 events) tracks engine switches, HQ toggles, preset loads, DSP anomalies (NaN/Inf/clipping), and host info. Flushed to disk every 30 s so it survives crashes.
- **Crash reporter:** Platform-specific signal handlers (macOS: SIGSEGV/SIGABRT/SIGFPE/SIGBUS/SIGILL; Windows: SetUnhandledExceptionFilter). On next launch, if a clean-shutdown marker is missing, the user is prompted to send the crash report.
- **Send to Developer (in-app):** Feedback dialog "Send to Developer" button opens the user's default mail client pre-filled with feedback text, usage summary, session log, and system info addressed to info@kaizenstrategic.ai.
- **DSP anomaly detection:** processBlock checks output for NaN, Inf, and sustained hard clipping (>2.0); logs to session log (throttled to 1 event per 2 s).
- **Host info in feedback:** Usage summary now includes DAW name, format (VST3/AU/AAX), sample rate, buffer size, CPU, and RAM.
- **Icon buttons:** Replaced text-based top-bar buttons (DEV, ABOUT, HELP, FEEDBACK) with 50x50 white-on-transparent PNG icons. Normal/hover/pressed states use 55%/100%/40% opacity.
- **TopBarDrawer animated menu:** New `TopBarDrawer` component replaces the static button row. Collapsed state shows a small tab with a left-pointing chevron in the top-right. Clicking expands the drawer leftward with smooth ease-out cubic slide (350 ms), micro-bounce on button positions (easeOutBack ~3.7% overshoot), fade-in (8%→50%), and arrow morphing (left→line→right). Mid-animation reversal supported.
- **Drawer hover-expansion tooltips:** Hovering a button in the expanded drawer slides the component downward to reveal the button's title (engine accent colour, bold). Leaving the drawer collapses the tooltip area back up with a smooth 180 ms animation.
- **Drawer dynamic engine-colour tinting:** Arrow chevron, icon button tint, and tooltip title colour all dynamically match the currently selected engine colour (Green/Blue/Red/Purple/Black) via `setAccentColour()`. Colour updates from three paths: initial construction, `parameterChanged` (preset/automation/host recall), and `engineColorBox.onChange` (direct user interaction) to ensure it never gets stuck.
- **HelpDialog:** New in-app help dialog with documentation link, support email, and feedback form access. Replaces direct URL launch from the help button.
- **`.cursorrules` commit workflow:** 7-step verify-then-commit protocol for Cursor AI, including 8 hard rules (Red defaults sacred, async-signal-safe crash handler, PID-keyed files, process liveness checks, unconditional session logging, crash report preservation, UI scale factor, no force-push), DSP guidelines, code style, and architecture docs.
- **Thiran output lowpass:** Gentle one-pole lowpass (~16 kHz, ~6 dB/oct) on Blue HQ allpass output to attenuate content the allpass passes at unity gain, matching the natural roll-off behaviour of polynomial interpolators (Lagrange, Catmull-Rom).

### Changed
- **Window height:** Plugin window increased by header bar height (36 design-px scaled). All UI Y-coordinates in `PluginEditorSetup::applyLayout()` offset by `getHeaderBarHeight()`.
- **Drawer repositioned into header bar:** TopBarDrawer moved from content area (below header) to inside the header bar, right-aligned and vertically centred.
- **Engine selector styling:** Engine ComboBox now uses the header bar's `HeaderLookAndFeel` (transparent background, accent-coloured arrow, white text) instead of the content area's `customLookAndFeel`. Global popup-menu and tooltip colours still applied via `customLookAndFeel`.
- **Signal chain:** Pre-emphasis moved inside processChorus (wet-path only). Legacy juce::dsp::Compressor replaced with transparent post-sum peak catcher (-2 dB / 2:1 / 4 dB knee / 1 ms attack / 100 ms release).
- **Per-core output trim:** Added virtual getOutputTrim() to ChorusCore base class; applied during crossfade blending.
- **Runtime tuning:** depthSmoothingMs 150→50, depthRateLimit 0.25→2.0, centreDelaySmoothingMs 150→60, tapeDelaySmoothingMs 180→90, tapeWetGain 1.15→1.05, greenBloomGain 0.10→0.05, blueFocusOutputGain 0.08→0.04.
- **Presets:** Green default rate 1.2→0.65 Hz, Black default rate 1.2→0.8 Hz.
- **Engine profiles:** Added migrateKnownBadEngineParamProfiles() to detect and replace stale bundled profiles on load.
- **HQ toggle UI:** Single click now toggles reliably in both directions. Drag threshold 4→12 px. Slider drag sensitivity disabled to prevent accidental value changes.
- **DevPanel UX:** Renamed "Profile"→"Engine", visually separated core assignment as advanced feature, improved tooltips. Factory reset loads from bundled factory sheet instead of regenerating.
- **Top-bar button layout:** All four icon buttons grouped into a single tight row anchored to top-right corner. Icons are 18 design-px with 5 px gaps. Buttons scale from 25%→100% during the slide animation for a polished reveal.
- **Button ownership:** `applyLayout()` no longer touches icon button bounds — the PluginEditor constructor is the single layout owner. Legacy `LayoutTuning` struct fields retained for serialisation compatibility.
- **TopBarDrawer restyled:** Drawer background, border, and inner glow now use HackerTheme colours (gradient from hackerBgElevated to hackerBg, hackerBorder outline) to match the engine colour selector dropdown.
- **AboutDialog restyled:** Migrated from hardcoded dark colours to DevPanel HackerTheme (hackerBg, hackerText, hackerBorder, makeLabelFont). Removed stale "DBA: Green DSP" label and separate contact label. Company shows brand name "Kaizen DSP"; copyright shows legal entity "Kaizen Strategic AI Inc.". Buttons use hacker-styled appearance. Dialog background uses HackerTheme gradient.
- **FeedbackDialog restyled:** Migrated to HackerTheme. Removed beta sign-up link (was pointing to feedback form URL, not sign-up). Renamed "Open Form" button to "Feedback Form" (widened to 110 px). Info text simplified. Text editor and buttons use hacker-styled appearance.
- **Company branding (legal vs brand split):** CMakeLists `COMPANY_NAME` changed to "Kaizen DSP" (brand, DAW-facing). All copyright headers use legal entity "Kaizen Strategic AI Inc." EULA updated from "doing business as Green DSP" to "doing business as Kaizen DSP". README, .cursorrules, docs, and reinstall script updated accordingly. Bundle ID kept as `com.kaizenstrategicai.Choroboros`.
- **Colour slider smoothing:** Increased from 25 ms to 80 ms for visible smooth motion instead of near-instant snap.
- **Crash handler (async-signal-safe):** Signal/exception handler no longer calls `SessionLog::flushToDisk()` (which holds a mutex and allocates). Instead it only calls `unlink()` (POSIX) / `DeleteFileA()` (Windows) to remove the clean-shutdown marker — both are async-signal-safe.
- **Session log PID-keyed files:** Session log files keyed by PID (`session_log_{pid}.json`, `.clean_shutdown_{pid}`). Multiple plugin instances in the same DAW share one file (same process). Different DAWs get separate files.
- **Crash reporter atomic refcount:** `CrashReporter::install()` / `uninstall()` use an `std::atomic<int>` refcount so uninstalling one `PluginProcessor` doesn't remove signal handlers while other instances in the same process are still alive.
- **Crash report preserved until user acts:** `readPendingCrashReport()` no longer deletes the crash file. Separate `clearPendingCrashReport()` is called only after the user sends, saves, or dismisses.
- **Engine/HQ logging from processor:** Engine-switch and HQ-toggle events now logged in `PluginProcessor::parameterChanged()` which covers automation, preset loads, and state restores — not just editor combobox clicks.
- **Unconditional session logging:** Engine-switch and HQ-toggle session-log events fire unconditionally in `parameterChanged()` before the `isBulkChange` early return.
- **Engine selector moved to top-left:** Engine color combo box relocated from bottom-left (Y=335) to top-left (Y=5), mirroring the TopBarDrawer position on the opposite corner. `applyLayout()` forces Y=5 so stale saved-layout JSON can't drag it back.
- **Help button:** Now opens HelpDialog instead of launching a URL directly.
- **Version string build fix:** `CHOROBOROS_VERSION_STRING` changed from a `CACHE STRING` to a normal `set()` variable so that stale `CMakeCache.txt` entries can never override the checked-in version.

### Fixed
- **Drawer border colour stuck on green:** Drawer outer border and tooltip separator line used hardcoded `devpanel::hackerBorder()` (always hacker green) instead of `accentColour_`. Now both use the current engine accent colour so they match Blue/Red/Purple/Black engines.
- **BBD (Red NQ) phaser sweep:** S&H clock images aliased into audio band. Added first-order hold interpolation (~40 dB alias rejection). Fixed a1 coefficient sign in 5th-order Butterworth cascade. Raised bbdClockMinHz 2000→6000, bbdFilterCutoffMinHz 1200→3000.
- **Tape (Red HQ) rate knob:** Undamped phase integrator caused DC drift (~73 samples). Added damping (tapePhaseDamping 1.0→0.99999). Widened LFO smoothing bandwidth (fc 10→56 Hz) so high rates track properly.
- **Thiran (Blue HQ) zippering/noise:** Per-sample 5th-order coefficient recomputation caused DFII-T state transients. Added 32-sample linear coefficient interpolation (~30 dB transient reduction). Delay smoothing 0.998→0.9985.
- **HQ toggle latency:** Quality switch reduced from ~146 ms to ~43 ms (18 ms warmup + 25 ms crossfade). Minimum severity floor 0.40→0.10 for quality toggles.
- **HQ toggle asymmetry (HQ→NQ):** Single click from HQ→NQ position required double-click because `Slider::mouseDown` on a LinearVertical slider snaps value based on Y click position — clicking near the top resolved to value 1.0 (no change), eating the toggle event. Fixed by removing `Slider::mouseDown`/`mouseUp` forwarding entirely; all value changes go through `commitToggleState()` → `setValue()`.
- **Colour slider visual stuck:** Slider thumb appeared to freeze before reaching its target. Two causes: (1) smoothing time 25 ms at 120 Hz with `skip(2)` arrived in ~1.5 frames (invisible smoothing); (2) `SmoothedValue::isSmoothing()` returned false before visual position exactly matched target. Fixed by increasing smoothing to 80 ms, changing to `skip(1)`, and snapping to target when smoothing completes.
- **UTF-8 encoding in dialogs/tooltips:** Raw byte escapes `\xE2\x80\xA2` (bullet), `\xE2\x80\x94` (em-dash), `\xC2\xA9` (copyright) rendered as "â€¢" / "â€"" / "Â©" because JUCE treats `\x` hex escapes as Latin-1. Replaced with `\u2022`, `\u2014`, `\u00A9` Unicode escapes which JUCE handles correctly as code points.
- **Drawer tooltip re-engagement:** After leaving and re-entering the expanded drawer, tooltips would not activate because JUCE routes mouse events to child buttons — the parent's `mouseMove`/`mouseEnter` never fire while hovering a button. Fixed by adding `addMouseListener(this, false)` on each button so the parent receives child mouse events, and hardening `mouseExit` to check `hitTest` before collapsing (prevents false collapse when moving between buttons).
- **Drawer icon resize on engine change:** Switching engine colour while the drawer was open caused button icons to blow up to full image size because `setImages(true, ...)` auto-resized buttons. Fixed by passing `false` for resize and calling `updateButtonStates()` after re-tinting to preserve correct scaled bounds.
- **Orphan-log false positives:** Orphan-log scanner now checks process liveness (`kill(pid, 0)` on POSIX, `OpenProcess`/`GetExitCodeProcess` on Windows) before promoting a session log as a crash report. Previously, if REAPER was still running Choroboros and Ableton opened a new instance, Ableton's scanner would misidentify REAPER's live log as a crash.
- **Drawer buttons non-interactive after expand:** `setInterceptsMouseClicks` always received false because `isTimerRunning()` was still true during the final `updateButtonStates()` call. Fix: re-run `updateButtonStates()` after `stopTimer()` in the animation-complete block.
- **Windows DAW freeze on close:** Four testers (FL Studio, Samplitude, Studio One, Cubase) reported intermittent DAW freeze when closing. Three root causes fixed: (1) `stopDeferredThemePrewarm()` called `join()` on the message thread while the worker waited for `callAsync()` — replaced with `detach()` and a `shared_ptr<atomic<bool>>` per-thread stop flag; (2) static `CriticalSection`, spritesheet `Image`, and `Typeface::Ptr` objects were destroyed under the Windows loader lock during `DLL_PROCESS_DETACH` — replaced with intentional heap leak, instance members, and `SharedResourcePointer` caches; (3) `SessionLog` performed file I/O in its destructor (called during unload) — moved to new `prepareForShutdown()` called early from `~PluginProcessor()` while the message thread is still available.
- **Gesture freeze after engine switch:** After any engine colour switch, Rate and Depth knobs (and all other sliders) became unresponsive to mouse dragging after ~5 minutes; value text-box inputs still worked. Root cause: `updateDSPParameters()` — which runs on the audio thread inside `processBlock()` — called `startTimer(0)` on engine switch. This is a JUCE threading violation (`startTimer` is message-thread-only) and silently replaced the healthy 10 Hz timer from `prepareToPlay()` with a 0 ms interval timer, flooding the message thread with continuous `timerCallback()` dispatches and starving all mouse, paint, and keyboard events. Removed the `startTimer(0)` call; `restoreEngineInternalsToDsp()` already copies tuning values synchronously and the existing 10 Hz timer applies them within 100 ms. Reported by F14 (Chris, Ableton Live / Windows 11 / v2.03 Beta).
- **Default knob drag sensitivity:** Lowered from 40% (factory JSON) / 100% (code fallback) to 31%. Three beta testers reported knobs were too sensitive; suggested range was 23–24%. Settled at 31% as a midpoint.

---

## [2.03] - 2026-03-06

### Added
- **Windows support:** First official Windows release — x64 and x86-compat VST3 + Standalone builds.
- **Platform-specific factory defaults:** Mac and Windows each use bundled factory JSON; embedded fallback when config files absent.
- **Unlock safety warning:** Modal with "Hide this message in the future" and Settings > Safety toggle.
- **Release packaging:** Windows x64/x86 zip packaging script.

### Changed
- **Dev Panel:** Modularized; engine-adaptive theming; lazy tab build; command console replaced Validation tab.
- **Performance:** Deferred Dev Panel creation; theme decode overlapped with host paint; shared theme cache.
- **Editor scale:** Main editor scaled to 91%.

### Fixed
- **Windows:** MSVC lambda captures, BinaryData/getNamedResource, knob anti-aliasing, editor layout/theme APIs.
- **Windows:** Load trace script PowerShell 5.1 compatibility; single-sample timing; percentile helper.

---

## [2.02.2] - 2026-03-04

### Added
- **Dev Panel Console:** Fully interactive command-line interface replacing static recent touches log.
- **Console Interface:** 30+ power user commands including `engine`, `hq`, `set`, `get`, `add`, `sub`, `macro`, `toggle`, `sweep`, `undo`, `redo`, `history`, `solo`, `watch`, `dump`, `diff factory`, `list`, `stats`, and aliases.
- **Interactive Tutorials:** Guided HUD overlay teaching analog DSP concepts (BBD whine, Tape wow/flutter, Phase Width) and dev panel validation workflows.
- **UI Settings Panel:** Surgical Dev Panel preferences including scalable text, color-vision assistance modes, reduced motion, focus rings, custom accent overrides, and safe-reset toggles.

### Changed
- **Dev Panel Layout:** Rebuilt Modulation layout into unified sections; reorganized Look & Feel engine groups; updated Tone tabs to "Engine Response"; simplified fixed-height inspectors.
- **Repo Architecture:** Dropped JUCE submodule in favor of lightweight `FetchContent` to reduce repository bloat.
- **DSP transactions:** Update skips during preset/state/profile transactions so audio thread won't consume intermediate parameter steps mid-switch.
- **Visuals:** Overview and Engine signal-flow Core rows now display exact algorithm names (e.g. "Lagrange 5th", "Thiran Allpass", "Phase Warp").

### Fixed
- **Engine switch artifacts:** Transactional profile apply with guard to prevent partial-state feedback.
- **Engine switch artifacts:** Adaptive warmup and crossfade duration (severity-based).
- **Engine switch artifacts:** Dual old/new rendering during crossfade—old core rendered with frozen old params.
- **Engine switch artifacts:** Old-path LFO phase continuation.
- **Manual Accent:** Manual accent color setting now properly overrides theme resolution rather than being neutralized.
- **Console Sync:** Fixed value text inputs/readouts to follow active engine accent dynamically instead of hardcoded default.
- **AU validation (Logic Pro):** Engine selection parameter now reports `isMetaParameter() = true` (from 2.02.1).

---

## [2.02.1] - 2026-02-26

### Fixed

- **AU validation (Logic Pro):** Engine selection parameter now reports `isMetaParameter() = true`, fixing AU validation failure on macOS Tahoe.

### Chore

- Binary distribution moved to GitHub Releases (fixes 4MB Vercel Blob truncation for large zip).

---

## [2.02-beta] - 2026-02-28

### Changed

- **Engine/dev-panel coherence:** Audit and align engine internals wiring with Dev Panel exposure across faces/HQ-NQ modes
- **BBD filtering:** Remove legacy bbdFilterPoles end-to-end; integrate BBD 5th-order cascade filtering path and dynamic cutoff redesign
- **Color/saturation routing:** Engine "color" modifiers affect wet path only; correct Red HQ/NQ color behavior and tooltip semantics
- **Engine switching:** Reduce artifacts with transition smoothing/crossfade handling; per-engine-color parameter memory (store/restore on return)
- **Typed value edit:** Inverse-map displayed values back to raw APVTS space before write; relax parser hard caps against tuned/displayed ranges
- **Rate quantize menu:** Musical subdivisions (Straight/Triplet/Dotted), cap quantized targets at 20 Hz
- **UI theming:** Plugin dropdown/popup/context/tooltip/callout surfaces match plugin aesthetic; About/Feedback dialogs restyled (Dev Panel excluded)

### Fixed

- Red NQ defaults set to tuned profile values
- Persist/copy/load new BBD-related internals consistently (stages, filter ratio, LPF fields)

### Chore

- Gitignore large third-party docs (PDFs, .url) in Documentation/

---

## [2.01-beta] - 2026

### Added

- **Black engine:** Fifth engine with linear interpolation (Normal) and Linear Ensemble (HQ)
- **Engine-specific slider layout:** Per-engine X, Y, and size controls for the color slider in Dev Panel
- **Defaults persistence:** "Set Current as Defaults" saves layout, tuning, and DSP internals to startup defaults
- **Feedback system:** Beta feedback dialog with usage stats, save-to-file, and email options
- **Beta sign-up link:** Google Form for beta tester registration
- **HQ switch inversion:** Switch up = HQ on, switch down = HQ off (corrected mapping)
- **Per-engine slider thumbs:** Green, Blue, Red, Purple, and Black each have dedicated slider thumb assets

### Changed

- Five engine colors, ten algorithms (was four colors, eight algorithms)
- Version display uses `CHOROBOROS_VERSION_STRING` from CMake for consistency

### Fixed

- Purple engine slider thumb now correctly uses purple asset (was incorrectly using red)
- Slider repaint on engine change for immediate visual update

---

## [1.0.1] - Earlier

- Initial release with four engines (Green, Blue, Red, Purple)
- VST3, AU, Standalone formats
- Presets: Classic, Vintage, Rich, Psychedelic, Duck, Ouroboros
