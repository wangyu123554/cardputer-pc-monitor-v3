@echo off
setlocal
cd /d "%~dp0"

net session >nul 2>&1
if errorlevel 1 (
    echo Requesting Administrator privileges...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo ========================================
echo  Fix LHM Remote Web Server
echo ========================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0start_lhm.ps1"
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
    echo LHM is running and HTTP is OK.
) else (
    echo LHM may still be starting. Check http://127.0.0.1:8090/data.json
)
echo.
echo Restart PC Monitor Agent after this:
echo   restart_agent.bat
echo.
pause
