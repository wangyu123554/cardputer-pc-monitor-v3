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

powershell -NoProfile -Command ^
  "$exe='!LHM_EXE!'; $dir=Split-Path -Path $exe -Parent; $cfg=Join-Path $dir 'LibreHardwareMonitor.config'; if (-not (Test-Path $cfg)) { Write-Host 'WARN: config not found'; exit 0 }; [xml]$xml=Get-Content $cfg -Encoding UTF8; $apps=$xml.configuration.appSettings; function Set-Key($k,$v){ $item=$apps.add | Where-Object { $_.key -eq $k }; if(-not $item){ $item=$xml.CreateElement('add'); $item.SetAttribute('key',$k); [void]$apps.AppendChild($item) }; $item.value=$v }; Set-Key 'listenerIp' '0.0.0.0'; Set-Key 'listenerPort' '8090'; Set-Key 'runWebServerMenuItem' 'true'; Set-Key 'authenticationEnabled' 'false'; $xml.Save($cfg); Write-Host 'LHM config: HTTP 0.0.0.0:8090'"

set TASK=LibreHardwareMonitor
schtasks /Query /TN "%TASK%" >nul 2>&1
if not errorlevel 1 schtasks /Delete /TN "%TASK%" /F >nul

echo Creating logon task for LHM...
schtasks /Create /TN "%TASK%" /TR "\"!LHM_EXE!\"" /SC ONLOGON /DELAY 0000:45 /RL HIGHEST /F
if errorlevel 1 (
    echo WARN: Could not create LHM task. Run this bat as Administrator.
) else (
    echo LHM scheduled task created.
)

set "WATCHDOG=%~dp0lhm_watchdog.bat"
set "WATCH_TASK=Cardputer LHM Watchdog"
schtasks /Query /TN "%WATCH_TASK%" >nul 2>&1
if not errorlevel 1 schtasks /Delete /TN "%WATCH_TASK%" /F >nul

echo Creating LHM watchdog (every 3 min, auto-restart if closed)...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$bat='%WATCHDOG%';" ^
  "$name='Cardputer LHM Watchdog';" ^
  "Unregister-ScheduledTask -TaskName $name -Confirm:$false -ErrorAction SilentlyContinue;" ^
  "$action=New-ScheduledTaskAction -Execute $bat;" ^
  "$logon=New-ScheduledTaskTrigger -AtLogOn;" ^
  "$logon.Delay='PT60S';" ^
  "$repeat=New-ScheduledTaskTrigger -Once -At (Get-Date) -RepetitionInterval (New-TimeSpan -Minutes 3) -RepetitionDuration ([TimeSpan]::MaxValue);" ^
  "$settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -ExecutionTimeLimit (New-TimeSpan -Minutes 1);" ^
  "Register-ScheduledTask -TaskName $name -Action $action -Trigger @($logon,$repeat) -Settings $settings -Description 'Restart LibreHardwareMonitor if not running' -Force | Out-Null;" ^
  "Write-Host 'Watchdog task created'"

call "%WATCHDOG%"

taskkill /IM LibreHardwareMonitor.exe /F >nul 2>&1
ping 127.0.0.1 -n 2 >nul

echo Starting LibreHardwareMonitor...
start "" "!LHM_EXE!"
ping 127.0.0.1 -n 6 >nul

powershell -NoProfile -Command ^
  "$ok=$false; foreach($port in 8090,8085){ try { $r=Invoke-WebRequest -Uri ('http://127.0.0.1:'+$port+'/data.json') -UseBasicParsing -TimeoutSec 4; Write-Host ('LHM HTTP OK port '+$port); $ok=$true; break } catch {} }; if(-not $ok){ Write-Host 'WARN: LHM web server not ready. In LHM: Options - Remote Web Server - Run' }"

echo.
if not defined SILENT pause
