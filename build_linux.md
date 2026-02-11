# Linux Build Instructions

## Prerequisites

Install required dependencies:

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libasound2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev
```

### Fedora/RHEL/CentOS
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    alsa-lib-devel \
    freetype-devel \
    libX11-devel \
    libXcomposite-devel \
    libXcursor-devel \
    libXext-devel \
    libXinerama-devel \
    libXrandr-devel \
    libXrender-devel \
    webkit2gtk3-devel \
    mesa-libGLU-devel
```

### Arch Linux
```bash
sudo pacman -S --needed \
    base-devel \
    cmake \
    alsa-lib \
    freetype2 \
    libx11 \
    libxcomposite \
    libxcursor \
    libxext \
    libxinerama \
    libxrandr \
    libxrender \
    webkit2gtk \
    glu
```

## Build Steps

```bash
# Navigate to project directory
cd /path/to/green_chorus

# Create build directory
mkdir -p Linux-Build
cd Linux-Build

# Configure CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release --parallel

# Or use make directly
make -j$(nproc)
```

## Output Locations

- **VST3:** `Linux-Build/Choroboros_artefacts/Release/VST3/Choroboros.vst3/Contents/x86_64-linux/Choroboros.so`
- **Standalone:** `Linux-Build/Choroboros_artefacts/Release/Standalone/Choroboros`

## Installation Paths

- **VST3:** `~/.vst3/Choroboros.vst3` or `/usr/local/lib/vst3/Choroboros.vst3`
- **Standalone:** User's choice (typically `/usr/local/bin/` or `~/bin/`)

## Notes

- Linux builds only support VST3 and Standalone (no AU on Linux)
- VST3 plugins on Linux are `.so` files inside a `.vst3` bundle
- Some DAWs may require specific VST3 paths
- Consider creating a `.desktop` file for the standalone app

## Packaging

After building, package the files:

```bash
# Create release directory
mkdir -p Release

# Copy plugin files
cp -r Linux-Build/Choroboros_artefacts/Release/VST3 Release/
cp -r Linux-Build/Choroboros_artefacts/Release/Standalone Release/

# Copy documentation
cp README.md DISTRIBUTION.md EULA.md INSTALL.txt Release/

# Create tarball
cd Release
tar -czf ../Choroboros-v1.0.1-Linux.tar.gz *

# Or create ZIP
zip -r ../Choroboros-v1.0.1-Linux.zip *
cd ..
```

## Testing

Test the VST3 plugin:
```bash
# Using Carla (if installed)
carla --load-plugin ~/.vst3/Choroboros.vst3

# Or test standalone
./Linux-Build/Choroboros_artefacts/Release/Standalone/Choroboros
```
