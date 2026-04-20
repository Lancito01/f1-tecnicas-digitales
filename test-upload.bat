@echo off
REM Quick test upload - uploads the test blink sketch
REM Usage: test-upload COM3
REM Or:    test-upload (auto-detect)

setlocal enabledelayedexpansion

set PORT=%1

if "!PORT!"=="" (
    echo [INFO] Auto-detecting COM port...
    for /f "tokens=1" %%A in ('C:\arduino-cli\arduino-cli.exe board list ^| findstr "USB"') do (
        set PORT=%%A
        goto :port_found
    )
    :port_found
    if "!PORT!"=="" (
        echo [ERROR] No USB device found. Specify port manually:
        echo   test-upload COM3
        exit /b 1
    )
)

echo [INFO] Uploading TEST sketch to !PORT!...
echo [INFO] Sketch: C:\arduino-cli\blink\blink.ino
echo.
C:\arduino-cli\quick-upload.bat blink.ino !PORT!
pause
