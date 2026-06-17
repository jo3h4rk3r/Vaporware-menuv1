@echo off
cd /d "%~dp0"

echo =========================
echo 1/3 Building firmware
echo =========================
call build.bat
if errorlevel 1 (
    echo BUILD FAILED
    pause
    exit /b 1
)

echo =========================
echo 2/3 Generating flash file
echo =========================
python gen_direct_flash.py
if errorlevel 1 (
    echo FLASH GENERATION FAILED
    pause
    exit /b 1
)

echo =========================
echo 3/3 Flashing device
echo =========================
call flash.bat
if errorlevel 1 (
    echo FLASH FAILED
    pause
    exit /b 1
)

echo =========================
echo ALL DONE
echo =========================
pause