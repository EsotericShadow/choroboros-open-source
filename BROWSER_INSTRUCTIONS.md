# GitHub Actions - Browser Instructions

## 🚀 Quick Actions in Browser

### Step 1: Open GitHub Actions
**URL:** https://github.com/EsotericShadow/choroboros-open-source/actions

### Step 2: Trigger a Build

#### Option A: Manual Trigger (Recommended)

1. On the Actions page, you'll see 3 workflows:
   - **Build Windows**
   - **Build Linux**  
   - **Build All Platforms**

2. Click on **"Build Windows"** or **"Build Linux"**

3. Click the **"Run workflow"** button (top right, green button)

4. Select branch: `main`

5. Click **"Run workflow"** again

6. Watch it run! 🎉

#### Option B: Create a Release Tag

1. Go to: https://github.com/EsotericShadow/choroboros-open-source/releases

2. Click **"Create a new release"**

3. Fill in:
   - **Tag version:** `v1.0.1`
   - **Release title:** `Choroboros v1.0.1`
   - **Description:** (optional)

4. Click **"Publish release"**

5. This triggers **"Build All Platforms"** automatically!

### Step 3: Watch the Build

1. Go back to: https://github.com/EsotericShadow/choroboros-open-source/actions

2. Click on the running workflow (yellow dot 🟡)

3. You'll see:
   - Real-time build progress
   - Each step executing
   - Logs from compilation

4. Wait ~10-15 minutes for completion

### Step 4: Download Builds

1. Once complete (green checkmark ✅), click the workflow

2. Scroll down to **"Artifacts"** section

3. Click the artifact name:
   - `Choroboros-Windows` → Downloads Windows ZIP
   - `Choroboros-Linux` → Downloads Linux ZIP

4. Extract and use!

## 📋 What You'll See

### Workflow Status Icons:
- 🟡 **Yellow dot** = Running (in progress)
- ✅ **Green checkmark** = Success (completed)
- ❌ **Red X** = Failed (check logs)

### Build Steps:
1. ✅ Checkout code
2. ✅ Install dependencies
3. ✅ Configure CMake
4. ✅ Build plugin
5. ✅ Package release
6. ✅ Upload artifacts

## 🎯 Recommended Workflow

### For Testing:
1. Click **"Build Windows"** → **"Run workflow"**
2. Wait ~15 minutes
3. Download and test

### For Release:
1. Go to **Releases** → **"Create a new release"**
2. Tag: `v1.0.1`
3. Publish
4. Wait ~30 minutes (all platforms)
5. Download all builds from Releases page

## 🔗 Direct Links

- **Actions:** https://github.com/EsotericShadow/choroboros-open-source/actions
- **Releases:** https://github.com/EsotericShadow/choroboros-open-source/releases
- **Repository:** https://github.com/EsotericShadow/choroboros-open-source

## 💡 Pro Tips

1. **Watch builds live:** Click on a running workflow to see real-time logs
2. **Check logs:** If build fails, expand the failed step to see errors
3. **Download quickly:** Artifacts expire after 30 days
4. **Use releases:** Creates permanent download links

---

**The browser should now be open!** Follow the steps above to trigger your first build! 🚀
