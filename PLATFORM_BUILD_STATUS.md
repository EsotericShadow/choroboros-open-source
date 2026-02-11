# Platform Build Status

## ✅ Completed Builds

### macOS Universal (Intel + Apple Silicon)
- **Status:** ✅ Complete
- **File:** `Release/Choroboros-v1.0.1-macOS-Universal.zip` (39MB)
- **Architectures:** arm64 + x86_64 (universal binary)
- **Formats:** VST3, AU, Standalone
- **Verified:** All binaries contain both architectures

**To build:**
```bash
./build_macos_universal.sh
```

---

## ❌ Pending Builds

### Windows (x86_64)
- **Status:** ❌ Not built
- **Required:** Windows machine with Visual Studio 2019+ or MinGW
- **Formats:** VST3, Standalone (no AU on Windows)
- **Instructions:** See `build_windows.md`

**Why not done:**
- Requires Windows machine or Windows VM
- Can be done via GitHub Actions CI/CD (recommended)

---

### Linux (x86_64)
- **Status:** ❌ Not built
- **Required:** Linux machine with g++ 7.0+ or Clang 6.0+
- **Formats:** VST3, Standalone (no AU on Linux)
- **Instructions:** See `build_linux.md`

**Why not done:**
- Requires Linux machine or Docker container
- Can be done via GitHub Actions CI/CD (recommended)

---

## Current Release Strategy

### Option 1: macOS Only (Immediate Release)
- ✅ Ready to ship now
- Upload `Choroboros-v1.0.1-macOS-Universal.zip` to Gumroad
- Add Windows/Linux later as updates

### Option 2: Multi-Platform Release (Delayed)
- Wait for Windows and Linux builds
- Create separate product listings or versions
- More complex but reaches more customers

### Option 3: Staged Release
- Release macOS now
- Add Windows when ready
- Add Linux when ready
- Update Gumroad listing as platforms become available

---

## Recommended Next Steps

1. **Immediate:** Upload macOS universal build to Gumroad
2. **Short-term:** Set up Windows build (use Windows VM or GitHub Actions)
3. **Short-term:** Set up Linux build (use Docker or GitHub Actions)
4. **Long-term:** Set up CI/CD for automated multi-platform builds

---

## Gumroad Listing Update

Update the description to reflect current platform support:

**Current:**
- ✅ macOS 10.13+ (Intel & Apple Silicon) - Universal Binary
- ⏳ Windows 10+ (coming soon)
- ⏳ Linux (coming soon)

Or if releasing macOS only:
- ✅ macOS 10.13+ (Intel & Apple Silicon) - Universal Binary
- Windows and Linux versions coming soon!

---

## File Locations

- **macOS Universal:** `Release/Choroboros-v1.0.1-macOS-Universal.zip`
- **Windows:** (not built yet)
- **Linux:** (not built yet)

---

## Build Scripts Available

- ✅ `build_macos_universal.sh` - macOS universal binary
- 📄 `build_windows.md` - Windows build instructions
- 📄 `build_linux.md` - Linux build instructions
