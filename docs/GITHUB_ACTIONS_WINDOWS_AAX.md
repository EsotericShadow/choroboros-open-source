# Windows AAX in GitHub Actions

**PACE materials:** PACE Central downloads and the AAX Code Signing manuals are **confidential** under your PACE agreement. Do **not** commit installers, manuals, or license keys into this repo—only store CI credentials in **GitHub Secrets** / **Variables**.

This repo builds **unsigned** Windows AAX on every **Windows x64** CI job (`Choroboros_AAX`). JUCE 8 bundles the AAX SDK headers/libs under the fetched JUCE tree, so you **do not** need to upload the full Avid AAX SDK zip for a basic CI build.

**PACE signing** (wraptool) is **optional** and controlled by a repository **variable** plus **secrets**. GitHub-hosted `windows-latest` does **not** include PACE Eden; typical approaches:

1. **Self-hosted Windows runner** — Install **Eden SDK Lite** for Windows from PACE Central (`EdenSDKLiteInstallerWin64.zip` per PACE’s “Install the Tools” flow). PACE documents **silent / unattended** installs for CI; expect tools under something like `C:\Program Files (x86)\PACEAntiPiracy\Eden\Fusion\Versions\<major>\bin\wraptool.exe` (version folder may change—set `WRAPTOOL` explicitly if needed). **Eden Tools** license must be active on that machine (**iLok USB** or **iLok Cloud** session, per PACE).
2. **PACE Cloud Signing Service** — For cloud agents that cannot use a USB, PACE describes a **Cloud Signing** option; contact PACE sales/support per their docs (`--allowsigningservice` in our script maps to that class of workflow when enabled).
3. **`PACE_WRAPTOOL_ZIP_URL`** — Only if you have a **private** layout you are allowed to host; respect PACE redistribution terms.

Follow **PACE license terms** for redistributing or copying wraptool.

---

## What CI does

| Step | When |
|------|------|
| `cmake --build … --target Choroboros_AAX` | Always on Windows x64 (`build.yml` + `release.yml`). |
| Download wraptool zip | Only if `WINDOWS_AAX_SIGNING_ENABLED` **variable** is `true` **and** secret `PACE_WRAPTOOL_ZIP_URL` is non-empty. |
| `windows/sign_aax_pace_windows.ps1` | Only if `WINDOWS_AAX_SIGNING_ENABLED` == `true`. |
| Zip contents | VST3, Standalone, and **AAX bundle** (if the build produced it). |

---

## 1) Repository variable (enable signing)

GitHub → **Settings** → **Secrets and variables** → **Actions** → **Variables** tab.

| Name | Value | Purpose |
|------|--------|---------|
| `WINDOWS_AAX_SIGNING_ENABLED` | `true` | Turns on PACE steps. Omit or set anything other than `true` to **skip** signing (still builds unsigned AAX). |

Optional:

| Name | Value | Purpose |
|------|--------|---------|
| `WRAPTOOL_ALLOW_SIGNING_SERVICE` | `1` | Passed through to wraptool as `--allowsigningservice` when your PACE workflow requires it (match local env from macOS if you use it). |

---

## 2) Repository secrets (PACE + optional zip)

**Settings** → **Secrets and variables** → **Actions** → **Secrets** tab → **New repository secret**.

| Secret | Description |
|--------|--------------|
| `ILOK_USER` | Same iLok account email/username you use for macOS `sign_aax_pace.sh`. |
| `ILOK_PASSWORD` | iLok / PACE account password (consider a **dedicated CI** account if PACE allows). |
| `WCGUID` | Wrap config GUID for this product (same as macOS `.env`). |
| `PACE_SIGNID` | Windows **`--signid`** string for wraptool (Authenticode subject / publisher string or thumbprint — use the exact form **PACE documents** for Windows; it may differ from the macOS “Developer ID Application: …” string). |
| `PACE_WRAPTOOL_ZIP_URL` | Optional. HTTPS URL to a **private** zip of wraptool + deps. If unset, CI expects a normal Eden/Fusion install on the runner (typ. under **Program Files (x86)** per PACE). |
| `PACE_WRAPTOOL_ZIP_TOKEN` | Optional. If the zip URL requires auth, set a **Bearer** token; the script sends `Authorization: Bearer <token>`. |

**Security**

- Restrict which workflows can use these secrets: **Environments** (e.g. `release-signing`) with required reviewers, and attach the environment to the job in YAML if you harden further.  
- Never commit `.env`, PFX, or wraptool binaries to the public tree.

---

## 3) Platform signing vs wraptool (PACE)

Per PACE’s **Get Signing Certificates** / signing overview: **`wraptool sign` performs the Windows platform digital signature (e.g. via SignTool) and then applies PACE’s publisher signature**. You do **not** need a separate “SignTool pass after wraptool” for the normal PACE workflow—configure certificates the way PACE documents (`--signid`, certified iLok, or Cloud Signing as applicable).

If PACE support gives you an **exception** workflow, adjust your pipeline accordingly; this repo only automates `sync` → `sign` → `verify`.

---

## 4) Local test (same as CI)

From repo root on Windows (after a Release build of AAX):

```powershell
$env:ILOK_USER = "…"
$env:ILOK_PASSWORD = "…"
$env:WCGUID = "…"
$env:PACE_SIGNID = "…"   # per PACE Windows docs
# optional (example layout — verify on your machine / PACE install):
# $env:WRAPTOOL = "C:\Program Files (x86)\PACEAntiPiracy\Eden\Fusion\Versions\5\bin\wraptool.exe"
.\windows\sign_aax_pace_windows.ps1 -CMakeBuildDir "build\windows-x64-release"
```

---

## 5) Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Configure fails on AAX | Rare if JUCE FetchContent is used; local `JUCE` path must still expose bundled AAX SDK. |
| Build `Choroboros_AAX` fails | MSVC / SDK path / JUCE version; fix locally first. |
| `wraptool.exe not found` | Eden not installed on runner and `PACE_WRAPTOOL_ZIP_URL` missing or zip layout wrong. |
| `wraptool sign` fails | Wrong `PACE_SIGNID`, bad `WCGUID`, or iLok credentials / license not valid on CI machine. |
| Signing works locally but not in CI | Use self-hosted runner with same Eden + cert setup, or fix zip/token URL. |

---

## 6) Scripts in this repo

- `windows/sign_aax_pace_windows.ps1` — sync / sign / verify.  
- `windows/ci_install_pace_wraptool.ps1` — CI-only: download zip, set `WRAPTOOL` in `GITHUB_ENV`.  
- macOS reference: `installer/sign_aax_pace.sh`.
