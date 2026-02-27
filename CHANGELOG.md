# Changelog

All notable changes to Choroboros are documented here.

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
