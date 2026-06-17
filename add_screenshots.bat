@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
chcp 65001 >nul

echo ========================================
echo  v3 - 导入六页截图到 docs/images
echo ========================================
echo.

set "SRC=%USERPROFILE%\Desktop\Cardputer-System-Monitor-main - 副本\截图2"
set "DEST=%~dp0docs\images"

if not exist "%DEST%" mkdir "%DEST%"

if not exist "%SRC%" (
    echo ERROR: 找不到截图文件夹:
    echo   %SRC%
    pause
    exit /b 1
)

python -c "from pathlib import Path; from PIL import Image; src=Path(r'%SRC%'); dest=Path(r'%DEST%'); files=sorted(src.glob('*.jpg')); names=['cpu-page.jpg','ram-page.jpg','proc-page.jpg','io-page.jpg','gpu-page.jpg','settings-page.jpg'];
assert len(files)==6, f'need 6 jpg, got {len(files)}';
[old.unlink() for old in dest.glob('*.jpg')];
[(lambda s,n: (lambda im: im.save(dest/n,'JPEG',quality=85,optimize=True))( (lambda im: im.resize((960,int(im.height*960/im.width)), Image.LANCZOS) if im.width>960 else im)(Image.open(s).convert('RGB')) ))(s,n) for s,n in zip(files,names)]; print('OK:', len(names), 'images')"

if errorlevel 1 (
    echo ERROR: Python/Pillow failed. Run: pip install pillow
    pause
    exit /b 1
)

echo.
echo Done. docs\images\ 已更新六页截图
echo Next: git add docs/images README.md README.en.md && git commit && git push
pause
