@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
chcp 65001 >nul

set "SILENT="
if /i "%~1"=="/silent" set "SILENT=1"
if /i "%~1"=="/nopause" set "SILENT=1"

echo ========================================
echo  LibreHardwareMonitor - Autostart Setup
echo ========================================
echo.

set "LHM_EXE="

for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\LibreHardwareMonitor*") do (
    if exist "%%D\LibreHardwareMonitor.exe" set "LHM_EXE=%%D\LibreHardwareMonitor.exe"
)

if not defined LHM_EXE (
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$p = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName; if ($p) { Write-Output $p }"`) do (
        set "LHM_EXE=%%F"
    )
)

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
    echo WARN: LibreHardwareMonitor not installed. CPU temp will show N/A on Cardputer.
    if not defined SILENT pause
    exit /b 0
)

echo Found: !LHM_EXE!

set "START_LHM=%~dp0start_lhm.ps1"

powershell -NoProfile -ExecutionPolicy Bypass -File "%START_LHM%"
if errorlevel 1 (
    echo WARN: Could not start LHM now.
)

set TASK=LibreHardwareMonitor
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 schtasks /Delete /TN "%TASK%" /F >nul

echo Creating logon task for LHM (hidden via wscript, once per login)...
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
  "Write-Host 'LHM logon task created'"
if errorlevel 1 (
    echo WARN: Could not create LHM task. Run this bat as Administrator.
) else (
    echo LHM scheduled task created.
)

echo Removing periodic LHM watchdog (flashes console — Agent handles LHM in-process)...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0remove_watchdog_tasks.ps1" >nul 2>&1

powershell -NoProfile -ExecutionPolicy Bypass -File "%START_LHM%" -Quiet >nul 2>&1

powershell -NoProfile -Command ^
  "$ok=$false; foreach($port in 8090,8085){ try { $r=Invoke-WebRequest -Uri ('http://127.0.0.1:'+$port+'/data.json') -UseBasicParsing -TimeoutSec 4; Write-Host ('LHM HTTP OK port '+$port); $ok=$true; break } catch {} }; if(-not $ok){ Write-Host 'WARN: LHM web server not ready. Run fix_lhm_webserver.bat as Administrator.' }"

echo.
if not defined SILENT pause
