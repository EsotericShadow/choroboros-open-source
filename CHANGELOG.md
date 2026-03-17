# Changelog

All notable changes to Choroboros are documented here.

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
