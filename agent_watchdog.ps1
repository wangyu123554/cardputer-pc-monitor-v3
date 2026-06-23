# Silent Agent watchdog — no console window when run with -WindowStyle Hidden.
$ErrorActionPreference = 'SilentlyContinue'
$dir = $PSScriptRoot

function Test-AgentHealth {
    try {
        $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8765/health' -UseBasicParsing -TimeoutSec 4
        return ($r.StatusCode -ge 200 -and $r.StatusCode -lt 300)
    } catch {
        return $false
    }
}

function Stop-AgentListeners {
    $pids = @()
    try {
        Get-NetTCPConnection -LocalPort 8765 -State Listen -ErrorAction Stop |
            ForEach-Object { $pids += $_.OwningProcess }
    } catch {
        netstat -ano | Select-String ':8765\s+.*LISTENING' | ForEach-Object {
            $parts = ($_ -replace '\s+', ' ').Trim().Split(' ')
            if ($parts.Length -ge 5) { $pids += [int]$parts[-1] }
        }
    }
    foreach ($procId in ($pids | Select-Object -Unique)) {
        if ($procId -gt 0) {
            Stop-Process -Id $procId -Force -ErrorAction SilentlyContinue
        }
    }
}

function Start-AgentSilent {
    $exe = Join-Path $dir 'PCMonitorAgent.exe'
    if (Test-Path -LiteralPath $exe) {
        Start-Process -FilePath $exe -WorkingDirectory $dir -WindowStyle Hidden
        return
    }
    $py = (Get-Command pythonw -ErrorAction SilentlyContinue).Source
    if (-not $py) {
        $py = (Get-Command python -ErrorAction SilentlyContinue).Source
    }
    if (-not $py) { return }
    $script = Join-Path $dir 'pc_monitor_agent.py'
    if (-not (Test-Path -LiteralPath $script)) { return }
    Start-Process -FilePath $py -ArgumentList "`"$script`"" -WorkingDirectory $dir -WindowStyle Hidden
}

if (Test-AgentHealth) { exit 0 }

Stop-AgentListeners
Start-Sleep -Seconds 2
Start-AgentSilent
exit 0
