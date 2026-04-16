@echo off
REM Upload script for ESP32 DevKit with hinhdongesp32
REM Before running: 
REM 1. Install Arduino IDE
REM 2. Install U8g2lib library
REM 3. Connect ESP32 via USB
REM 4. Change COM_PORT below to your port (COM3, COM4, etc)

REM ===== CONFIGURE THESE =====
set ARDUINO_PATH=C:\Program Files\Arduino
set BOARD=esp32:esp32:esp32
set COM_PORT=COM5
set SPEED=115200

REM ===== RUN UPLOAD =====
echo.
echo Uploading hinhdongesp32.ino to ESP32...
echo Board: %BOARD%
echo Port: %COM_PORT%
echo.

"%ARDUINO_PATH%\arduino.exe" --upload --board %BOARD% --port %COM_PORT% --speed %SPEED% "%CD%\hinhdongesp32.ino"

if %ERRORLEVEL% equ 0 (
    echo.
    echo SUCCESS! Code uploaded to ESP32
    echo.
) else (
    echo.
    echo FAILED! Check:
    echo 1. Arduino IDE path: %ARDUINO_PATH%
    echo 2. COM port: %COM_PORT% (check Device Manager)
    echo 3. Board installed: esp32 package
    echo 4. Library installed: U8g2lib
    echo.
)

pause
