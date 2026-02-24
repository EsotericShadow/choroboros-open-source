# GitHub Actions Build Guide

## Overview

GitHub Actions workflows have been set up to automatically build Choroboros for Windows and Linux whenever you push code or create a release tag.

## Workflows

### 1. Individual Platform Builds

**`build-windows.yml`** - Builds Windows version
- Triggers: Push to main, PRs, or manual dispatch
- Output: `Choroboros-v1.0.1-Windows.zip`

**`build-linux.yml`** - Builds Linux version
- Triggers: Push to main, PRs, or manual dispatch
- Output: `Choroboros-v1.0.1-Linux.zip` and `.tar.gz`

### 2. All Platforms Build

**`build-all-platforms.yml`** - Builds macOS, Windows, and Linux
- Triggers: When you push a tag (e.g., `v1.0.1`) or manual dispatch
- Output: All three platform builds
- Creates GitHub Release if triggered by tag

## How to Use

### Option 1: Automatic Builds on Push

Just push to the `main` branch:
```bash
git push origin main
```

GitHub Actions will automatically:
1. Build Windows version
2. Build Linux version
3. Upload artifacts (downloadable for 30 days)

### Option 2: Manual Trigger

1. Go to your GitHub repository
2. Click "Actions" tab
3. Select the workflow you want (e.g., "Build Windows")
4. Click "Run workflow"
5. Select branch and click "Run workflow"

### Option 3: Create a Release (Recommended for Distribution)

1. Create and push a tag:
```bash
git tag v1.0.1
git push origin v1.0.1
```

2. This triggers `build-all-platforms.yml` which:
   - Builds all three platforms
   - Creates a GitHub Release
   - Attaches all build artifacts

## Downloading Builds

### From GitHub Actions

1. Go to your repository on GitHub
2. Click "Actions" tab
3. Click on a completed workflow run
4. Scroll down to "Artifacts"
5. Download the build you need

### From GitHub Releases

1. Go to your repository
2. Click "Releases" (right sidebar)
3. Click on a release
4. Download the platform builds

## Build Artifacts

Each build includes:
- ✅ VST3 plugin
- ✅ Standalone application (Windows/Linux)
- ✅ AU plugin (macOS only)
- ✅ README.md
- ✅ DISTRIBUTION.md
- ✅ EULA.md
- ✅ SHA256 checksum file

## Troubleshooting

### Build Fails

1. Check the "Actions" tab for error messages
2. Common issues:
   - Missing dependencies (should be auto-installed)
   - CMake configuration errors
   - Compilation errors

### Artifacts Not Appearing

- Artifacts are only available for 30 days (90 days for releases)
- Make sure the workflow completed successfully
- Check the "Artifacts" section at the bottom of the workflow run

### Need to Rebuild

- Push a new commit to trigger builds
- Or manually trigger the workflow from the Actions tab

## Next Steps

1. **Test the workflows:**
   ```bash
   git add .github/workflows/
   git commit -m "Add GitHub Actions workflows for Windows and Linux builds"
   git push origin main
   ```

2. **Check the Actions tab** to see builds in progress

3. **Download and test** the Windows/Linux builds

4. **Create a release tag** when ready to distribute:
   ```bash
   git tag v1.0.1
   git push origin v1.0.1
   ```

## Notes

- **Free tier limits:** GitHub Actions provides 2,000 minutes/month for free
- **Build time:** ~10-15 minutes per platform
- **Storage:** Artifacts are stored for 30-90 days depending on workflow
- **Private repos:** Same limits apply, but you can upgrade if needed

## Alternative: Local Builds

If you prefer to build locally, see:
- `build_windows.md` - Windows build instructions
- `build_linux.md` - Linux build instructions
- `build_macos_universal.sh` - macOS universal build script
