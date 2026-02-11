# GitHub Actions Quick Start Guide

## ✅ What's Already Set Up

You have **3 GitHub Actions workflows** ready to go:

1. **`build-windows.yml`** - Builds Windows version automatically
2. **`build-linux.yml`** - Builds Linux version automatically  
3. **`build-all-platforms.yml`** - Builds all platforms when you create a release tag

## 🚀 How to Use GitHub Actions

### Option 1: Automatic Builds (Easiest)

**Just push to GitHub!**

```bash
# Make any change and push
git add .
git commit -m "Trigger builds"
git push origin main
```

**What happens:**
- ✅ Windows build starts automatically
- ✅ Linux build starts automatically
- ✅ Both run in parallel
- ✅ Takes ~10-15 minutes each

### Option 2: Manual Trigger

1. Go to your GitHub repository:
   ```
   https://github.com/EsotericShadow/choroboros-open-source
   ```

2. Click the **"Actions"** tab (top menu)

3. Select a workflow:
   - "Build Windows"
   - "Build Linux"
   - "Build All Platforms"

4. Click **"Run workflow"** button (right side)

5. Select branch: `main`

6. Click **"Run workflow"**

### Option 3: Create a Release (Best for Distribution)

When you're ready to release v1.0.1:

```bash
# Create and push a tag
git tag v1.0.1
git push origin v1.0.1
```

**This automatically:**
- ✅ Builds macOS Universal
- ✅ Builds Windows
- ✅ Builds Linux
- ✅ Creates a GitHub Release
- ✅ Attaches all build files

## 📥 How to Download Builds

### From GitHub Actions

1. Go to: `https://github.com/EsotericShadow/choroboros-open-source/actions`

2. Click on a **completed workflow** (green checkmark ✅)

3. Scroll down to **"Artifacts"** section

4. Click the artifact name:
   - `Choroboros-Windows` - Windows build
   - `Choroboros-Linux` - Linux build
   - `Choroboros-macOS` - macOS build (from release)

5. Download the ZIP file

### From GitHub Releases

1. Go to: `https://github.com/EsotericShadow/choroboros-open-source/releases`

2. Click on a release (e.g., "v1.0.1")

3. Scroll down to **"Assets"**

4. Download the platform builds:
   - `Choroboros-v1.0.1-macOS-Universal.zip`
   - `Choroboros-v1.0.1-Windows.zip`
   - `Choroboros-v1.0.1-Linux.zip`

## 🔍 Check Build Status

### View Running Builds

1. Go to: `https://github.com/EsotericShadow/choroboros-open-source/actions`

2. You'll see:
   - 🟡 **Yellow dot** = Running
   - ✅ **Green checkmark** = Success
   - ❌ **Red X** = Failed

3. Click on a workflow to see:
   - Build logs
   - Each step's status
   - Error messages (if any)

### Real-time Updates

- The page auto-refreshes
- You can watch builds in real-time
- See compilation progress

## 🎯 Quick Commands

### Trigger Windows Build
```bash
git commit --allow-empty -m "Trigger Windows build"
git push origin main
```

### Trigger Linux Build
```bash
git commit --allow-empty -m "Trigger Linux build"  
git push origin main
```

### Create Release (All Platforms)
```bash
git tag v1.0.1
git push origin v1.0.1
```

## 📊 Build Times

- **Windows:** ~10-15 minutes
- **Linux:** ~10-15 minutes
- **macOS:** ~15-20 minutes
- **All platforms:** ~20-30 minutes (parallel)

## ⚠️ Troubleshooting

### Build Failed?

1. Click on the failed workflow
2. Expand the failed step
3. Check error messages
4. Common issues:
   - Missing dependencies (should auto-install)
   - CMake errors
   - Compilation errors

### Can't See Actions Tab?

- Make sure you're logged into GitHub
- Check repository permissions
- Actions might be disabled (check Settings > Actions)

### Artifacts Not Appearing?

- Wait for build to complete (check status)
- Artifacts expire after 30 days (90 days for releases)
- Make sure workflow completed successfully

## 🎉 Example: Full Release Workflow

```bash
# 1. Make sure code is committed
git add .
git commit -m "Ready for v1.0.1 release"
git push origin main

# 2. Create release tag
git tag v1.0.1
git push origin v1.0.1

# 3. Wait ~20-30 minutes

# 4. Go to GitHub Releases
# https://github.com/EsotericShadow/choroboros-open-source/releases

# 5. Download all platform builds!

# 6. Upload to Gumroad
```

## 📝 What Each Build Includes

- ✅ VST3 plugin
- ✅ Standalone application
- ✅ README.md
- ✅ DISTRIBUTION.md
- ✅ EULA.md
- ✅ SHA256 checksum

## 🔗 Useful Links

- **Repository:** https://github.com/EsotericShadow/choroboros-open-source
- **Actions:** https://github.com/EsotericShadow/choroboros-open-source/actions
- **Releases:** https://github.com/EsotericShadow/choroboros-open-source/releases

## 💡 Pro Tips

1. **Test before release:** Push to main first, test the builds
2. **Use tags for releases:** Creates proper GitHub Releases
3. **Check logs:** Always review build logs for warnings
4. **Download immediately:** Artifacts expire after 30 days
5. **Version tags:** Use semantic versioning (v1.0.1, v1.0.2, etc.)

---

**Ready to go!** Just push to GitHub and watch the magic happen! 🚀
