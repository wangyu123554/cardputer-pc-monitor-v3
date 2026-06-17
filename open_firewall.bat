@echo off
:: Run as Administrator: right-click -> Run as administrator
echo Adding Windows Firewall rule for Cardputer Agent port 8765...
netsh advfirewall firewall add rule name="Cardputer Monitor Agent 8765" dir=in action=allow protocol=TCP localport=8765 profile=any
if errorlevel 1 (
    echo.
    echo FAILED. Please right-click this file and choose "Run as administrator"
) else (
    echo.
    echo SUCCESS. Port 8765 is now allowed for inbound connections.
    echo Cardputer URL should be: http://YOUR_PC_IP:8765/stats
    echo Current PC IP - run: ipconfig
)
pause
