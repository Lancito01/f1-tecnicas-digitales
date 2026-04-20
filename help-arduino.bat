@echo off
REM Master help file - shows all available alias commands
REM Usage: help-arduino

cls
echo.
echo ╔══════════════════════════════════════════════════════════════════════════╗
echo ║              ARDUINO ALIAS COMMANDS - QUICK REFERENCE                    ║
echo ╚══════════════════════════════════════════════════════════════════════════╝
echo.
echo 📍 All alias commands are in your current directory.
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 🔍 FINDING DEVICES:
echo.
echo    find-port
echo    └─ Shows connected COM ports
echo    └─ Use this to find your ESP32's port number
echo    └─ Usage: find-port
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 📤 UPLOADING CODE:
echo.
echo    upload
echo    └─ Upload a sketch to ESP32
echo    └─ Usage: upload sketch.ino COM3
echo    └─ Or:    upload sketch.ino (auto-detect port)
echo.
echo    test-upload
echo    └─ Upload the test blink sketch
echo    └─ Usage: test-upload COM3
echo    └─ Or:    test-upload (auto-detect)
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 🔧 COMPILING:
echo.
echo    compile
echo    └─ Compile a sketch without uploading
echo    └─ Usage: compile C:\path\to\sketch
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 📚 LIBRARY MANAGEMENT:
echo.
echo    search-lib
echo    └─ Search for available libraries
echo    └─ Usage: search-lib WiFi
echo    └─ Usage: search-lib DHT
echo.
echo    install-lib
echo    └─ Install a library
echo    └─ Usage: install-lib "WiFi"
echo    └─ Usage: install-lib "Adafruit GFX Library"
echo.
echo    list-libs
echo    └─ Show all installed libraries
echo    └─ Usage: list-libs
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 💡 QUICK EXAMPLES:
echo.
echo    REM Find your port
echo    find-port
echo.
echo    REM Test if everything works
echo    test-upload COM3
echo.
echo    REM Upload your sketch
echo    upload my_sketch.ino COM3
echo.
echo    REM Search for a library
echo    search-lib WiFi
echo.
echo    REM Install that library
echo    install-lib "WiFi"
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo 🎯 TYPICAL FIRST-TIME WORKFLOW:
echo.
echo    1. find-port                           (find your COM port)
echo    2. test-upload COM3                    (test with blink sketch)
echo    3. upload my_sketch.ino COM3           (upload your code)
echo.
echo ═══════════════════════════════════════════════════════════════════════════
echo.
echo ✨ That's it! All commands run from this directory.
echo.
