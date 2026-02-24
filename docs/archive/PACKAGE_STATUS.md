# ⚠️ Package Status

## Current macOS Universal Package

**Location:** `Release/Choroboros-v1.0.1-macOS-Universal.zip`

**Size:** 13 MB (INCOMPLETE - should be ~39 MB)

### What's Included ✅
- ✅ Standalone application (Choroboros.app)
- ✅ Documentation (README.md, DISTRIBUTION.md, INSTALL.txt)
- ✅ GPL files (LICENSE, COPYING, SOURCE_LINK.txt)
- ✅ Installer scripts (install.sh, Install Choroboros.command)

### What's Missing ❌
- ❌ VST3 plugin binary
- ❌ AU plugin binary

## To Fix

The build didn't complete fully. You need to rebuild:

```bash
./scripts/build_macos_universal.sh
```

This will take a few minutes but will create a complete package with all formats.

## Alternative

If you have a previous complete package (39 MB), you can add the installer scripts to it manually.
