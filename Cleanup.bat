@echo off
setlocal

echo ===================================================================
echo   McMaster Smart Classroom - Environment Cleanup
echo ===================================================================
echo This script will attempt to uninstall all libraries and boards
echo installed by the setup script.
echo.

:: --- 1. Find CLI ---
echo [+] Searching for arduino-cli.exe...
set "CLI_DIR=%CD%\arduino_cli"
set "ARDUINO_CLI_PATH="
where arduino-cli >nul 2>nul
if %errorlevel%==0 (
    echo    --> Found arduino-cli in system PATH.
    set ARDUINO_CLI_PATH=FOUND
)

if not defined ARDUINO_CLI_PATH (
    if exist "%CLI_DIR%\arduino-cli.exe" (
        echo    --> Found local CLI in %CLI_DIR%
        set "PATH=%PATH%;%CLI_DIR%"
    )
)

if not defined ARDUINO_CLI_PATH (
    echo WARNING: arduino-cli.exe not found. Cannot run uninstall commands.
    echo          Please uninstall libraries and boards manually in the Arduino IDE.
    goto :delete_cli
)

:: --- 2. Reset Security Setting and Uninstall Libraries ---
echo.
echo [1/3] Resetting Config and Uninstalling Libraries...
arduino-cli config set library.enable_unsafe_install false
arduino-cli lib uninstall "SmartDevice" "DHT sensor library" "Adafruit Unified Sensor" "Adafruit NeoPixel" "PubSubClient" "ArduinoJson"

:: --- 3. Uninstall Board ---
echo.
echo [2/3] Uninstalling ESP8266 Board Package...
arduino-cli core uninstall esp8266:esp8266

:: --- 4. Delete Downloaded CLI ---
:delete_cli
echo.
echo [3/3] Deleting temporary CLI folder...
if exist "%CLI_DIR%" (
    rmdir /s /q "%CLI_DIR%"
    echo    --> Folder '%CLI_DIR%' has been removed.
) else (
    echo    --> No local CLI folder to remove.
)

echo.
echo ===================================================================
echo   Cleanup Complete.
echo ===================================================================

:end
endlocal
echo Press any key to exit.
pause > nul