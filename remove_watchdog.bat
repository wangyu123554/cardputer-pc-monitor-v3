@echo off
setlocal
cd /d "%~dp0"

net session >nul 2>&1
if errorlevel 1 (
    echo Requesting Administrator privileges...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo Removing periodic watchdog scheduled tasks (source of console flashes)...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0remove_watchdog_tasks.ps1"
echo.
pause
