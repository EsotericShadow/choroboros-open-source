# Windows Build Instructions

## Prerequisites

1. **Visual Studio 2019 or later** (Community edition is fine)
   - Install "Desktop development with C++" workload
   - Include Windows 10 SDK

2. **CMake 3.22 or later**
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

3. **Git** (if not already installed)

## Build Steps

### Option 1: Using Visual Studio GUI

1. Clone the repo with submodules: `git clone --recursive https://github.com/EsotericShadow/choroboros-open-source.git choroboros`
2. Open Visual Studio
3. File → Open → CMake...
4. Navigate to `choroboros/CMakeLists.txt`
5. Select "x64-Release" configuration
6. Build → Build All

### Option 2: Using Command Line

```powershell
# Clone with submodules (JUCE is required)
git clone --recursive https://github.com/EsotericShadow/choroboros-open-source.git choroboros
cd choroboros

# If already cloned without --recursive:
# git submodule update --init --recursive

# Create build directory
mkdir build
cd build

# Configure CMake
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release --parallel
```

### Option 3: Using build_windows.bat (if created)

```cmd
build_windows.bat
```

## Output Locations

- **VST3:** `build/Choroboros_artefacts/Release/VST3/Choroboros.vst3`
- **Standalone:** `build/Choroboros_artefacts/Release/Standalone/Choroboros.exe`

## Installation Paths

- **VST3:** `C:\Program Files\Common Files\VST3\Choroboros.vst3`
- **Standalone:** User's choice (typically `C:\Program Files\Choroboros\`)

## Notes

- Windows builds only support VST3 and Standalone (no AU on Windows)
- The plugin will need to be code-signed for distribution (optional but recommended)
- Windows Defender may flag unsigned executables

## Packaging

After building, package the files:

```powershell
# Create release directory
mkdir Release

# Copy plugin files
xcopy /E /I "Windows-Build\Choroboros_artefacts\Release\VST3" "Release\VST3"
xcopy /E /I "Windows-Build\Choroboros_artefacts\Release\Standalone" "Release\Standalone"

# Copy documentation
copy README.md Release\
copy DISTRIBUTION.md Release\
copy EULA.md Release\
copy INSTALL.txt Release\

# Create ZIP (requires 7-Zip or WinRAR)
# Or use PowerShell:
Compress-Archive -Path Release\* -DestinationPath Release\Choroboros-v1.0.1-Windows.zip
```
