Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
    [string]$BuildDir = "",
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [bool]$DspOnly = $true
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot ("build\windows-x64-" + $Config.ToLowerInvariant())
}

$candidates = @(
    (Join-Path $BuildDir ($Config + "\ChoroborosRegressionTests.exe")),
    (Join-Path $BuildDir "ChoroborosRegressionTests.exe")
)

$testExe = $null
foreach ($c in $candidates) {
    if (Test-Path $c) {
        $testExe = $c
        break
    }
}

if ($null -eq $testExe) {
    throw "Could not find ChoroborosRegressionTests.exe in expected paths under '$BuildDir'."
}

Write-Host "Running regression harness: $testExe" -ForegroundColor Cyan

$args = @()
if ($DspOnly) {
    $args += "--dsp-only"
}

& $testExe @args
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "Regression harness failed with exit code $exitCode"
}

Write-Host "Regression harness passed." -ForegroundColor Green
