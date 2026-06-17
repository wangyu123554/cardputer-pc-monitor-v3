@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo ========================================
echo  Add screenshots to docs/images
echo ========================================
echo.
echo Put your 4 photos on the Desktop named:
echo   cardputer-cpu.png
echo   cardputer-disk.png
echo   cardputer-gpu.png
echo   cardputer-settings.png
echo.
echo Or edit this script to point to your files.
echo.

set "DESK=%USERPROFILE%\Desktop"
set "DEST=%~dp0docs\images"

if not exist "%DEST%" mkdir "%DEST%"

set OK=1
for %%F in (cardputer-cpu.png cardputer-disk.png cardputer-gpu.png cardputer-settings.png) do (
    if not exist "%DESK%\%%F" (
        echo MISSING: %DESK%\%%F
        set OK=0
    )
)

if "!OK!"=="0" (
    echo.
    echo Save the 4 chat photos to Desktop with the names above, then run again.
    pause
    exit /b 1
)

copy /Y "%DESK%\cardputer-cpu.png" "%DEST%\cpu-page.png"
copy /Y "%DESK%\cardputer-disk.png" "%DEST%\disk-page.png"
copy /Y "%DESK%\cardputer-gpu.png" "%DEST%\gpu-page.png"
copy /Y "%DESK%\cardputer-settings.png" "%DEST%\settings-page.png"

echo.
echo Done. Files copied to docs\images\
echo Next: git add docs/images && git commit && git push
pause
