@echo off
REM Quick alias to compile without uploading
REM Usage: compile your_sketch_folder

setlocal enabledelayedexpansion

if "%1"=="" (
    echo Usage: compile sketch_folder
    echo.
    echo Example:
    echo   compile C:\Projects\my_sketch
    exit /b 1
)

set SKETCH_DIR=%1

echo [INFO] Compiling !SKETCH_DIR!...
echo.
C:\arduino-cli\arduino-cli.exe compile --fqbn esp32:esp32:esp32 "!SKETCH_DIR!" -v

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
) else (
    echo.
    echo [SUCCESS] Compilation completed!
    pause
    exit /b 0
)
