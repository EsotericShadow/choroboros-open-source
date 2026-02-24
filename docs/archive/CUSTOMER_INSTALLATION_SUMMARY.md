# Customer Installation Process

## What Customers Get

When customers download from Gumroad, they get a ZIP file containing:

```
Choroboros-v1.0.1-macOS-Universal.zip
├── VST3/
│   └── Choroboros.vst3          ← Plugin for most DAWs
├── AU/
│   └── Choroboros.component     ← Plugin for Logic Pro
├── Standalone/
│   └── Choroboros.app           ← Standalone application
├── README.md                    ← Full documentation
├── INSTALL.txt                  ← Quick installation guide
├── DISTRIBUTION.md              ← Detailed instructions
├── LICENSE                      ← GPLv3 license
├── COPYING                      ← GPL notice
└── SOURCE_LINK.txt              ← Link to GitHub source
```

## Installation Steps (Simple Version)

**For Customers:**

1. **Download** the ZIP from Gumroad
2. **Unzip** the file (double-click on macOS)
3. **Copy plugins** to the correct locations:
   - **VST3**: Copy `Choroboros.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
   - **AU**: Copy `Choroboros.component` to `~/Library/Audio/Plug-Ins/Components/`
   - **Standalone**: Drag `Choroboros.app` to `/Applications/`
4. **Rescan plugins** in your DAW
5. **Done!** The plugin should appear in your DAW

## Important Notes

- **Not just unzip and go** - plugins must be copied to system plugin folders
- **User-specific location** (`~/Library/`) is recommended (no admin needed)
- **DAW rescan required** - plugins won't appear until DAW rescans
- **macOS security** - may need to allow plugin in System Preferences on first use

## What We Should Tell Customers

**In Gumroad Receipt/Download Page:**

"After downloading:
1. Unzip the file
2. Copy the plugin files to your system plugin folders (see INSTALL.txt)
3. Rescan plugins in your DAW
4. Enjoy!"

**INSTALL.txt** provides the quick reference they need.
