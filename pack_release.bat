@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo ========================================
echo  Cardputer PC Monitor v3 - Pack PC Release
echo ========================================
echo.

set "OUT=%~dp0release\PC-Agent-v3"
if exist "%OUT%" rmdir /S /Q "%OUT%"
mkdir "%OUT%"

for %%F in (
    pc_monitor_agent.py
    requirements.txt
    setup_pc_once.bat
    一键安装.bat
    install_autostart.bat
    install_lhm_autostart.bat
    lhm_watchdog.bat
    agent_watchdog.ps1
    lhm_watchdog.ps1
    remove_watchdog_tasks.ps1
    remove_watchdog.bat
    run_lhm_hidden.vbs
    start_lhm.ps1
    restart_agent.bat
    restart_lhm.bat
    run_agent.bat
    run_agent_hidden.vbs
    open_firewall.bat
    uninstall_all.bat
    uninstall_autostart.bat
    fix_lhm_webserver.bat
    setup_lhm.bat
    build_agent_exe.bat
) do (
    if exist "%~dp0%%F" copy /Y "%~dp0%%F" "%OUT%\%%F" >nul
)

if exist "%~dp0PCMonitorAgent.exe" (
    copy /Y "%~dp0PCMonitorAgent.exe" "%OUT%\PCMonitorAgent.exe" >nul
    echo Included PCMonitorAgent.exe
)

> "%OUT%\README.txt" echo Cardputer PC Monitor v3 - PC Agent
>> "%OUT%\README.txt" echo.
>> "%OUT%\README.txt" echo 1. 解压到任意文件夹
>> "%OUT%\README.txt" echo 2. 双击「一键安装.bat」以管理员身份完成配置
>> "%OUT%\README.txt" echo 3. 在 Cardputer 网页配置 PC 局域网 IP
>> "%OUT%\README.txt" echo.
>> "%OUT%\README.txt" echo 可选：运行 build_agent_exe.bat 生成免 Python 的 PCMonitorAgent.exe

powershell -NoProfile -Command "Compress-Archive -Path '%OUT%\*' -DestinationPath '%~dp0release\PC-Agent-v3.zip' -Force"

echo.
echo OK: release\PC-Agent-v3.zip
echo.
pause
