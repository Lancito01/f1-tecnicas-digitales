@echo off
REM Quick alias to list installed libraries
REM Usage: list-libs

echo [INFO] Listing installed libraries...
echo.
C:\arduino-cli\arduino-cli.exe lib list
pause
