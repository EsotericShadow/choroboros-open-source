Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$SkipTests
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

Write-Host "Building Choroboros for Windows x86/Win32 ($Config)" -ForegroundColor Cyan
Write-Host "Note: x86 is optional and intended for legacy host compatibility checks." -ForegroundColor Yellow

& (Join-Path $scriptRoot "preflight.ps1")

$buildDir = Join-Path $repoRoot ("build\windows-x86-" + $Config.ToLowerInvariant())

Write-Host ""
Write-Host "Configure: $buildDir" -ForegroundColor Cyan
cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A Win32

Write-Host ""
Write-Host "Build: VST3 + Standalone" -ForegroundColor Cyan
cmake --build $buildDir --config $Config --target Choroboros_VST3 Choroboros_Standalone --parallel

if (-not $SkipTests) {
    Write-Host ""
    Write-Host "Build: Regression harness" -ForegroundColor Cyan
    cmake --build $buildDir --config $Config --target ChoroborosRegressionTests --parallel
    Write-Host "Skipping automatic regression run for x86 by default."
}

Write-Host ""
Write-Host "x86 build complete." -ForegroundColor Green
