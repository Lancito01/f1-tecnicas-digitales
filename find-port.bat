@echo off
REM Quick alias to find your ESP32 COM port
REM Usage: find-port

echo Finding ESP32 COM port...
echo.
C:\arduino-cli\arduino-cli.exe board list
echo.
echo Note the port with (USB) in it - that's your ESP32!
pause
