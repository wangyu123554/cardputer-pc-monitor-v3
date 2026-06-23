# Silent LHM watchdog — delegates to start_lhm.ps1 without opening a console.
$ErrorActionPreference = 'SilentlyContinue'
& (Join-Path $PSScriptRoot 'start_lhm.ps1') -Quiet
exit $LASTEXITCODE
