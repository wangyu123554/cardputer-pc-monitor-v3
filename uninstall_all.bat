@echo off
setlocal
cd /d "%~dp0"

echo Removing all Cardputer PC Monitor autostart tasks...

set TASK=Cardputer PC Monitor Agent
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 (
    schtasks /Delete /TN "%TASK%" /F
    echo Removed: %TASK%
) else (
    echo Not found: %TASK%
)

set TASK=LibreHardwareMonitor
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 (
    schtasks /Delete /TN "%TASK%" /F
    echo Removed: %TASK%
) else (
    echo Not found: %TASK%
)

echo.
echo Done. Running processes were NOT stopped.
echo To stop Agent now: restart_agent.bat (kills port 8765 first)
pause
