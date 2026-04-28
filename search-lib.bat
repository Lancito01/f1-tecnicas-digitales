@echo off
REM Quick alias to search for libraries
REM Usage: search-lib WiFi

setlocal enabledelayedexpansion

if "%1"=="" (
    echo Usage: search-lib keyword
    echo.
    echo Examples:
    echo   search-lib WiFi
    echo   search-lib DHT
    echo   search-lib Adafruit
    exit /b 1
)

set KEYWORD=%1

echo [INFO] Searching for libraries with keyword: !KEYWORD!
echo.
C:\arduino-cli\arduino-cli.exe lib search !KEYWORD!
pause
