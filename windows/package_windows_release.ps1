param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [string]$VersionLabel = "v2.05",
    [string]$RepoRoot = "",
    [switch]$SkipX64,
    [switch]$SkipX86
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProductName = "Choroboros Beta"
$ProductSlug = "Choroboros-Beta"

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$PathValue)
    if (-not (Test-Path $PathValue)) {
        throw "Path not found: $PathValue"
    }
    return (Resolve-Path $PathValue).Path
}

function Get-AssetPackVersion {
    param([Parameter(Mandatory = $true)][string]$RepoRoot)

    $headerPath = Join-Path $RepoRoot "Source\Assets\AssetPackVersion.h"
    if (-not (Test-Path $headerPath)) {
        throw "AssetPackVersion.h not found: $headerPath"
    }

    $match = Select-String -Path $headerPath -Pattern 'CHOROBOROS_ASSET_PACK_VERSION "([^"]+)"'
    if (-not $match -or -not $match.Matches.Count) {
        throw "Could not parse CHOROBOROS_ASSET_PACK_VERSION from $headerPath"
    }

    return $match.Matches[0].Groups[1].Value
}

function New-WindowsPackage {
    param(
        [Parameter(Mandatory = $true)][string]$ArchLabel,
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$ConfigName,
        [Parameter(Mandatory = $true)][string]$ReleaseDir,
        [Parameter(Mandatory = $true)][string]$PackageVersion,
        [Parameter(Mandatory = $true)][string]$GitSha,
        [Parameter(Mandatory = $true)][string]$BuiltUtcIso,
        [Parameter(Mandatory = $true)][string[]]$SharedDocs,
        [Parameter(Mandatory = $true)][string]$SharedAssetPackDir
    )

    $artefactsDir = Join-Path $BuildDir ("Choroboros_artefacts\" + $ConfigName)
    $srcVst3 = Join-Path $artefactsDir ("VST3\" + $ProductName + ".vst3")
    $srcStandalone = Join-Path $artefactsDir ("Standalone\" + $ProductName + ".exe")

    if (-not (Test-Path $srcVst3)) {
        throw "Missing VST3 bundle for ${ArchLabel}: $srcVst3"
    }
    if (-not (Test-Path $srcStandalone)) {
        throw "Missing Standalone executable for ${ArchLabel}: $srcStandalone"
    }

    $packageName = "$ProductSlug-$PackageVersion-Windows-$ArchLabel"
    $stageRoot = Join-Path $ReleaseDir $packageName
    $zipPath = Join-Path $ReleaseDir ($packageName + ".zip")
    $hashPath = $zipPath + ".sha256"

    if (Test-Path $stageRoot) { Remove-Item $stageRoot -Recurse -Force }
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    if (Test-Path $hashPath) { Remove-Item $hashPath -Force }

    New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot "Assets") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot "VST3") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot "Standalone") | Out-Null

    Copy-Item $SharedAssetPackDir (Join-Path $stageRoot "Assets") -Recurse -Force
    $stagedVst3 = Join-Path $stageRoot ("VST3\" + $ProductName + ".vst3")
    Copy-Item $srcVst3 $stagedVst3 -Recurse -Force
    Copy-Item (Join-Path $stageRoot "Assets") (Join-Path $stagedVst3 "Assets") -Recurse -Force
    Copy-Item $srcStandalone (Join-Path $stageRoot ("Standalone\" + $ProductName + ".exe")) -Force

    foreach ($doc in $SharedDocs) {
        $leafName = [System.IO.Path]::GetFileName($doc)
        Copy-Item $doc (Join-Path $stageRoot $leafName) -Force
    }

    $buildInfo = @"
Plugin: $ProductName
Package: $packageName
Arch: $ArchLabel
Config: $ConfigName
Commit: $GitSha
BuiltUTC: $BuiltUtcIso
BuildDir: $BuildDir
GeneratedBy: windows/package_windows_release.ps1
"@
    Set-Content -Path (Join-Path $stageRoot "BUILD_INFO.txt") -Value $buildInfo -Encoding UTF8

    Compress-Archive -Path $stageRoot -DestinationPath $zipPath -CompressionLevel Optimal
    $zipHash = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash
    ($zipHash + "  " + [System.IO.Path]::GetFileName($zipPath)) | Set-Content -Path $hashPath -Encoding ASCII

    Remove-Item $stageRoot -Recurse -Force

    return [pscustomobject]@{
        Arch = $ArchLabel
        Zip = $zipPath
        Sha256 = $zipHash
        HashFile = $hashPath
    }
}

if ($SkipX64 -and $SkipX86) {
    throw "Both -SkipX64 and -SkipX86 were set. Package at least one architecture."
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path
} else {
    $RepoRoot = Resolve-ExistingPath -PathValue $RepoRoot
}

$safeVersion = $VersionLabel -replace '[^A-Za-z0-9._-]', '-'
$releaseDir = Join-Path $RepoRoot "Release"
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

$assetPackVersion = Get-AssetPackVersion -RepoRoot $RepoRoot
$assetPackBuildRoot = Join-Path $releaseDir "_asset_pack_build"
if (Test-Path $assetPackBuildRoot) { Remove-Item $assetPackBuildRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $assetPackBuildRoot | Out-Null
py -3 (Join-Path $RepoRoot "scripts\build_asset_pack.py") `
    --source-root (Join-Path $RepoRoot "Assets") `
    --output-root $assetPackBuildRoot `
    --pack-version $assetPackVersion | Out-Null
$sharedAssetPackDir = Resolve-ExistingPath -PathValue (Join-Path $assetPackBuildRoot ("ChoroborosAssets-" + $assetPackVersion))

$x64BuildDir = ""
$x86BuildDir = ""
if (-not $SkipX64) {
    $x64BuildDir = Resolve-ExistingPath -PathValue (Join-Path $RepoRoot ("build\windows-x64-" + $Config.ToLowerInvariant()))
}
if (-not $SkipX86) {
    $x86BuildDir = Resolve-ExistingPath -PathValue (Join-Path $RepoRoot ("build\windows-x86-" + $Config.ToLowerInvariant()))
}

$gitSha = "unknown"
try {
    $gitSha = (git -C $RepoRoot rev-parse --short HEAD).Trim()
} catch {
    $gitSha = "unknown"
}
if ([string]::IsNullOrWhiteSpace($gitSha)) { $gitSha = "unknown" }

$builtUtc = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
$sharedDocs = @(
    (Join-Path $RepoRoot "README.md"),
    (Join-Path $RepoRoot "COPYING"),
    (Join-Path $RepoRoot "LICENSE"),
    (Join-Path $RepoRoot "EULA.md"),
    (Join-Path $RepoRoot "windows\README.md"),
    (Join-Path $RepoRoot "windows\HOST_PLUGIN_PATHS.md"),
    (Join-Path $RepoRoot "windows\windows_factory_defaults.json")
) | Where-Object { Test-Path $_ }

$results = New-Object System.Collections.Generic.List[object]
if (-not $SkipX64) {
    $results.Add((New-WindowsPackage -ArchLabel "x64" -BuildDir $x64BuildDir -ConfigName $Config -ReleaseDir $releaseDir -PackageVersion $safeVersion -GitSha $gitSha -BuiltUtcIso $builtUtc -SharedDocs $sharedDocs -SharedAssetPackDir $sharedAssetPackDir))
}
if (-not $SkipX86) {
    $results.Add((New-WindowsPackage -ArchLabel "x86-compat" -BuildDir $x86BuildDir -ConfigName $Config -ReleaseDir $releaseDir -PackageVersion $safeVersion -GitSha $gitSha -BuiltUtcIso $builtUtc -SharedDocs $sharedDocs -SharedAssetPackDir $sharedAssetPackDir))
}

if (Test-Path $assetPackBuildRoot) { Remove-Item $assetPackBuildRoot -Recurse -Force }

Write-Host ""
Write-Host "Windows release packages created:" -ForegroundColor Green
foreach ($r in $results) {
    Write-Host ("  [{0}] {1}" -f $r.Arch, $r.Zip)
    Write-Host ("       SHA256: {0}" -f $r.Sha256)
    Write-Host ("       Hash file: {0}" -f $r.HashFile)
}
