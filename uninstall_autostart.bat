@echo off
setlocal
cd /d "%~dp0"

set TASK=Cardputer PC Monitor Agent

schtasks /Query /TN "%TASK%" >nul 2>&1
if errorlevel 1 (
    echo Task not found: %TASK%
    pause
    exit /b 0
)

schtasks /Delete /TN "%TASK%" /F
echo Autostart removed.
pause
