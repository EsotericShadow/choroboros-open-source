# Legacy Thiran (reference only)

The **previous** Blue HQ Thiran implementation (`ChorusCoreThiranLegacyInactive`) is kept here for **diff / archaeology** against the current shipping core.

**Shipping code:** `../ChorusCoreThiran.cpp` and `../ChorusCoreThiran.h` — **fresh** integer-line + dual Thiran allpass (Laakso/JOS reference model; **not** a copy of this legacy file). This folder remains **diff / archaeology** only.

These `legacy_inactive` files are **not** part of the CMake target. To compare behavior, copy the pair into `blue_engine_modern/`, wire `ChorusDSP::createCoreForId`, and build locally (do not ship both cores under the same `CoreId` without a product decision).
