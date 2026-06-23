@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "SILENT="
if /i "%~1"=="/silent" set "SILENT=1"
if /i "%~1"=="/nopause" set "SILENT=1"

echo ========================================
echo  Cardputer PC Monitor v3 - Install Autostart
echo ========================================
echo.

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "USE_EXE=0"
if exist "%PROJECT_DIR%\PCMonitorAgent.exe" set "USE_EXE=1"

if "%USE_EXE%"=="0" (
    where pythonw >nul 2>&1
    if errorlevel 1 (
        echo ERROR: pythonw not found. Install Python or run build_agent_exe.bat
        if not defined SILENT pause
        exit /b 1
    )
)

echo Installing scheduled task (logon + auto-restart on failure)...
if "%USE_EXE%"=="1" (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
      "$ErrorActionPreference='Stop';" ^
      "$dir='%PROJECT_DIR%';" ^
      "$exe=Join-Path $dir 'PCMonitorAgent.exe';" ^
      "$name='Cardputer PC Monitor Agent';" ^
      "Unregister-ScheduledTask -TaskName $name -Confirm:$false -ErrorAction SilentlyContinue;" ^
      "$action=New-ScheduledTaskAction -Execute $exe -WorkingDirectory $dir;" ^
      "$trigger=New-ScheduledTaskTrigger -AtLogOn;" ^
      "$trigger.Delay='PT30S';" ^
      "$settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -ExecutionTimeLimit ([TimeSpan]::Zero) -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1);" ^
      "Register-ScheduledTask -TaskName $name -Action $action -Trigger $trigger -Settings $settings -Description 'Cardputer Wi-Fi PC Monitor Agent v3 (EXE)' -Force | Out-Null;" ^
      "Write-Host 'Task created (EXE):' $name"
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
      "$ErrorActionPreference='Stop';" ^
      "$dir='%PROJECT_DIR%';" ^
      "$py=(Get-Command pythonw).Source;" ^
      "$script=Join-Path $dir 'pc_monitor_agent.py';" ^
      "$name='Cardputer PC Monitor Agent';" ^
      "Unregister-ScheduledTask -TaskName $name -Confirm:$false -ErrorAction SilentlyContinue;" ^
      "$action=New-ScheduledTaskAction -Execute $py -Argument ('\"{0}\"' -f $script) -WorkingDirectory $dir;" ^
      "$trigger=New-ScheduledTaskTrigger -AtLogOn;" ^
      "$trigger.Delay='PT30S';" ^
      "$settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -ExecutionTimeLimit ([TimeSpan]::Zero) -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1);" ^
      "Register-ScheduledTask -TaskName $name -Action $action -Trigger $trigger -Settings $settings -Description 'Cardputer Wi-Fi PC Monitor Agent v3 (Python)' -Force | Out-Null;" ^
      "Write-Host 'Task created (Python):' $name"
)

if errorlevel 1 (
    echo ERROR: Failed to create scheduled task.
    if not defined SILENT pause
    exit /b 1
)

echo Starting agent now...
schtasks /Run /TN "Cardputer PC Monitor Agent" >nul 2>&1
ping 127.0.0.1 -n 4 >nul

powershell -NoProfile -Command ^
  "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8765/stats' -UseBasicParsing -TimeoutSec 5; Write-Host ('Agent OK - HTTP ' + $r.StatusCode) } catch { Write-Host ('Agent starting... if FAIL persists, run restart_agent.bat') }"

echo Removing periodic watchdog tasks (they flash a console on Windows)...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0remove_watchdog_tasks.ps1" >nul 2>&1

echo.
echo Done. Agent runs hidden at logon (~30s after login).
echo LHM health is checked inside the Agent process — no scheduled watchdog.
echo Remove autostart: uninstall_autostart.bat
echo.
if not defined SILENT pause
