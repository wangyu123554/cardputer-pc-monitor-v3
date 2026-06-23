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

set "START_LHM=%~dp0start_lhm.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "%START_LHM%"

set TASK=LibreHardwareMonitor
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 schtasks /Delete /TN "%TASK%" /F >nul

echo Creating logon task (hidden via wscript, once per login)...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$vbs='%~dp0run_lhm_hidden.vbs';" ^
  "$name='LibreHardwareMonitor';" ^
  "Unregister-ScheduledTask -TaskName $name -Confirm:$false -ErrorAction SilentlyContinue;" ^
  "$action=New-ScheduledTaskAction -Execute 'wscript.exe' -Argument ('//B \"{0}\"' -f $vbs);" ^
  "$trigger=New-ScheduledTaskTrigger -AtLogOn;" ^
  "$trigger.Delay='PT45S';" ^
  "$settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -Hidden -ExecutionTimeLimit (New-TimeSpan -Minutes 2);" ^
  "Register-ScheduledTask -TaskName $name -Action $action -Trigger $trigger -Settings $settings -Description 'Start LibreHardwareMonitor with HTTP web server' -Force | Out-Null;" ^
  "Write-Host 'Scheduled task created.'"
if errorlevel 1 (
    echo WARN: Could not create scheduled task. Right-click this bat - Run as administrator.
) else (
    echo Scheduled task created.
)

echo.
powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8090/data.json' -UseBasicParsing -TimeoutSec 5; Write-Host ('HTTP OK - data.json size ' + $r.Content.Length + ' bytes') } catch { try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8085/data.json' -UseBasicParsing -TimeoutSec 5; Write-Host ('HTTP OK port 8085 - ' + $r.Content.Length + ' bytes') } catch { Write-Host 'WARN: data.json not reachable. Run fix_lhm_webserver.bat as Administrator.' } }"

echo.
echo Done. Keep LibreHardwareMonitor running for CPU temperature on Cardputer.
echo Verify in browser: http://127.0.0.1:8085/data.json
echo.
pause
