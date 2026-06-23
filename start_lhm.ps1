# Start LibreHardwareMonitor with a valid Remote Web Server config.
param([switch]$Quiet)

$ErrorActionPreference = 'Stop'

function Write-Info([string]$Message) {
    if (-not $Quiet) { Write-Host $Message }
}

function Find-LhmExe {
    $base = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'
    if (Test-Path -LiteralPath $base) {
        $hit = Get-ChildItem -LiteralPath $base -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($hit) { return $hit }
    }
    return $null
}

function Set-LhmConfig {
    param([string]$ExePath)
    $dir = Split-Path -Path $ExePath -Parent
    $cfg = Join-Path $dir 'LibreHardwareMonitor.config'
    if (-not (Test-Path -LiteralPath $cfg)) {
        Write-Info "WARN: config not found: $cfg"
        return
    }
    [xml]$xml = Get-Content -LiteralPath $cfg -Encoding UTF8
    $apps = $xml.configuration.appSettings
    function Set-Key([string]$Key, [string]$Value) {
        $item = $apps.add | Where-Object { $_.key -eq $Key }
        if (-not $item) {
            $item = $xml.CreateElement('add')
            $item.SetAttribute('key', $Key)
            [void]$apps.AppendChild($item)
        }
        $item.value = $Value
    }
    Set-Key 'listenerIp' '0.0.0.0'
    Set-Key 'listenerPort' '8090'
    Set-Key 'runWebServerMenuItem' 'true'
    Set-Key 'authenticationEnabled' 'false'
    $xml.Save($cfg)
}

function Test-LhmHttp {
    foreach ($port in 8090, 8085) {
        try {
            $r = Invoke-WebRequest -Uri "http://127.0.0.1:$port/data.json" -UseBasicParsing -TimeoutSec 3
            if ($r.StatusCode -ge 200 -and $r.StatusCode -lt 300) {
                return $true
            }
        } catch {}
    }
    return $false
}

$exe = Find-LhmExe
if (-not $exe) {
    Write-Info 'LibreHardwareMonitor.exe not found'
    exit 1
}

Set-LhmConfig -ExePath $exe.FullName

if (Test-LhmHttp) {
    Write-Info "LHM HTTP already OK ($($exe.FullName))"
    exit 0
}

$proc = Get-Process -Name 'LibreHardwareMonitor' -ErrorAction SilentlyContinue
if ($proc) {
    Write-Info 'LHM running but HTTP down — restarting...'
    Stop-Process -Name 'LibreHardwareMonitor' -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
}

Write-Info "Starting LHM: $($exe.FullName)"
Start-Process -FilePath $exe.FullName -WorkingDirectory $exe.DirectoryName

foreach ($wait in 1..12) {
    Start-Sleep -Seconds 1
    if (Test-LhmHttp) {
        Write-Info 'LHM HTTP OK'
        exit 0
    }
}

Write-Info 'WARN: LHM started but HTTP not ready yet'
exit 2
