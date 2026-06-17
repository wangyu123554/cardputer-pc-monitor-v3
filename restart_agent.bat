@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "SILENT="
if /i "%~1"=="/silent" set "SILENT=1"

if not defined SILENT echo Restarting PC Monitor Agent...

for /f "tokens=5" %%P in ('netstat -ano ^| findstr ":8765.*LISTENING"') do (
    if not defined SILENT echo Stopping PID %%P on port 8765...
    taskkill /PID %%P /F >nul 2>&1
)

ping 127.0.0.1 -n 2 >nul

if exist "%~dp0PCMonitorAgent.exe" (
    if not defined SILENT echo Starting PCMonitorAgent.exe...
    start "" "%~dp0PCMonitorAgent.exe"
    goto :verify
)

set "PY="
where pythonw >nul 2>&1
if not errorlevel 1 (
    for /f "delims=" %%P in ('where pythonw') do set "PY=%%P"
)
if not defined PY (
    for /f "delims=" %%P in ('where python') do set "PY=%%P"
)

if not defined PY (
    echo ERROR: python not found
    if not defined SILENT pause
    exit /b 1
)

if not defined SILENT echo Starting: !PY!
start "" "!PY!" "%~dp0pc_monitor_agent.py"

:verify
ping 127.0.0.1 -n 4 >nul

if not defined SILENT (
    powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8765/stats' -UseBasicParsing -TimeoutSec 5; Write-Host ('OK - ' + $r.StatusCode) } catch { Write-Host ('FAIL - ' + $_.Exception.Message) }"
    pause
)
