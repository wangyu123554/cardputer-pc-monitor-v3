@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo ========================================
echo  Cardputer PC Monitor v3 - Build Agent EXE
echo ========================================
echo.

where python >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found.
    pause
    exit /b 1
)

echo Installing build deps...
python -m pip install -r "%~dp0requirements.txt" -q
python -m pip install pyinstaller -q
if errorlevel 1 (
    echo ERROR: pip install failed.
    pause
    exit /b 1
)

echo Building one-file agent...
python -m PyInstaller --noconfirm --clean ^
  --onefile --noconsole ^
  --name PCMonitorAgent ^
  --hidden-import=psutil ^
  --collect-all psutil ^
  "%~dp0pc_monitor_agent.py"

if errorlevel 1 (
    echo ERROR: PyInstaller build failed.
    pause
    exit /b 1
)

if not exist "%~dp0dist\PCMonitorAgent.exe" (
    echo ERROR: dist\PCMonitorAgent.exe not found.
    pause
    exit /b 1
)

copy /Y "%~dp0dist\PCMonitorAgent.exe" "%~dp0PCMonitorAgent.exe" >nul
echo.
echo OK: PCMonitorAgent.exe ready in project folder.
echo Run setup_pc_once.bat to install autostart (prefers EXE over Python).
echo.
pause
