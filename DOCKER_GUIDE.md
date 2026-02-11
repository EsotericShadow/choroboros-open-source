# Docker Build and Test Guide

## Overview

Docker containers allow you to build and test Choroboros on Linux without needing a Linux machine. This is perfect for:
- Building Linux versions on macOS/Windows
- Testing Linux builds in isolation
- Ensuring consistent build environments

## Prerequisites

Install Docker Desktop:
- **macOS:** https://www.docker.com/products/docker-desktop/
- **Windows:** https://www.docker.com/products/docker-desktop/
- **Linux:** `sudo apt-get install docker.io docker-compose`

## Quick Start

### Build Linux Version

```bash
# Build using Docker
docker build -f Dockerfile.linux -t choroboros-linux-builder .

# Run the build (outputs to ./Release)
docker run --rm -v $(pwd)/Release:/workspace/Release choroboros-linux-builder

# Or use docker-compose
docker-compose run --rm build-linux
```

### Test Linux Build

```bash
# Build test container
docker build -f Dockerfile.linux-test -t choroboros-linux-tester .

# Run test environment
docker run --rm -it \
  -v $(pwd)/Release:/workspace/Release \
  --network host \
  --privileged \
  choroboros-linux-tester
```

## Using Docker Compose

### Build and Test in One Command

```bash
# Build Linux version
docker-compose run --rm build-linux

# Test the build
docker-compose run --rm test-linux
```

## Manual Build Steps

### Step 1: Build the Plugin

```bash
docker build -f Dockerfile.linux -t choroboros-builder .
```

### Step 2: Run Build Container

```bash
docker run --rm \
  -v $(pwd)/Release:/workspace/Release \
  choroboros-builder
```

### Step 3: Check Output

```bash
ls -lh Release/
# You should see:
# - Choroboros-v1.0.1-Linux.zip
# - Choroboros-v1.0.1-Linux.zip.sha256
```

## Testing the Build

### Option 1: Extract and Inspect

```bash
# Extract the zip
cd Release
unzip Choroboros-v1.0.1-Linux.zip

# Check VST3 structure
ls -R VST3/

# Check standalone
ls -lh Standalone/
```

### Option 2: Test in Container

```bash
# Build test container
docker build -f Dockerfile.linux-test -t choroboros-tester .

# Run interactive test
docker run --rm -it \
  -v $(pwd)/Release:/workspace/Release \
  -e DISPLAY=$DISPLAY \
  --network host \
  choroboros-tester

# Inside container, test with Carla:
carla --load-plugin /workspace/Release/VST3/Choroboros.vst3
```

## Advanced Usage

### Custom Build Options

Edit `Dockerfile.linux` to change:
- Build type (Release/Debug)
- CMake options
- Compiler flags

### Multi-stage Build (Smaller Image)

```dockerfile
# Build stage
FROM ubuntu:22.04 AS builder
# ... build steps ...

# Runtime stage (smaller)
FROM ubuntu:22.04
COPY --from=builder /workspace/Release /Release
```

### Build for Different Linux Distributions

```bash
# Ubuntu 20.04
docker build -f Dockerfile.linux -t choroboros-ubuntu20 --build-arg BASE=ubuntu:20.04 .

# Debian
docker build -f Dockerfile.linux -t choroboros-debian --build-arg BASE=debian:bullseye .
```

## Troubleshooting

### Permission Issues

```bash
# Fix permissions on Release directory
sudo chown -R $USER:$USER Release/
```

### Build Fails

```bash
# Run interactively to debug
docker run --rm -it \
  -v $(pwd):/workspace \
  choroboros-builder bash

# Inside container, run build manually
cmake -B Linux-Build -DCMAKE_BUILD_TYPE=Release
cmake --build Linux-Build
```

### Missing Dependencies

Check the Dockerfile and ensure all dependencies are listed in the `apt-get install` command.

### X11/Display Issues (for GUI testing)

```bash
# Allow X11 connections
xhost +local:docker

# Run with display
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  --network host \
  choroboros-tester
```

## Integration with CI/CD

The GitHub Actions workflows already build Linux automatically, but you can also use Docker in CI:

```yaml
# .github/workflows/docker-build.yml
- name: Build with Docker
  run: |
    docker build -f Dockerfile.linux -t choroboros .
    docker run --rm -v $PWD/Release:/workspace/Release choroboros
```

## File Structure

```
.
├── Dockerfile.linux          # Build container
├── Dockerfile.linux-test     # Test container
├── docker-compose.yml        # Orchestration
├── DOCKER_GUIDE.md          # This file
└── Release/                  # Build output (created)
    ├── Choroboros-v1.0.1-Linux.zip
    ├── Choroboros-v1.0.1-Linux.zip.sha256
    ├── VST3/
    └── Standalone/
```

## Next Steps

1. **Build:** `docker-compose run --rm build-linux`
2. **Test:** Extract and verify the build
3. **Distribute:** Upload to Gumroad or GitHub Releases

## Notes

- Docker builds are slower than native builds but ensure consistency
- The build container is ~2GB (includes all build tools)
- The test container is smaller (~500MB) for runtime testing
- Build artifacts are saved to `./Release` directory
