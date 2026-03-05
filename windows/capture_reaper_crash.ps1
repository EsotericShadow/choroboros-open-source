Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
    [string]$DumpDir = "$env:LOCALAPPDATA\CrashDumps",
    [int]$EventLookbackMinutes = 30,
    [switch]$KeepOldDumps
)

function Ensure-LocalDumpConfig {
    param(
        [Parameter(Mandatory = $true)][string]$ExeName,
        [Parameter(Mandatory = $true)][string]$TargetDumpDir
    )

    $base = "HKCU:\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps"
    if (-not (Test-Path $base)) {
        New-Item -Path $base -Force | Out-Null
    }

    $appKey = Join-Path $base $ExeName
    if (-not (Test-Path $appKey)) {
        New-Item -Path $appKey -Force | Out-Null
    }

    New-Item -ItemType Directory -Force -Path $TargetDumpDir | Out-Null
    New-ItemProperty -Path $appKey -Name "DumpFolder" -PropertyType ExpandString -Value $TargetDumpDir -Force | Out-Null
    New-ItemProperty -Path $appKey -Name "DumpType" -PropertyType DWord -Value 2 -Force | Out-Null
    New-ItemProperty -Path $appKey -Name "DumpCount" -PropertyType DWord -Value 10 -Force | Out-Null
}

function Find-ReaperExe {
    $candidates = @(
        "$env:ProgramFiles\REAPER (x64)\reaper.exe",
        "$env:ProgramFiles\REAPER\reaper.exe",
        "${env:ProgramFiles(x86)}\REAPER\reaper.exe"
    )

    foreach ($path in $candidates) {
        if ($path -and (Test-Path $path)) {
            return $path
        }
    }

    $cmd = Get-Command reaper.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    throw "Could not find reaper.exe in standard install paths."
}

function Stop-ReaperHosts {
    Get-Process reaper, reaper_host64, reaper_host32, reaper64 -ErrorAction SilentlyContinue | Stop-Process -Force
}

function Get-RelevantEvents {
    param(
        [Parameter(Mandatory = $true)][datetime]$StartTime
    )

    $ids = @(1000, 1001)
    $events = @()
    foreach ($id in $ids) {
        $events += Get-WinEvent -FilterHashtable @{
            LogName = "Application"
            Id = $id
            StartTime = $StartTime
        } -ErrorAction SilentlyContinue
    }

    return $events |
        Where-Object { $_.Message -match "(?i)reaper|choroboros" } |
        Sort-Object TimeCreated -Descending
}

Write-Host "Preparing crash capture for REAPER + Choroboros..." -ForegroundColor Cyan
Ensure-LocalDumpConfig -ExeName "reaper.exe" -TargetDumpDir $DumpDir
Ensure-LocalDumpConfig -ExeName "reaper_host64.exe" -TargetDumpDir $DumpDir
Ensure-LocalDumpConfig -ExeName "reaper_host32.exe" -TargetDumpDir $DumpDir

if (-not $KeepOldDumps) {
    Get-ChildItem -Path $DumpDir -Filter "*.dmp" -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
}

Stop-ReaperHosts
$reaperExe = Find-ReaperExe
Write-Host ("REAPER path: {0}" -f $reaperExe)
Write-Host ("Dump folder: {0}" -f $DumpDir)
Write-Host "Launching REAPER now. Reproduce crash by opening DevPanel > Modulation tab." -ForegroundColor Yellow

$startTime = Get-Date
$process = Start-Process -FilePath $reaperExe -PassThru
Wait-Process -Id $process.Id
Start-Sleep -Seconds 2

$dump = Get-ChildItem -Path $DumpDir -Filter "*.dmp" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

Write-Host ""
Write-Host "=== Crash Capture Summary ===" -ForegroundColor Cyan
if ($dump) {
    $hash = Get-FileHash -Algorithm SHA256 -Path $dump.FullName
    [pscustomobject]@{
        DumpPath = $dump.FullName
        DumpSizeBytes = $dump.Length
        DumpLastWrite = $dump.LastWriteTime
        DumpSHA256 = $hash.Hash
    } | Format-List
} else {
    Write-Warning "No dump file was created. If REAPER did not crash, this is expected."
}

$events = Get-RelevantEvents -StartTime $startTime
if ($events.Count -gt 0) {
    Write-Host ""
    Write-Host "Recent Application log events (ID 1000/1001):" -ForegroundColor Cyan
    $events |
        Select-Object TimeCreated, Id, ProviderName, Message |
        Format-List
} else {
    Write-Host "No relevant application crash events found in the lookback window."
}

Write-Host ""
Write-Host "Re-run command:" -ForegroundColor Cyan
Write-Host "  powershell -ExecutionPolicy Bypass -File .\windows\capture_reaper_crash.ps1"
