@echo off
setlocal enabledelayedexpansion

if "%1"=="" (
    echo Usage: upload sketch_dir_or_file [PORT]
    echo Example: upload sketch_mar17a COM3
    exit /b 1
)

set SKETCH_PATH=%1
set PORT=%2

REM Replace forward slashes with backslashes
set "SKETCH_PATH=!SKETCH_PATH:/=\!"

REM Auto-detect port if not provided
if "!PORT!"=="" (
    echo [INFO] Auto-detecting COM port...
    for /f "tokens=1" %%A in ('C:\arduino-cli\arduino-cli.exe board list ^| findstr "USB"') do (
        set PORT=%%A
        goto :port_found
    )
    :port_found
    if "!PORT!"=="" (
        echo [ERROR] No USB device found
        exit /b 1
    )
)

echo [INFO] Uploading !SKETCH_PATH! to !PORT!...
C:\arduino-cli\arduino-cli.exe compile --upload -p !PORT! --fqbn esp32:esp32:esp32 "!SKETCH_PATH!" -v

if !errorlevel! equ 0 (
    echo [INFO] Upload successful. Opening Serial Monitor...
    timeout /t 2 /nobreak
    
    REM Check if putty.exe is available
    where putty.exe >nul 2>&1
    if !errorlevel! equ 0 (
        start putty.exe -serial !PORT! -sercfg 9600,8,1,N,N
    ) else (
        echo [WARNING] PuTTY not found. Install PuTTY to use Serial Monitor, or use Arduino IDE.
        echo [INFO] To install: Download from https://www.putty.org/ or via Chocolatey: choco install putty
    )
) else (
    echo [ERROR] Upload failed. Not opening Serial Monitor.
)
