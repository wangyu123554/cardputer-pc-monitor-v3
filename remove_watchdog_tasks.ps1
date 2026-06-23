# Remove Cardputer periodic watchdog tasks. Agent still monitors LHM in-process (no extra windows).
$ErrorActionPreference = 'SilentlyContinue'

$names = @(
    'Cardputer Agent Watchdog',
    'Cardputer LHM Watchdog'
)

foreach ($name in $names) {
    $task = Get-ScheduledTask -TaskName $name -ErrorAction SilentlyContinue
    if ($task) {
        Unregister-ScheduledTask -TaskName $name -Confirm:$false
        Write-Host "Removed: $name"
    } else {
        Write-Host "Not found: $name"
    }
}

Write-Host ''
Write-Host 'Done. Periodic watchdog disabled — no more scheduled console popups.'
Write-Host 'Agent (pythonw) still keeps LHM healthy in the background.'
