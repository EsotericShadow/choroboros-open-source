param(
    [string]$LogPath = "$env:APPDATA\Choroboros\load_trace.ndjson",
    [int]$Tail = 20,
    [switch]$Reset
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Reset) {
    if (Test-Path $LogPath) {
        Remove-Item $LogPath -Force
        Write-Host "Cleared load trace log: $LogPath" -ForegroundColor Yellow
    } else {
        Write-Host "No load trace log to clear at: $LogPath"
    }
    return
}

if (-not (Test-Path $LogPath)) {
    Write-Host "Load trace log not found: $LogPath" -ForegroundColor Red
    Write-Host "Open the plugin at least once, then re-run this script."
    return
}

function Get-Percentile {
    param(
        [Parameter(Mandatory = $true)][object]$Values,
        [Parameter(Mandatory = $true)][double]$P
    )
    $valueArray = @($Values | ForEach-Object { [double]$_ })
    if ($valueArray.Count -eq 0) { return [double]::NaN }
    $sorted = @($valueArray | Sort-Object)
    $idx = [math]::Round(($sorted.Count - 1) * $P)
    return [double]$sorted[[int]$idx]
}

function Get-EventMetric {
    param(
        [Parameter(Mandatory = $true)][object[]]$InstanceEvents,
        [Parameter(Mandatory = $true)][string]$EventName
    )

    $match = $InstanceEvents | Where-Object { $_.event -eq $EventName -and $_.elapsedMs -ne $null } | Select-Object -First 1
    if ($null -eq $match) { return $null }
    return [double]$match.elapsedMs
}

$invalidBlocks = 0
$events = @()
$currentBlock = New-Object System.Text.StringBuilder
$braceDepth = 0
$inString = $false
$escaped = $false

Get-Content -Path $LogPath -Encoding UTF8 | ForEach-Object {
    $line = $_
    if ($braceDepth -eq 0 -and [string]::IsNullOrWhiteSpace($line)) {
        return
    }

    [void]$currentBlock.AppendLine($line)

    foreach ($ch in $line.ToCharArray()) {
        if ($escaped) {
            $escaped = $false
            continue
        }

        if ($ch -eq '\') {
            if ($inString) {
                $escaped = $true
            }
            continue
        }

        if ($ch -eq '"') {
            $inString = -not $inString
            continue
        }

        if ($inString) {
            continue
        }

        if ($ch -eq '{') {
            $braceDepth++
        } elseif ($ch -eq '}') {
            $braceDepth--
        }
    }

    if ($braceDepth -eq 0 -and $currentBlock.Length -gt 0) {
        $jsonBlock = $currentBlock.ToString().Trim()
        if (-not [string]::IsNullOrWhiteSpace($jsonBlock)) {
            try {
                $events += ($jsonBlock | ConvertFrom-Json)
            } catch {
                $invalidBlocks++
            }
        }
        [void]$currentBlock.Clear()
        $inString = $false
        $escaped = $false
    }
}

if ($currentBlock.Length -gt 0) {
    $invalidBlocks++
}

Write-Host ""
Write-Host "=== Choroboros Load Performance Trace ===" -ForegroundColor Cyan
Write-Host ("LogPath: {0}" -f $LogPath)
Write-Host ("Events: {0} | Invalid blocks: {1}" -f $events.Count, $invalidBlocks)

if ($events.Count -eq 0) {
    Write-Host "No parsable events found." -ForegroundColor Yellow
    return
}

$instanceCount = ($events | Select-Object -ExpandProperty instanceId -Unique | Measure-Object).Count
$hostCount = ($events | Select-Object -ExpandProperty host -Unique | Measure-Object).Count
Write-Host ("Instances: {0} | Hosts: {1}" -f $instanceCount, $hostCount)

$latest = $events | Sort-Object tsUtc -Descending | Select-Object -First 1
Write-Host ""
Write-Host "System / Host Snapshot (latest event):" -ForegroundColor Cyan
[pscustomobject]@{
    TimeUtc = $latest.tsUtc
    Host = $latest.host
    HostPath = $latest.hostPath
    WrapperType = $latest.wrapperType
    OS = $latest.os
    IsOS64Bit = $latest.isOS64Bit
    CpuVendor = $latest.cpuVendor
    CpuModel = $latest.cpuModel
    CpuSpeedMHz = $latest.cpuSpeedMHz
    CpuCores = $latest.cpuCores
    RamMB = $latest.ramMB
    PluginVersion = $latest.pluginVersion
    BuildConfig = $latest.buildConfig
} | Format-List

$interesting = @(
    "processor_ctor_total_ms",
    "processor_load_persisted_defaults_ms",
    "processor_create_editor_ms",
    "editor_ctor_total_ms",
    "editor_theme_setup_ms",
    "editor_active_theme_ready_ms",
    "editor_controls_setup_ms",
    "editor_first_paint_ms",
    "devpanel_tab_switch_ms",
    "devpanel_tab_build_ms"
)

$rows = @()
foreach ($eventName in $interesting) {
    $samples = @($events | Where-Object { $_.event -eq $eventName -and $_.elapsedMs -ne $null } | ForEach-Object { [double]$_.elapsedMs })
    if ($samples.Count -eq 0) { continue }
    $rows += [pscustomobject]@{
        Event = $eventName
        Count = $samples.Count
        MinMs = [math]::Round(($samples | Measure-Object -Minimum).Minimum, 3)
        P50Ms = [math]::Round((Get-Percentile -Values $samples -P 0.50), 3)
        P95Ms = [math]::Round((Get-Percentile -Values $samples -P 0.95), 3)
        MaxMs = [math]::Round(($samples | Measure-Object -Maximum).Maximum, 3)
        AvgMs = [math]::Round(($samples | Measure-Object -Average).Average, 3)
    }
}

Write-Host ""
Write-Host "Timing Summary (ms):" -ForegroundColor Cyan
if ($rows.Count -gt 0) {
    $rows | Sort-Object Event | Format-Table -AutoSize
} else {
    Write-Host "No matching timing events yet."
}

$instanceGroups = @($events | Group-Object instanceId | Sort-Object {[int64]$_.Name})
if ($instanceGroups.Count -gt 0) {
    $instanceRows = @()
    foreach ($group in $instanceGroups) {
        $instanceEvents = @($group.Group | Sort-Object { [datetimeoffset]::Parse($_.tsUtc) })
        if ($instanceEvents.Count -eq 0) { continue }

        $processorCreateMs = Get-EventMetric -InstanceEvents $instanceEvents -EventName "processor_create_editor_ms"
        $editorCtorMs = Get-EventMetric -InstanceEvents $instanceEvents -EventName "editor_ctor_total_ms"
        $editorFirstPaintMs = Get-EventMetric -InstanceEvents $instanceEvents -EventName "editor_first_paint_ms"
        $activeThemeReadyMs = Get-EventMetric -InstanceEvents $instanceEvents -EventName "editor_active_theme_ready_ms"

        $gapCreateToFirstPaintMs = $null
        $gapCtorToFirstPaintMs = $null
        $gapFirstPaintToThemeReadyMs = $null
        if ($null -ne $processorCreateMs -and $null -ne $editorFirstPaintMs) {
            $gapCreateToFirstPaintMs = [math]::Round(($editorFirstPaintMs - $processorCreateMs), 3)
        }
        if ($null -ne $editorCtorMs -and $null -ne $editorFirstPaintMs) {
            $gapCtorToFirstPaintMs = [math]::Round(($editorFirstPaintMs - $editorCtorMs), 3)
        }
        if ($null -ne $activeThemeReadyMs -and $null -ne $editorFirstPaintMs) {
            $gapFirstPaintToThemeReadyMs = [math]::Round(($activeThemeReadyMs - $editorFirstPaintMs), 3)
        }

        $instanceRows += [pscustomobject]@{
            InstanceId = [int64]$group.Name
            Host = $instanceEvents[0].host
            Wrapper = $instanceEvents[0].wrapperType
            ProcessorCreateEditorMs = $processorCreateMs
            EditorCtorMs = $editorCtorMs
            EditorFirstPaintMs = $editorFirstPaintMs
            ActiveThemeReadyMs = $activeThemeReadyMs
            GapCreateToFirstPaintMs = $gapCreateToFirstPaintMs
            GapCtorToFirstPaintMs = $gapCtorToFirstPaintMs
            GapFirstPaintToThemeReadyMs = $gapFirstPaintToThemeReadyMs
        }
    }

    Write-Host ""
    Write-Host "Per-instance startup timeline (ms):" -ForegroundColor Cyan
    $instanceRows | Format-Table -AutoSize
}

Write-Host ""
Write-Host "Slowest events captured:" -ForegroundColor Cyan
$events |
    Where-Object { $_.elapsedMs -ne $null } |
    Sort-Object {[double]$_.elapsedMs} -Descending |
    Select-Object -First 15 tsUtc, event, elapsedMs, host, instanceId, notes |
    Format-Table -AutoSize

Write-Host ""
Write-Host ("Recent {0} raw events:" -f $Tail) -ForegroundColor Cyan
Get-Content -Path $LogPath -Tail $Tail

Write-Host ""
Write-Host "Tip: clear log before a controlled test run with:" -ForegroundColor DarkCyan
Write-Host "  powershell -ExecutionPolicy Bypass -File .\windows\trace_load_performance.ps1 -Reset"
