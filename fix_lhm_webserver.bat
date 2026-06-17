@echo off
setlocal EnableDelayedExpansion
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

set "LHM_EXE="
for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\LibreHardwareMonitor*") do (
    if exist "%%D\LibreHardwareMonitor.exe" set "LHM_EXE=%%D\LibreHardwareMonitor.exe"
)
if not defined LHM_EXE (
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$p = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Recurse -Filter 'LibreHardwareMonitor.exe' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName; if ($p) { Write-Output $p }"`) do set "LHM_EXE=%%F"
)

if not defined LHM_EXE (
    echo ERROR: LibreHardwareMonitor.exe not found
    pause
    exit /b 1
)

echo Found: !LHM_EXE!

powershell -NoProfile -Command ^
  "$exe='!LHM_EXE!'; $dir=Split-Path -Path $exe -Parent; $cfg=Join-Path $dir 'LibreHardwareMonitor.config'; [xml]$xml=Get-Content $cfg -Encoding UTF8; $apps=$xml.configuration.appSettings; function Set-Key($k,$v){ $item=$apps.add | Where-Object { $_.key -eq $k }; if(-not $item){ $item=$xml.CreateElement('add'); $item.SetAttribute('key',$k); [void]$apps.AppendChild($item) }; $item.value=$v }; Set-Key 'listenerIp' '0.0.0.0'; Set-Key 'listenerPort' '8090'; Set-Key 'runWebServerMenuItem' 'true'; Set-Key 'authenticationEnabled' 'false'; $xml.Save($cfg); Write-Host 'Config: listener 0.0.0.0:8090, web server enabled'"

echo Stopping old LHM...
taskkill /IM LibreHardwareMonitor.exe /F >nul 2>&1
ping 127.0.0.1 -n 2 >nul

echo Starting LHM (Administrator)...
start "" "!LHM_EXE!"
ping 127.0.0.1 -n 5 >nul

echo Testing data.json...
powershell -NoProfile -Command ^
  "$ok=$false; foreach($port in 8090,8085){ try { $r=Invoke-WebRequest -Uri ('http://127.0.0.1:'+$port+'/data.json') -UseBasicParsing -TimeoutSec 4; Write-Host ('OK port '+$port+' - '+$r.Content.Length+' bytes'); $ok=$true; break } catch { Write-Host ('FAIL port '+$port+' - '+$_.Exception.Message) } }; if(-not $ok){ Write-Host 'Web server still not running.'; Write-Host 'In LHM: Options - Remote Web Server - set IP 0.0.0.0 port 8090 - check Run' }"

echo.
echo Restart PC Monitor Agent after this:
echo   restart_agent.bat
echo.
pause
