# HIL-C — HQ/NQ clicks and GUI ↔ Dev Panel macro parity

## HQ/NQ clicks
- **Path:** `ChorusDSP::switchCore` computes `switchSeverity`, then **longer** warmup/crossfade for `qualityToggleOnly` (65–130 ms warmup, 90–180 ms crossfade, severity-mapped) so cubic/Thiran/Lagrange/tape/etc. handoffs stay masked.
- **Consolidation risk:** shortening these globally regresses clicks; lengthening hurts responsiveness when toggling quality quickly.
- **Status:** No additional severity remap in this pass; Thiran/local fixes reduce buzz independent of HQ/NQ.

## Dev Panel ↔ main UI parity
- **Engine tab** “Main Macro Workbench” exposes Rate, Depth, Offset, Width, Color, Mix via `makeLiveMappedControl` and is on `liveReadoutProperties` for timer refresh.
- **Modulation tab** workbench now includes **Color** and **Mix** with the same IDs and 0–100% display mapping as the engine macro controls, so the modulation deck matches the main macro row for the six primary macro parameters.

## Smallest safe consolidation
- Keep APVTS + `makeLiveMappedControl` as the **single** write path for those parameters; avoid parallel raw setters for the same logical control.
