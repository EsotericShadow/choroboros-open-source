# v2.05 Release Blockers: Defaults, Install Behavior, and Regression Audit

**Date:** 2026-04-18  
**Status:** HIGH PRIORITY - do not rerun the `v2.05` release final until the items in this document are resolved

---

## Why this document exists

This is the durable record of the release-blocking discoveries made during the `v2.05` release push.

It answers:

1. what the plugin actually uses as the factory default for new users
2. what happens for users who already have Choroboros installed
3. whether current installers/unpacked releases uninstall old versions or just replace files
4. which Dev Panel / DSP regressions must be fixed before the release is rerun

This doc is intentionally explicit so the answers do not get lost between release attempts.

**DSP triage briefs (HIL):** `docs/HIL_V205_A_THIRAN_BLUE_HQ.md`, `docs/HIL_V205_B_DEPTH_COLOR_ZIPPER.md`, `docs/HIL_V205_C_HQ_NQ_AND_DEVPANEL.md`.

---

## Current release decisions

- **Final factory JSON is deferred for now.** Gabriel will retune it in the Dev Panel after the regressions below are fixed.
- **Windows AAX remains deferred** for this release cycle.
- **Existing installs now have a one-shot force-refresh mechanism** keyed to the bundled factory JSON content. When the shipped factory JSON changes, persisted defaults are overwritten once to match it.

---

## Factory default source of truth

### Repo source of truth

The maintained factory-default quartet is:

- `Assets/defaults_factory_mac.json`
- `json_defaults_dump.json`
- `linux/linux_factory_defaults.json`
- `windows/windows_factory_defaults.json`

These four files must stay byte-identical. See:

- `docs/FACTORY_DEFAULTS_JSON_SYNC.md`
- `scripts/verify_factory_json_sync.sh`

### What is embedded into builds

At build time, the plugin embeds platform-specific BinaryData inputs:

- macOS: `json_defaults_dump.json`
- Linux: `linux/linux_factory_defaults.json`
- Windows: `windows/windows_factory_defaults.json`

`Assets/defaults_factory_mac.json` is the editorial canonical copy, but it is **not** the direct runtime resource by itself.

### What new users actually load on launch

For a truly new user, the plugin does **not** re-read the bundled JSON on every launch.

The actual startup behavior is:

1. `seedPersistedDefaultsFromBundledFactory()` checks whether `defaults_user.json` and `defaults_factory.json` already exist and are non-empty.
2. If either is missing or empty, the plugin seeds the missing file(s) from the embedded factory JSON.
3. After that, startup loads persisted defaults in this order:
   - `defaults_user.json`
   - if that is unavailable, `defaults_factory.json`
   - if that is unavailable, embedded factory JSON

Relevant code:

- `Source/Plugin/PluginProcessor.cpp`
- `Source/Config/DefaultsPersistence.cpp`

### Important implication

The bundled factory JSON is the **seed source for new installs**, but it is **not** the live source consulted on every launch once persisted defaults exist.

After first-run seeding, persisted files win.

---

## Existing users: what happens today

This applies to users upgrading from older releases such as:

- `v2.00`
- `v2.01`
- `v2.01.1`
- `v2.02`
- `v2.02.1`
- `v2.03`
- `v2.03.1`
- `v2.04`
- `v2.04.1`

### There is no version-specific defaults migration path today

There is no code branch that says "if upgrading from `v2.03` do X" or "if upgrading from `v2.04.1` do Y."

The current logic is path-based, not version-based:

- if persisted defaults files already exist, they stay in control
- if they do not exist, the embedded factory seeds them

### What this means in practice

If an existing user already has:

- `defaults_user.json`, or
- `defaults_factory.json`

then older builds would historically keep those files in control unless something explicitly overwrote them.

That means:

- older binaries could ship with new factory JSON
- but an existing user could still hear/see old defaults because their persisted files overrode the embedded factory

### Legacy `defaults.json`

There is a legacy migration path in `DefaultsPersistence::loadUser()` for `defaults.json`.

However, the processor currently seeds `defaults_user.json` and `defaults_factory.json` **before** calling the load path, so older installs that only still have `defaults.json` should be treated carefully and explicitly tested during the `v2.05` migration work.

### Current `v2.05` answer to the upgrade question

`v2.05` now contains a deliberate **force-refresh existing installs** path inside `seedPersistedDefaultsFromBundledFactory()`.

The mechanism is:

1. load the bundled factory JSON from BinaryData
2. compute a content hash from that bundled JSON
3. compare it against a persisted marker file:
   - `defaults_factory_seed_hash.txt`
4. if the hash changed (or the marker does not exist yet), force-write:
   - `defaults_factory.json`
   - `defaults_user.json`
5. update `defaults_factory_seed_hash.txt` so the refresh does **not** happen again on every launch

That makes the upgrade policy **one-shot and content-based**, not "overwrite every startup."

### Backups before force-refresh

When `v2.05` force-refreshes persisted defaults because the bundled factory changed, it also creates pre-refresh backups for non-matching on-disk JSON:

- `defaults_user_pre_factory_refresh_<hash>_<timestamp>.json`
- `defaults_factory_pre_factory_refresh_<hash>_<timestamp>.json`

These backups live in the same user config directory as the defaults files.

### Practical effect for users upgrading from `v2.00` through `v2.04.1`

Under the new `v2.05` code path, existing installs are **not sticky anymore once the shipped bundled factory changes**.

On the first launch of the final `v2.05` build:

- existing `defaults_user.json` is overwritten to the bundled factory baseline
- existing `defaults_factory.json` is overwritten to the bundled factory baseline
- the migration marker is written
- future launches do **not** repeat the overwrite unless the shipped factory JSON changes again

---

## Installer / package behavior by platform

## macOS signed installer (`.pkg`)

The macOS release installer built by:

- `scripts/release_macos_signed_installer.sh`
- `installer/build_installer.sh`
- `installer/distribution.xml`

installs system-wide components to standard locations:

- VST3: `/Library/Audio/Plug-Ins/VST3/`
- AU: `/Library/Audio/Plug-Ins/Components/`
- Standalone: `/Applications/`
- AAX: `/Library/Application Support/Avid/Audio/Plug-Ins/`

### Does it fully uninstall first?

**No, not in the "wipe everything" sense.**

What it effectively does is **replace payloads at the install destinations** through normal installer overwrite behavior.

It does **not** currently imply:

- removal of user config defaults files
- a cross-version cleanup of every possible old path
- a dedicated uninstaller

So the current mac `.pkg` should be thought of as **replace installed bundles in place**, not **uninstall the product and reset user state**.

## macOS manual `install.sh`

`install.sh` is more explicit for user-level installs:

- removes the current beta bundle if present
- otherwise removes a detected legacy `Choroboros` bundle
- copies the new bundle into place

This is a **replace current bundle** flow, not a "remove all traces of prior versions" flow.

It also does **not** touch persisted defaults under the user config directory.

## macOS AAX helper

`installer/install_aax_pro_tools_system.sh` explicitly:

- `sudo rm -rf`s the destination AAX bundle
- `ditto`s the new one into `/Library/Application Support/Avid/Audio/Plug-Ins/`

So AAX is a direct **replace that bundle** operation.

## Windows

Current Windows release packaging is zip-based:

- `windows/package_windows_release.ps1`
- GitHub Actions `build.yml`

There is **no end-user Windows installer** in the repo for the current release flow.

That means there is **no repo-managed uninstall step** on Windows right now.

The shipped Windows artifacts are archive/manual-install style bundles:

- replace/copy the VST3
- replace/copy the standalone executable

Any old install cleanup is manual unless the user/script does it.

## Linux

Current Linux delivery is also archive/manual-install style.

There is no Linux uninstall or migration script in the repo for the current release path.

So Linux is also **manual replace**, not "uninstall old version first."

---

## User config paths that can override the new factory

### macOS

- `~/Library/Choroboros/defaults_user.json`
- `~/Library/Choroboros/defaults_factory.json`
- `~/Library/Choroboros/defaults.json`

### Windows

- `%APPDATA%\\Choroboros\\defaults_user.json`
- `%APPDATA%\\Choroboros\\defaults_factory.json`
- `%APPDATA%\\Choroboros\\defaults.json`

### Linux

- `~/.config/Choroboros/defaults_user.json`
- `~/.config/Choroboros/defaults_factory.json`
- `~/.config/Choroboros/defaults.json`

These paths are the reason a new build can appear to "ignore" updated factory JSON on an existing machine.

---

## Release-blocking regressions reported today

These are the regressions that must be addressed before the release final is rerun.

## 1) Dev Panel / GUI macro parity

When changing the macros from the Dev Panel, the GUI/spritesheet-linked macro controls are not staying in parity.

Problems reported:

- GUI changes are not always reflected back into the Dev Panel
- some values reflect, some do not
- the full cross-update matrix needs a systematic parity audit

This must be audited across:

- Rate
- Depth
- Offset
- Width
- Color
- Mix

in both directions:

- GUI -> Dev Panel
- Dev Panel -> GUI

## 2) Color macro sound-shaping regressions

Reported behavior:

- Green color slider effect is not apparent
- Blue color slider effect is not apparent
- Red saturation is mostly making output louder instead of adding the intended compression/character
- output level should remain approximately gain-compensated relative to input, except for small chorus-related variation

## 3) Blue HQ Thiran distortion

Reported behavior:

- Blue HQ (Thiran) still has a distortion artifact that should not be there
- this needs immediate correction

## 4) HQ/NQ click/pop regressions

Reported behavior:

- Black: no click when switching NQ -> HQ, but a click/pop when switching HQ -> NQ
- Red: clicks in both directions
- Green: clicks in both directions
- Blue: click from NQ -> HQ, and Blue HQ distortion may be related

The crossfade/switching math needs to be re-checked against the modulation path and curve behavior.

## 5) Zipper-noise regressions

Reported behavior:

- Depth zipper:
  - Green HQ
  - Green NQ
  - Blue NQ
  - Red HQ
  - Black NQ
- Color zipper:
  - Blue NQ
  - Green HQ
  - Green NQ

Additionally, those color controls were reported as having zipper noise **and** weak/no audible effect.

---

## Release-blocking implementation order

1. document the current behavior so it is not lost
2. fix Dev Panel / GUI macro parity
3. fix color behavior and Blue HQ distortion
4. fix HQ/NQ clicks and zipper noise
5. let Gabriel retune the final factory JSON in the Dev Panel
6. propagate the final JSON across the four synced factory files
7. implement the `v2.05` force-refresh upgrade behavior for existing installs
8. rerun the release final

---

## References

- `docs/FACTORY_DEFAULTS_JSON_SYNC.md`
- `Source/Plugin/PluginProcessor.cpp`
- `Source/Config/DefaultsPersistence.cpp`
- `Source/UI/DevPanelTabBuilders.cpp`
- `Source/UI/DevPanelRuntime.cpp`
- `Source/UI/PluginEditor.cpp`
- `Source/DSP/ChorusDSP.cpp`
- `Source/DSP/ChorusDSPProcess.cpp`
- `Source/Cores/blue_engine_modern/ChorusCoreThiran.cpp`
- `install.sh`
- `installer/install_aax_pro_tools_system.sh`
- `installer/distribution.xml`
- `windows/package_windows_release.ps1`
