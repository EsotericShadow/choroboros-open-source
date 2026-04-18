#Requires -Version 5.1
<#
.SYNOPSIS
  Download and unpack a private zip that contains PACE wraptool.exe (+ deps) for GitHub Actions.

.DESCRIPTION
  GitHub-hosted runners do not include PACE Eden. Host a zip (from your licensed Eden
  install layout, per PACE license terms) in private storage and pass URL + optional token.

  Environment:
    PACE_WRAPTOOL_ZIP_URL     — HTTPS URL to zip (required)
    PACE_WRAPTOOL_ZIP_TOKEN   — Optional Bearer token (sent as Authorization: Bearer …)
    GITHUB_ENV                — Set by Actions; adds WRAPTOOL=… for later steps

  The zip should expand so wraptool.exe exists at one of:
    <root>\wraptool.exe
    <root>\bin\wraptool.exe
    <root>\PACEAntiPiracy\Eden\Fusion\Current\bin\wraptool.exe
    <root>\PACEAntiPiracy\Eden\Fusion\Versions\5\bin\wraptool.exe
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$url = $env:PACE_WRAPTOOL_ZIP_URL
if ([string]::IsNullOrWhiteSpace($url)) {
    throw "PACE_WRAPTOOL_ZIP_URL is not set"
}
if ([string]::IsNullOrWhiteSpace($env:GITHUB_ENV)) {
    throw "GITHUB_ENV is not set (run this script only from GitHub Actions)"
}

$destRoot = Join-Path $env:RUNNER_TEMP "pace-wraptool"
$zipPath = Join-Path $env:RUNNER_TEMP "pace-wraptool.zip"
if (Test-Path $destRoot) { Remove-Item -Recurse -Force $destRoot }
New-Item -ItemType Directory -Force -Path $destRoot | Out-Null

$headers = @{ "User-Agent" = "Choroboros-CI" }
$token = $env:PACE_WRAPTOOL_ZIP_TOKEN
if (-not [string]::IsNullOrWhiteSpace($token)) {
    $headers["Authorization"] = "Bearer $token"
}

Write-Host "Downloading wraptool zip…"
Invoke-WebRequest -Uri $url -Headers $headers -OutFile $zipPath
Expand-Archive -LiteralPath $zipPath -DestinationPath $destRoot -Force

$candidates = @(
    (Join-Path $destRoot "wraptool.exe"),
    (Join-Path $destRoot "bin\wraptool.exe"),
    (Join-Path $destRoot "PACEAntiPiracy\Eden\Fusion\Current\bin\wraptool.exe"),
    (Join-Path $destRoot "PACEAntiPiracy\Eden\Fusion\Versions\5\bin\wraptool.exe")
)
$found = $null
foreach ($c in $candidates) {
    if (Test-Path -LiteralPath $c) {
        $found = (Resolve-Path $c).Path
        break
    }
}
if ($null -eq $found) {
    throw "wraptool.exe not found after extract. Expected one of: $($candidates -join ', ')"
}

Add-Content -LiteralPath $env:GITHUB_ENV -Value "WRAPTOOL=$found"
Write-Host "WRAPTOOL=$found appended to GITHUB_ENV"
