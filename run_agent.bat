@echo off
cd /d "%~dp0"
echo Starting Cardputer PC Monitor Agent...
python pc_monitor_agent.py
if errorlevel 1 (
    echo.
    echo Agent exited with an error. Check that Python and psutil are installed.
)
pause
