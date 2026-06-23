@echo off
setlocal
cd /d "%~dp0"
powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%~dp0agent_watchdog.ps1"
exit /b 0
