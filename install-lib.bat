@echo off
REM Quick alias to install libraries
REM Usage: install-lib "WiFi"

setlocal enabledelayedexpansion

if "%1"=="" (
    echo Usage: install-lib "Library Name"
    echo.
    echo Examples:
    echo   install-lib "WiFi"
    echo   install-lib "Adafruit GFX Library"
    echo   install-lib "DHT sensor library"
    echo.
    echo Not sure of the exact name? Run: search-lib keyword
    exit /b 1
)

set LIBRARY=%1

echo [INFO] Installing library: !LIBRARY!
echo This may take a moment...
echo.
C:\arduino-cli\arduino-cli.exe lib install !LIBRARY!

if errorlevel 1 (
    echo.
    echo [ERROR] Installation failed. Check the library name with:
    echo   search-lib !LIBRARY!
    pause
    exit /b 1
) else (
    echo.
    echo [SUCCESS] Library installed: !LIBRARY!
    pause
    exit /b 0
)
