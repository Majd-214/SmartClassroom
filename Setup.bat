@echo off
setlocal

:: =================================================================
:: Setup.bat (Improved Performance)
:: Purpose: Sets up the Arduino IDE environment efficiently.
:: 1. Checks for ESP8266 board support before installing.
:: 2. Checks for required libraries before installing.
:: =================================================================

:: Path to Arduino15 folder where packages and libraries are stored
set "ARDUINO_DATA_PATH=%APPDATA%\Arduino15"
set "ARDUINO_LIB_PATH=%USERPROFILE%\Documents\Arduino\libraries"
set "ESP8266_PACKAGE_PATH=%ARDUINO_DATA_PATH%\packages\esp8266"
set "ESP8266_BOARD_URL=http://arduino.esp8266.com/stable/package_esp8266com_index.json"

echo.
echo [INFO] Checking Arduino IDE environment...
echo.

:: --- Step 1: Check/Install ESP8266 Board Support ---
if exist "%ESP8266_PACKAGE_PATH%" (
    echo [OK] ESP8266 Boards package already installed.
) else (
    echo [INSTALL] ESP8266 Boards package not found. Installing...
    :: This command requires the Arduino CLI to be in the system PATH
    :: Alternatively, you can bundle the CLI with your USB installer.
    arduino-cli.exe core update-index --additional-urls %ESP8266_BOARD_URL%
    arduino-cli.exe core install esp8266:esp8266 --additional-urls %ESP8266_BOARD_URL%
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to install ESP8266 package. Please install manually from the Boards Manager.
    ) else (
        echo [SUCCESS] ESP8266 Boards package installed.
    )
)

:: --- Step 2: Check/Install Required Libraries ---
:: The libraries are PubSubClient, ArduinoJson, and Adafruit NeoPixel
set "LIBRARIES_TO_CHECK=PubSubClient ArduinoJson Adafruit_NeoPixel"

echo.
echo [INFO] Checking for required libraries...
for %%L in (%LIBRARIES_TO_CHECK%) do (
    if exist "%ARDUINO_LIB_PATH%\%%L" (
        echo [OK] Library '%%L' already installed.
    ) else (
        echo [INSTALL] Library '%%L' not found. Installing...
        arduino-cli.exe lib install "%%L"
        if !errorlevel! neq 0 (
            echo [ERROR] Failed to install library '%%L'. Please install it manually.
        ) else (
            echo [SUCCESS] Library '%%L' installed.
        )
    )
)

echo.
echo [SUCCESS] Environment check complete.
echo.
pause