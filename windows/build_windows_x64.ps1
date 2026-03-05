Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$SkipTests,
    [bool]$DspOnlyTests = $true
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

Write-Host "Building Choroboros for Windows x64 ($Config)" -ForegroundColor Cyan

# Run preflight first so failures are front-loaded.
& (Join-Path $scriptRoot "preflight.ps1")

$buildDir = Join-Path $repoRoot ("build\windows-x64-" + $Config.ToLowerInvariant())

Write-Host ""
Write-Host "Configure: $buildDir" -ForegroundColor Cyan
cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A x64

Write-Host ""
Write-Host "Build: VST3 + Standalone" -ForegroundColor Cyan
cmake --build $buildDir --config $Config --target Choroboros_VST3 Choroboros_Standalone --parallel

if (-not $SkipTests) {
    Write-Host ""
    Write-Host "Build: Regression harness" -ForegroundColor Cyan
    cmake --build $buildDir --config $Config --target ChoroborosRegressionTests --parallel

    Write-Host ""
    Write-Host "Run: Regression harness" -ForegroundColor Cyan
    $args = @()
    if ($DspOnlyTests) {
        $args += "--dsp-only"
    }
    & (Join-Path $scriptRoot "run_regression.ps1") -BuildDir $buildDir -Config $Config -DspOnly $DspOnlyTests
}

Write-Host ""
Write-Host "Build complete." -ForegroundColor Green
Write-Host "Expected VST3 path:"
Write-Host "  $buildDir\Choroboros_artefacts\Release\VST3\Choroboros.vst3"
