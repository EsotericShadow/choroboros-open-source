# Multi-Platform Build Plan

## Current Status
- ✅ macOS arm64 (Apple Silicon) - Built
- ❌ macOS x86_64 (Intel) - Not built
- ❌ macOS Universal (arm64 + x86_64) - Not built
- ❌ Windows (x86_64) - Not built
- ❌ Linux (x86_64) - Not built

## Required Builds

### 1. macOS Universal Binary
- **Architectures:** arm64 + x86_64
- **Formats:** VST3, AU, Standalone
- **Build Command:** `cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`
- **Output:** Single binary that works on both Intel and Apple Silicon Macs

### 2. Windows
- **Architecture:** x86_64
- **Formats:** VST3, Standalone
- **Build Requirements:** Visual Studio 2019+ or MinGW
- **Output:** `.vst3` bundle and `.exe` standalone

### 3. Linux
- **Architecture:** x86_64
- **Formats:** VST3, Standalone
- **Build Requirements:** g++ 7.0+ or Clang 6.0+
- **Output:** `.so` files for VST3 and standalone

## Build Strategy

### Option A: Build on Each Platform (Recommended)
- Build macOS universal on macOS
- Build Windows on Windows (or use GitHub Actions)
- Build Linux on Linux (or use Docker/GitHub Actions)

### Option B: Cross-Compilation
- More complex, requires toolchains
- Not recommended for initial release

## Implementation Steps

1. **Update CMakeLists.txt** - Add universal macOS support
2. **Create build scripts** for each platform
3. **Update package.sh** - Handle multiple platforms
4. **Create GitHub Actions workflow** (optional, for CI/CD)
5. **Update Gumroad listing** - Specify platform support

## File Structure After Builds

```
Release/
├── Choroboros-v1.0.1-macOS-Universal.zip
│   ├── VST3/Choroboros.vst3 (universal)
│   ├── AU/Choroboros.component (universal)
│   └── Standalone/Choroboros.app (universal)
├── Choroboros-v1.0.1-Windows.zip
│   ├── VST3/Choroboros.vst3
│   └── Standalone/Choroboros.exe
└── Choroboros-v1.0.1-Linux.zip
    ├── VST3/Choroboros.vst3
    └── Standalone/Choroboros
```

## Next Steps

1. Build macOS universal binary (can do now)
2. Set up Windows build environment
3. Set up Linux build environment
4. Package all platforms
5. Update distribution documentation
