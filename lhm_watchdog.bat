@echo off
setlocal EnableDelayedExpansion

set "LHM_EXE="
for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\LibreHardwareMonitor*") do (
    if exist "%%D\LibreHardwareMonitor.exe" set "LHM_EXE=%%D\LibreHardwareMonitor.exe"
)

if not defined LHM_EXE (
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$p = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName; if ($p) { Write-Output $p }"`) do (
        set "LHM_EXE=%%F"
    )
)

if not defined LHM_EXE exit /b 0

tasklist /FI "IMAGENAME eq LibreHardwareMonitor.exe" 2>nul | find /I "LibreHardwareMonitor.exe" >nul
if not errorlevel 1 exit /b 0

start "" "!LHM_EXE!"
exit /b 0
