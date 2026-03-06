# Website Agent Briefing — Download Page Maintenance

**For:** choro-beta-site.vercel.app download page  
**Repo:** https://github.com/EsotericShadow/choroboros-open-source  
**Beta site:** https://choro-beta-site.vercel.app

---

## How Downloads Work

All binaries live on **GitHub Releases**. The download page on the beta site links **directly** to GitHub Release assets.

**Never host binaries on Vercel** — the 4MB blob limit will truncate them (this already happened once with v2.02).

---

## Current Release

- **Tag:** `v2.03-beta`
- **Release page:** https://github.com/EsotericShadow/choroboros-open-source/releases/tag/v2.03-beta

---

## Download URL Pattern

```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/{tag}/{filename}
```

Only `{tag}` and `{filename}` change between releases. The structure stays the same.

---

## Current v2.03-beta Asset URLs

**macOS Universal:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-macOS-Universal.zip
```

**macOS SHA256:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-macOS-Universal.zip.sha256
```

**Windows x64:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x64.zip
```

**Windows x64 SHA256:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x64.zip.sha256
```

**Windows x86:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x86-compat.zip
```

**Windows x86 SHA256:**
```
https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x86-compat.zip.sha256
```

---

## When a New Release Ships

1. Update the **tag** in the URL pattern from old version to new version (e.g. `v2.03-beta` → `v2.04`)
2. Update the **filenames** if they changed (e.g. `Choroboros-v2.04-beta-macOS-Universal.zip`)
3. Update the **version number** displayed on the download page
4. Update the **changelog/release notes** section if one exists on the site
5. Update **platform availability** — e.g. Windows was "coming soon" before v2.03, now it's live
6. **Never change the URL structure** — only the tag and filenames

---

## Current Platform Status (for the site)

| Platform | Status |
|----------|--------|
| macOS Universal (Intel + Apple Silicon) | ✓ Live |
| Windows x64 VST3 | ✓ Live as of v2.03 |
| Windows x86 VST3 | ✓ Live as of v2.03 |
| AAX (Pro Tools) | Coming in V1.0 commercial release |
| Linux | Not planned |

---

## Known Issues to Display on Download Page

- **Ableton Live on Windows:** unverified, feedback welcome
- **macOS code signing pending:** users may need to allow in Security & Privacy settings
- **AAX Windows:** not yet available

---

## HTML Link Rendering

Ensure download links use proper `<a href="...">` attributes pointing to the full GitHub URLs above. Links must be **direct** — not relative paths or placeholders. If links don't render, check that:

1. The `href` contains the full `https://github.com/...` URL
2. No client-side routing or framework is stripping external links
3. The anchor text or button correctly wraps the link element
