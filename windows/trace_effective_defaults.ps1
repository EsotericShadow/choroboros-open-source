param(
    [string]$RepoRoot = "",
    [string]$SourceJson = ""
)

$ErrorActionPreference = "Stop"

function Resolve-PathSafe {
    param([string]$PathValue)
    if ([string]::IsNullOrWhiteSpace($PathValue)) { return $null }
    if (Test-Path $PathValue) { return (Resolve-Path $PathValue).Path }
    return $PathValue
}

function Read-JsonObject {
    param([string]$PathValue)
    if (-not (Test-Path $PathValue)) { return $null }
    $raw = Get-Content -Raw -Path $PathValue
    if ([string]::IsNullOrWhiteSpace($raw)) { return $null }
    try {
        return ($raw | ConvertFrom-Json)
    } catch {
        return $null
    }
}

function Get-TopLevelKeys {
    param([object]$JsonObject)
    if ($null -eq $JsonObject) { return @() }
    if ($JsonObject -is [System.Collections.IDictionary]) {
        return @($JsonObject.Keys | Sort-Object)
    }
    return @($JsonObject.PSObject.Properties.Name | Sort-Object)
}

function Get-JsonInfo {
    param([string]$PathValue)

    $exists = Test-Path $PathValue
    $size = 0
    $sha = ""
    $valid = $false
    $keys = @()
    $obj = $null

    if ($exists) {
        $item = Get-Item $PathValue
        $size = [int64]$item.Length
        $sha = (Get-FileHash -Algorithm SHA256 -Path $PathValue).Hash
        $obj = Read-JsonObject -PathValue $PathValue
        $valid = ($null -ne $obj)
        if ($valid) {
            $keys = Get-TopLevelKeys -JsonObject $obj
        }
    }

    return [pscustomobject]@{
        Path = $PathValue
        Exists = $exists
        Bytes = $size
        Sha256 = $sha
        ValidJson = $valid
        TopLevelKeys = ($keys -join ",")
        Object = $obj
    }
}

function Flatten-JsonNode {
    param(
        [object]$Node,
        [string]$Prefix,
        [hashtable]$OutMap
    )

    if ($null -eq $Node) {
        $OutMap[$Prefix] = "<null>"
        return
    }

    if ($Node -is [System.Collections.IDictionary]) {
        foreach ($key in ($Node.Keys | Sort-Object)) {
            $next = if ([string]::IsNullOrEmpty($Prefix)) { [string]$key } else { "$Prefix.$key" }
            Flatten-JsonNode -Node $Node[$key] -Prefix $next -OutMap $OutMap
        }
        return
    }

    if ($Node -is [System.Collections.IList] -and -not ($Node -is [string])) {
        for ($i = 0; $i -lt $Node.Count; $i++) {
            $next = if ([string]::IsNullOrEmpty($Prefix)) { "[$i]" } else { "$Prefix[$i]" }
            Flatten-JsonNode -Node $Node[$i] -Prefix $next -OutMap $OutMap
        }
        return
    }

    if ($Node -is [pscustomobject]) {
        foreach ($prop in ($Node.PSObject.Properties | Sort-Object Name)) {
            $next = if ([string]::IsNullOrEmpty($Prefix)) { [string]$prop.Name } else { "$Prefix.$($prop.Name)" }
            Flatten-JsonNode -Node $prop.Value -Prefix $next -OutMap $OutMap
        }
        return
    }

    $OutMap[$Prefix] = [string]$Node
}

function Convert-JsonObjectToFlatMap {
    param([object]$JsonObject)
    $map = @{}
    if ($null -ne $JsonObject) {
        Flatten-JsonNode -Node $JsonObject -Prefix "" -OutMap $map
    }
    return $map
}

function Compare-FlatMaps {
    param(
        [hashtable]$LeftMap,
        [hashtable]$RightMap,
        [string]$LeftName,
        [string]$RightName
    )

    $rows = New-Object System.Collections.Generic.List[object]
    $allKeys = @($LeftMap.Keys + $RightMap.Keys | Sort-Object -Unique)
    foreach ($key in $allKeys) {
        $leftExists = $LeftMap.ContainsKey($key)
        $rightExists = $RightMap.ContainsKey($key)
        $leftVal = if ($leftExists) { $LeftMap[$key] } else { "<missing>" }
        $rightVal = if ($rightExists) { $RightMap[$key] } else { "<missing>" }
        if ($leftVal -ne $rightVal) {
            $rows.Add([pscustomobject]@{
                Key = $key
                $LeftName = $leftVal
                $RightName = $rightVal
            })
        }
    }
    return $rows
}

function Get-JsonPathValue {
    param(
        [object]$JsonObject,
        [string]$DotPath
    )

    if ($null -eq $JsonObject -or [string]::IsNullOrWhiteSpace($DotPath)) {
        return "<missing>"
    }

    $current = $JsonObject
    foreach ($segment in $DotPath.Split(".")) {
        if ($null -eq $current) { return "<missing>" }
        if ($current -is [System.Collections.IDictionary]) {
            if (-not $current.Contains($segment)) { return "<missing>" }
            $current = $current[$segment]
            continue
        }
        $prop = $current.PSObject.Properties[$segment]
        if ($null -eq $prop) { return "<missing>" }
        $current = $prop.Value
    }

    if ($null -eq $current) { return "<null>" }
    return [string]$current
}

function Get-FileMatchCount {
    param(
        [string]$PathValue,
        [string]$Pattern
    )

    if (-not (Test-Path $PathValue)) { return 0 }
    try {
        $matches = Select-String -Path $PathValue -Pattern $Pattern -SimpleMatch -ErrorAction Stop
        if ($null -eq $matches) { return 0 }
        return @($matches).Count
    } catch {
        return 0
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $candidate = Resolve-PathSafe -PathValue (Join-Path $PSScriptRoot "..")
    if ($null -ne $candidate) {
        $RepoRoot = $candidate
    } else {
        $RepoRoot = (Get-Location).Path
    }
}

$RepoRoot = Resolve-PathSafe -PathValue $RepoRoot
if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "Could not resolve repo root."
}

if ([string]::IsNullOrWhiteSpace($SourceJson)) {
    $SourceJson = Join-Path $RepoRoot "windows\windows_factory_defaults.json"
}
$SourceJson = Resolve-PathSafe -PathValue $SourceJson

$roamingCfg = Join-Path ([Environment]::GetFolderPath("ApplicationData")) "Choroboros"
$localCfg = Join-Path ([Environment]::GetFolderPath("LocalApplicationData")) "Choroboros"
$reaperCfg = Join-Path ([Environment]::GetFolderPath("ApplicationData")) "REAPER"

$paths = [ordered]@{
    EffectiveUser = Join-Path $roamingCfg "defaults_user.json"
    Factory = Join-Path $roamingCfg "defaults_factory.json"
    Legacy = Join-Path $roamingCfg "defaults.json"
    LocalShadowUser = Join-Path $localCfg "defaults_user.json"
    RepoSource = $SourceJson
}

$info = @{}
foreach ($name in $paths.Keys) {
    $info[$name] = Get-JsonInfo -PathValue $paths[$name]
}

$effectivePath = ""
$effectiveReason = ""
$effectiveObject = $null

if ($info.EffectiveUser.Exists) {
    $effectivePath = $info.EffectiveUser.Path
    $effectiveReason = "DefaultsPersistence::load() -> loadUser() found defaults_user.json"
    $effectiveObject = $info.EffectiveUser.Object
} elseif ($info.Legacy.Exists -and $info.Legacy.ValidJson) {
    $effectivePath = $info.Legacy.Path
    $effectiveReason = "DefaultsPersistence::loadUser() fallback: valid legacy defaults.json (migrates to defaults_user.json)"
    $effectiveObject = $info.Legacy.Object
} else {
    $effectiveReason = "No readable defaults_user/legacy file. Runtime keeps in-memory defaults this launch; on Windows, constructor seeds files from bundled resource only when missing/empty."
}

Write-Host ""
Write-Host "=== Choroboros Defaults Trace (Runtime-Equivalent) ===" -ForegroundColor Cyan
Write-Host "RepoRoot: $RepoRoot"
Write-Host ""
Write-Host "Code path (verified):"
Write-Host "  - Processor tuning/internals: DefaultsPersistence::load() in PluginProcessor loadPersistedDefaults()"
Write-Host "  - Editor layout/text animations: DefaultsPersistence::load() in PluginEditor loadPersistedLayoutDefaults()"
Write-Host "  - Host project state (APVTS chunk) can override parameter values for existing instances."
Write-Host ""

$table = foreach ($name in $paths.Keys) {
    $row = $info[$name]
    [pscustomobject]@{
        Name = $name
        Exists = $row.Exists
        ValidJson = $row.ValidJson
        Bytes = $row.Bytes
        Sha256 = if ($row.Exists) { $row.Sha256.Substring(0, 16) } else { "" }
        Path = $row.Path
    }
}
$table | Format-Table -AutoSize

Write-Host ""
if ([string]::IsNullOrWhiteSpace($effectivePath)) {
    Write-Host "Effective defaults source: <none>" -ForegroundColor Yellow
} else {
    Write-Host "Effective defaults source: $effectivePath" -ForegroundColor Green
}
Write-Host "Reason: $effectiveReason"

$probeKeys = @(
    "tuning.rate.min",
    "tuning.rate.max",
    "coreAssignments.green.nq",
    "coreAssignments.green.hq",
    "layout.mainKnobSizeGreen",
    "layout.mixKnobSizeGreen",
    "layout.mainValueFlipDurationMs",
    "layout.mixValueFlipDurationMs",
    "layout.valueFxEnabled"
)

if ($null -ne $effectiveObject) {
    Write-Host ""
    Write-Host "Probe values from EFFECTIVE JSON:"
    foreach ($k in $probeKeys) {
        $v = Get-JsonPathValue -JsonObject $effectiveObject -DotPath $k
        Write-Host ("  {0} = {1}" -f $k, $v)
    }
}

if ($null -ne $effectiveObject -and $info.RepoSource.ValidJson) {
    $effectiveMap = Convert-JsonObjectToFlatMap -JsonObject $effectiveObject
    $sourceMap = Convert-JsonObjectToFlatMap -JsonObject $info.RepoSource.Object
    $diff = Compare-FlatMaps -LeftMap $effectiveMap -RightMap $sourceMap -LeftName "Effective" -RightName "RepoSource"

    Write-Host ""
    Write-Host ("Diff vs RepoSource ({0}): {1} changed keys" -f $info.RepoSource.Path, $diff.Count)
    if ($diff.Count -gt 0) {
        $diff | Select-Object -First 80 | Format-Table -AutoSize
    }
}

if ($null -ne $effectiveObject -and $info.Factory.ValidJson) {
    $effectiveMap = Convert-JsonObjectToFlatMap -JsonObject $effectiveObject
    $factoryMap = Convert-JsonObjectToFlatMap -JsonObject $info.Factory.Object
    $diffFactory = Compare-FlatMaps -LeftMap $effectiveMap -RightMap $factoryMap -LeftName "Effective" -RightName "Factory"

    Write-Host ""
    Write-Host ("Diff vs Factory ({0}): {1} changed keys" -f $info.Factory.Path, $diffFactory.Count)
    if ($diffFactory.Count -gt 0) {
        $diffFactory | Select-Object -First 80 | Format-Table -AutoSize
    }
}

$reaperFiles = [ordered]@{
    VstPresets64 = Join-Path $reaperCfg "reaper-vstpresets64.ini"
    VstPresets32 = Join-Path $reaperCfg "reaper-vstpresets.ini"
    VstPlugins64 = Join-Path $reaperCfg "reaper-vstplugins64.ini"
    ReaperIni = Join-Path $reaperCfg "reaper.ini"
}

$reaperTable = foreach ($name in $reaperFiles.Keys) {
    $pathValue = $reaperFiles[$name]
    $exists = Test-Path $pathValue
    [pscustomobject]@{
        Name = $name
        Exists = $exists
        ChoroborosMentions = if ($exists) { Get-FileMatchCount -PathValue $pathValue -Pattern "Choroboros" } else { 0 }
        Path = $pathValue
    }
}

Write-Host ""
Write-Host "REAPER host-state override scan (new inserts can inherit host default preset/state):"
$reaperTable | Format-Table -AutoSize

$presetMentions = ($reaperTable | Where-Object { $_.Name -like "VstPresets*" } | Measure-Object -Property ChoroborosMentions -Sum).Sum
if ($presetMentions -gt 0) {
    Write-Host "WARNING: REAPER preset files contain Choroboros entries. Host default preset/state may override JSON defaults on insert." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "One-line rerun:"
Write-Host "  powershell -ExecutionPolicy Bypass -File .\windows\trace_effective_defaults.ps1"
Write-Host ""
