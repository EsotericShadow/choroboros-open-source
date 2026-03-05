Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Test-CommandExists {
    param([Parameter(Mandatory = $true)][string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $cmd) {
        return $false
    }
    return $true
}

function Write-Check {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][bool]$Ok,
        [string]$Detail = ""
    )
    if ($Ok) {
        Write-Host ("[OK]   {0} {1}" -f $Label, $Detail) -ForegroundColor Green
    } else {
        Write-Host ("[WARN] {0} {1}" -f $Label, $Detail) -ForegroundColor Yellow
    }
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

Write-Host "Windows preflight for Choroboros" -ForegroundColor Cyan
Write-Host "Repo root: $repoRoot"
Write-Host ""

$hasCmake = Test-CommandExists -Name "cmake"
$hasGit = Test-CommandExists -Name "git"
$hasCl = Test-CommandExists -Name "cl"
$hasMsbuild = Test-CommandExists -Name "msbuild"

Write-Check -Label "cmake in PATH" -Ok $hasCmake
Write-Check -Label "git in PATH" -Ok $hasGit
Write-Check -Label "MSVC cl in PATH" -Ok $hasCl -Detail "(Developer PowerShell recommended)"
Write-Check -Label "msbuild in PATH" -Ok $hasMsbuild

$cmakePresets = Join-Path $repoRoot "CMakePresets.json"
Write-Check -Label "CMakePresets.json present" -Ok (Test-Path $cmakePresets)

$juceLocal = Join-Path $repoRoot "JUCE\CMakeLists.txt"
$hasLocalJuce = Test-Path $juceLocal
Write-Check -Label "Local JUCE tree present" -Ok $hasLocalJuce

$internetLikely = $false
try {
    $result = Test-NetConnection github.com -Port 443 -WarningAction SilentlyContinue
    if ($result.TcpTestSucceeded) {
        $internetLikely = $true
    }
} catch {
    $internetLikely = $false
}
Write-Check -Label "GitHub HTTPS reachable" -Ok $internetLikely -Detail "(needed if JUCE fetch is required)"

if ((-not $hasLocalJuce) -and (-not $internetLikely)) {
    Write-Host ""
    Write-Host "Blocking risk: no local JUCE and no GitHub connectivity. Configure may fail." -ForegroundColor Red
}

$pathLength = $repoRoot.Length
$pathLengthOk = $pathLength -lt 120
Write-Check -Label "Repo path length" -Ok $pathLengthOk -Detail "($pathLength chars; shorter is safer on Windows)"

$longPathsEnabled = $false
try {
    $reg = Get-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -ErrorAction Stop
    $longPathsEnabled = ($reg.LongPathsEnabled -eq 1)
} catch {
    $longPathsEnabled = $false
}
Write-Check -Label "Windows long paths enabled" -Ok $longPathsEnabled

Write-Host ""
Write-Host "Suggested next command:" -ForegroundColor Cyan
Write-Host "  powershell -ExecutionPolicy Bypass -File .\windows\build_windows_x64.ps1 -Config Release"
