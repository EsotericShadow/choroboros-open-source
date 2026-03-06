param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [string]$VersionLabel = "v2.03-beta",
    [string]$RepoRoot = "",
    [switch]$SkipX64,
    [switch]$SkipX86
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$PathValue)
    if (-not (Test-Path $PathValue)) {
        throw "Path not found: $PathValue"
    }
    return (Resolve-Path $PathValue).Path
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
        [Parameter(Mandatory = $true)][string[]]$SharedDocs
    )

    $artefactsDir = Join-Path $BuildDir ("Choroboros_artefacts\" + $ConfigName)
    $srcVst3 = Join-Path $artefactsDir "VST3\Choroboros.vst3"
    $srcStandalone = Join-Path $artefactsDir "Standalone\Choroboros.exe"

    if (-not (Test-Path $srcVst3)) {
        throw "Missing VST3 bundle for $ArchLabel: $srcVst3"
    }
    if (-not (Test-Path $srcStandalone)) {
        throw "Missing Standalone executable for $ArchLabel: $srcStandalone"
    }

    $packageName = "Choroboros-$PackageVersion-Windows-$ArchLabel"
    $stageRoot = Join-Path $ReleaseDir $packageName
    $zipPath = Join-Path $ReleaseDir ($packageName + ".zip")
    $hashPath = $zipPath + ".sha256"

    if (Test-Path $stageRoot) { Remove-Item $stageRoot -Recurse -Force }
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    if (Test-Path $hashPath) { Remove-Item $hashPath -Force }

    New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot "VST3") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot "Standalone") | Out-Null

    Copy-Item $srcVst3 (Join-Path $stageRoot "VST3\Choroboros.vst3") -Recurse -Force
    Copy-Item $srcStandalone (Join-Path $stageRoot "Standalone\Choroboros.exe") -Force

    foreach ($doc in $SharedDocs) {
        $leafName = [System.IO.Path]::GetFileName($doc)
        Copy-Item $doc (Join-Path $stageRoot $leafName) -Force
    }

    $buildInfo = @"
Plugin: Choroboros
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
    (Join-Path $RepoRoot "EULA.md"),
    (Join-Path $RepoRoot "COPYING"),
    (Join-Path $RepoRoot "LICENSE"),
    (Join-Path $RepoRoot "windows\README.md"),
    (Join-Path $RepoRoot "windows\HOST_PLUGIN_PATHS.md"),
    (Join-Path $RepoRoot "windows\windows_factory_defaults.json")
) | Where-Object { Test-Path $_ }

$results = New-Object System.Collections.Generic.List[object]
if (-not $SkipX64) {
    $results.Add((New-WindowsPackage -ArchLabel "x64" -BuildDir $x64BuildDir -ConfigName $Config -ReleaseDir $releaseDir -PackageVersion $safeVersion -GitSha $gitSha -BuiltUtcIso $builtUtc -SharedDocs $sharedDocs))
}
if (-not $SkipX86) {
    $results.Add((New-WindowsPackage -ArchLabel "x86-compat" -BuildDir $x86BuildDir -ConfigName $Config -ReleaseDir $releaseDir -PackageVersion $safeVersion -GitSha $gitSha -BuiltUtcIso $builtUtc -SharedDocs $sharedDocs))
}

Write-Host ""
Write-Host "Windows release packages created:" -ForegroundColor Green
foreach ($r in $results) {
    Write-Host ("  [{0}] {1}" -f $r.Arch, $r.Zip)
    Write-Host ("       SHA256: {0}" -f $r.Sha256)
    Write-Host ("       Hash file: {0}" -f $r.HashFile)
}
