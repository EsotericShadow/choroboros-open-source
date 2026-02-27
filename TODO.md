# TODO

## Engine tuning

- [ ] Tune all 10 engines (5 engines × 2 HQ/NQ variants)

## UI visual tuning

- [ ] Visually tune slider, text, knob sizings and offsets for each engine (per-engine layout in Dev Panel)

## Asset creation (match Green skin)

- [ ] **Red engine:** Final mix, depth, rate, width, offset knobs + toggle switch
- [ ] **Black engine:** Final mix, depth, rate, width, offset knobs + toggle switch
- [ ] **Purple engine:** Final mix, depth, rate, width, offset knobs + toggle switch
- [ ] **Blue engine:** Final mix, depth, rate, width, offset knobs + toggle switch
- [ ] **Backpanels:** Update Red, Black, Purple, Blue backpanels to match Green (2× sizing, consistency)

## Green main knobs (rerender)

- [ ] Rerender green main knobs — paint layer added shadows; 3D UI component builder shadow/lighting is now fixed, so rerender without paint shadows

## Asset resolution & compression

- [ ] Update knob spritesheets to 384px frames (from 512px)
- [ ] Adjust CustomLookAndFeel: `frameWidth`/`frameHeight` and padding for new frame size
- [ ] Compress PNGs and size assets correctly (~2× rendered size)

## Packaging & scripts

- [ ] Fix `scripts/package.sh`: version (1.0.1 → 2.01-beta), build path (`Build/` → `build/`)
- [ ] Verify packaging scripts work end-to-end (package.sh, create_dmg.sh)

## Documentation

- [ ] Add `docs/user/` and `docs/developer/` (referenced in docs/README but missing)
- [x] Add Black engine preset to preset list (README, PluginProcessor)
- [x] Add identifiers to presets: append `(engine_color_name)` to preset names, e.g. `Psychedelic (Purple)`

## Future / optional

- [ ] Windows build (currently macOS only)
- [ ] Linux build (currently macOS only)
- [ ] Expand regression tests (defaults persistence, more engine coverage)
