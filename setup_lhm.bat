@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
chcp 65001 >nul

echo ========================================
echo  LibreHardwareMonitor - Setup
echo ========================================
echo.

set "LHM_EXE="

REM Method 1: WinGet package folder (for /d supports trailing wildcard on directory)
for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\LibreHardwareMonitor*") do (
    if exist "%%D\LibreHardwareMonitor.exe" (
        set "LHM_EXE=%%D\LibreHardwareMonitor.exe"
    )
)

REM Method 2: PowerShell search (handles Unicode paths)
if not defined LHM_EXE (
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$p = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName; if ($p) { Write-Output $p }"`) do (
        set "LHM_EXE=%%F"
    )
)

REM Method 3: Install via winget if still missing
if not defined LHM_EXE (
    echo LibreHardwareMonitor not found. Installing via winget...
    winget install LibreHardwareMonitor.LibreHardwareMonitor --accept-package-agreements --accept-source-agreements
    for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\LibreHardwareMonitor*") do (
        if exist "%%D\LibreHardwareMonitor.exe" set "LHM_EXE=%%D\LibreHardwareMonitor.exe"
    )
    if not defined LHM_EXE (
        for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$p = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName; if ($p) { Write-Output $p }"`) do (
            set "LHM_EXE=%%F"
        )
    )
)

if not defined LHM_EXE (
    echo.
    echo ERROR: Could not find LibreHardwareMonitor.exe
    echo It may already be installed via winget. Try running manually:
    echo   %%LOCALAPPDATA%%\Microsoft\WinGet\Packages\LibreHardwareMonitor*\LibreHardwareMonitor.exe
    echo.
    pause
    exit /b 1
)

echo Found: !LHM_EXE!

REM Enable HTTP web server on port 8085 (required for CPU temp)
powershell -NoProfile -Command "$exe='!LHM_EXE!'; $dir=Split-Path -Path $exe -Parent; $cfg=Join-Path $dir 'LibreHardwareMonitor.config'; if (-not (Test-Path $cfg)) { Write-Host 'WARN: config not found'; exit 0 }; [xml]$xml=Get-Content $cfg -Encoding UTF8; $apps=$xml.configuration.appSettings; $key=$apps.add | Where-Object { $_.key -eq 'runWebServerMenuItem' }; if (-not $key) { $n=$xml.CreateElement('add'); $n.SetAttribute('key','runWebServerMenuItem'); $n.SetAttribute('value','true'); [void]$apps.AppendChild($n) } else { $key.value='true' }; $xml.Save($cfg); Write-Host 'Enabled Remote Web Server (port 8085)'"

set TASK=LibreHardwareMonitor
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 schtasks /Delete /TN "%TASK%" /F >nul

echo Creating logon task (run in background)...
schtasks /Create /TN "%TASK%" /TR "\"!LHM_EXE!\"" /SC ONLOGON /RL HIGHEST /F
if errorlevel 1 (
    echo WARN: Could not create scheduled task. Right-click this bat - Run as administrator.
) else (
    echo Scheduled task created.
)

echo Stopping old instance if any...
taskkill /IM LibreHardwareMonitor.exe /F >nul 2>&1
ping 127.0.0.1 -n 3 >nul

echo Starting LibreHardwareMonitor...
start "" "!LHM_EXE!"
ping 127.0.0.1 -n 6 >nul

powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8085/data.json' -UseBasicParsing -TimeoutSec 5; Write-Host ('HTTP OK - data.json size ' + $r.Content.Length + ' bytes') } catch { Write-Host 'WARN: data.json not reachable. Open LHM - Options - Remote Web Server - Run - then restart LHM.' }"

echo.
echo Done. Keep LibreHardwareMonitor running for CPU temperature on Cardputer.
echo Verify in browser: http://127.0.0.1:8085/data.json
echo.
pause
