@echo off
setlocal

:: =================================================================
:: Cleanup.bat
:: =================================================================

set "PROJECT_FOLDER=%USERPROFILE%\Desktop\SmartClassroom-3fa4168a3424f2cbcc5a3b10c67077607f5d2977"
set "SKETCH_FILE=%USERPROFILE%\Desktop\Station_Run.ino"
set "ARDUINO_LIB_PATH=%USERPROFILE%\Documents\Arduino\libraries"
set "ESP8266_PACKAGE_PATH=%LOCALAPPDATA%\Arduino15\packages\esp8266"

echo.
echo This script will remove the Smart Classroom project and related files.
echo.
set /p "choice=Are you sure you want to continue? (Y/N): "
if /i not "%choice%"=="Y" goto :eof

:: --- Step 1: Delete Project Folder and Sketch ---
echo [CLEANUP] Removing project folder and desktop sketch...
if exist "%PROJECT_FOLDER%" (
    rmdir /s /q "%PROJECT_FOLDER%"
    echo [OK] Project folder deleted.
)
if exist "%SKETCH_FILE%" (
    del "%SKETCH_FILE%"
    echo [OK] Desktop sketch deleted.
)

:: --- Step 2: Uninstall Environment (Optional) ---
echo.
set /p "uninstall_env=Do you want to uninstall the ESP8266 boards and libraries? (Y/N): "
if /i not "%uninstall_env%"=="Y" goto :finished

echo [CLEANUP] Removing ESP8266 board package...
if exist "%ESP8266_PACKAGE_PATH%" (
    rmdir /s /q "%ESP8266_PACKAGE_PATH%"
    echo [OK] ESP8266 package deleted.
)

echo [CLEANUP] Removing installed libraries...
set "LIBRARIES_TO_UNINSTALL=PubSubClient ArduinoJson Adafruit_NeoPixel"
for %%L in (%LIBRARIES_TO_UNINSTALL%) do (
    if exist "%ARDUINO_LIB_PATH%\%%L" (
        rmdir /s /q "%ARDUINO_LIB_PATH%\%%L"
        echo [OK] Library '%%L' deleted.
    )
)

:finished
echo.
echo [SUCCESS] Cleanup complete.
pause