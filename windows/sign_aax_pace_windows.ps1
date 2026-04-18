#Requires -Version 5.1
<#
.SYNOPSIS
  PACE wraptool sync/sign/verify for a Windows Release AAX bundle (Choroboros).

.DESCRIPTION
  Mirrors the macOS installer/sign_aax_pace.sh flow: wraptool sync, sign, verify.
  Requires PACE Eden wraptool.exe (install Eden locally, on a self-hosted runner, or
  extract a private tools zip — see docs/GITHUB_ACTIONS_WINDOWS_AAX.md).

  Environment (required for signing):
    ILOK_USER, ILOK_PASSWORD, WCGUID
    PACE_SIGNID   — Windows signing identity string for wraptool --signid (see PACE docs)

  Environment (optional):
    WRAPTOOL      — Full path to wraptool.exe (else standard Eden install paths)
    WRAPTOOL_ALLOW_SIGNING_SERVICE=1 — append --allowsigningservice to sign

  Parameters:
    -CMakeBuildDir  Path to cmake -B output (default: build\windows-x64-release)
    -Config         MSBuild config (default: Release)
    -ProductName    Bundle folder name without .aaxplugin (default: Choroboros Beta)
#>
param(
    [string]$CMakeBuildDir = "build\windows-x64-release",
    [string]$Config = "Release",
    [string]$ProductName = "Choroboros Beta"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$aaxRel = Join-Path $CMakeBuildDir (Join-Path "Choroboros_artefacts" (Join-Path $Config (Join-Path "AAX" "${ProductName}.aaxplugin")))
$aaxPath = Join-Path $repoRoot $aaxRel

if (-not (Test-Path -LiteralPath $aaxPath)) {
    throw "AAX bundle not found: $aaxPath (build Choroboros_AAX first)"
}

function Find-Wraptool {
    if (-not [string]::IsNullOrWhiteSpace($env:WRAPTOOL)) {
        $p = $env:WRAPTOOL.Trim()
        if (Test-Path -LiteralPath $p) { return $p }
        throw "WRAPTOOL is set but file not found: $p"
    }
    $candidates = @(
        "${env:ProgramFiles}\PACEAntiPiracy\Eden\Fusion\Current\bin\wraptool.exe",
        "${env:ProgramFiles(x86)}\PACEAntiPiracy\Eden\Fusion\Current\bin\wraptool.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c) { return $c }
    }
    return $null
}

$wraptool = Find-Wraptool
if ($null -eq $wraptool) {
    throw @"
wraptool.exe not found.
  Install PACE Anti-Piracy Eden (Fusion) for Windows, or set WRAPTOOL to wraptool.exe,
  or run windows/ci_install_pace_wraptool.ps1 from CI with a private tools zip.
See docs/GITHUB_ACTIONS_WINDOWS_AAX.md
"@
}

$ilokUser = $env:ILOK_USER
$ilokPass = $env:ILOK_PASSWORD
$wcGuid = $env:WCGUID
$signId = $env:PACE_SIGNID

foreach ($pair in @(
        @{ Name = "ILOK_USER"; Val = $ilokUser },
        @{ Name = "ILOK_PASSWORD"; Val = $ilokPass },
        @{ Name = "WCGUID"; Val = $wcGuid },
        @{ Name = "PACE_SIGNID"; Val = $signId })) {
    if ([string]::IsNullOrWhiteSpace($pair.Val)) {
        throw "Missing required environment variable: $($pair.Name)"
    }
}

Write-Host "AAX bundle: $aaxPath"
Write-Host "wraptool:   $wraptool"

Write-Host "==> wraptool sync"
& $wraptool sync --verbose --account $ilokUser --password $ilokPass
if ($LASTEXITCODE -ne 0) { throw "wraptool sync failed (exit $LASTEXITCODE)" }

$signArgs = @(
    "sign", "--verbose",
    "--account", $ilokUser,
    "--signid", $signId,
    "--wcguid", $wcGuid,
    "--in", $aaxPath,
    "--out", $aaxPath
)
if ($env:WRAPTOOL_ALLOW_SIGNING_SERVICE -eq "1") {
    $signArgs += "--allowsigningservice"
}

Write-Host "==> wraptool sign"
& $wraptool @signArgs
if ($LASTEXITCODE -ne 0) { throw "wraptool sign failed (exit $LASTEXITCODE)" }

Write-Host "==> wraptool verify"
& $wraptool verify --verbose --in $aaxPath
if ($LASTEXITCODE -ne 0) { throw "wraptool verify failed (exit $LASTEXITCODE)" }

Write-Host ""
Write-Host "Done: $aaxPath"
