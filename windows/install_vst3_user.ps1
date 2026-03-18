Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
    [string]$BuildDir = ""
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build\windows-x64-release"
}

$vst3Candidates = Get-ChildItem -Path $BuildDir -Recurse -Directory -Filter "Choroboros.vst3" -ErrorAction SilentlyContinue
if ($null -eq $vst3Candidates -or $vst3Candidates.Count -eq 0) {
    throw "No Choroboros.vst3 directory found under '$BuildDir'. Build VST3 first."
}

$source = ($vst3Candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
$destRoot = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
$dest = Join-Path $destRoot "Choroboros.vst3"

Write-Host "Installing VST3 bundle to user path..." -ForegroundColor Cyan
Write-Host "Source: $source"
Write-Host "Dest:   $dest"

New-Item -Path $destRoot -ItemType Directory -Force | Out-Null
if (Test-Path $dest) {
    Remove-Item -Path $dest -Recurse -Force
}

Copy-Item -Path $source -Destination $dest -Recurse -Force

Write-Host "Install complete. Rescan VST3 plugins in your DAW." -ForegroundColor Green
