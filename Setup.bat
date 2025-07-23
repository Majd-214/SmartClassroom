@echo off
setlocal

echo ===================================================================
echo   McMaster Smart Classroom - Full Project Setup
echo ===================================================================
echo This script will install the Arduino CLI (if needed), all required
echo libraries, and install the SmartDevice library from this repo.
echo.

:: --- 1. Find or Install Arduino CLI ---
echo [+] Searching for arduino-cli.exe...
set "ARDUINO_CLI_PATH="

where arduino-cli >nul 2>nul
if %errorlevel%==0 (
    echo    Found arduino-cli in system PATH.
    set ARDUINO_CLI_PATH=FOUND
)

if not defined ARDUINO_CLI_PATH (
    if exist "%ProgramFiles(x86)%\Arduino\arduino-cli.exe" set "ARDUINO_CLI_PATH=%ProgramFiles(x86)%\Arduino"
    if exist "%ProgramFiles%\Arduino\arduino-cli.exe" set "ARDUINO_CLI_PATH=%ProgramFiles%\Arduino"
    if exist "%LOCALAPPDATA%\Arduino15\arduino-cli.exe" set "ARDUINO_CLI_PATH=%LOCALAPPDATA%\Arduino15"
    
    if defined ARDUINO_CLI_PATH (
        echo    Found arduino-cli at: %ARDUINO_CLI_PATH%
        set "PATH=%PATH%;%ARDUINO_CLI_PATH%"
    )
)

if not defined ARDUINO_CLI_PATH (
    if exist ".\arduino_cli\arduino-cli.exe" (
        echo    Found local CLI in .\arduino_cli
        set "PATH=%PATH%;%CD%\arduino_cli"
    ) else (
        echo    Arduino CLI not found. Attempting to download...
        powershell -NoProfile -ExecutionPolicy Bypass -Command "& { mkdir -p '.\arduino_cli'; $url = 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip'; $zipPath = '.\arduino_cli\arduino-cli.zip'; Write-Host 'Downloading from ' $url; Invoke-WebRequest -Uri $url -OutFile $zipPath; Write-Host 'Extracting...'; Expand-Archive -Path $zipPath -DestinationPath '.\arduino_cli' -Force; Remove-Item $zipPath; }"
        if exist ".\arduino_cli\arduino-cli.exe" (
            echo    Download and extraction complete.
            set "PATH=%PATH%;%CD%\arduino_cli"
        ) else (
            echo.
            echo ERROR: Download or extraction failed. Please check your internet connection.
            goto :end
        )
    )
)

:: --- 2. Configure and Install Cores/Libraries ---
set "ESP_URL=http://arduino.esp8266.com/stable/package_esp8266com_index.json"
echo.
echo [1/5] Configuring Arduino IDE for ESP8266 Boards...
arduino-cli config init --overwrite
arduino-cli config set board_manager.additional_urls %ESP_URL%
arduino-cli core update-index --additional-urls %ESP_URL%
if %errorlevel% neq 0 ( echo ERROR: Failed to update core index. & goto :end )

echo.
echo [2/5] Installing ESP8266 Board Support Package...
arduino-cli core install esp8266:esp8266 --additional-urls %ESP_URL%
if %errorlevel% neq 0 ( echo ERROR: Failed to install ESP8266 boards. & goto :end )

echo.
echo [3/5] Installing Core Libraries...
arduino-cli lib install "PubSubClient" "ArduinoJson" "Adafruit NeoPixel"
if %errorlevel% neq 0 ( echo ERROR: Failed to install core libraries. & goto :end )

echo.
echo [4/5] Installing Station-Specific Libraries...
arduino-cli lib install "Adafruit Unified Sensor" "DHT sensor library"
if %errorlevel% neq 0 ( echo ERROR: Failed to install station-specific libraries. & goto :end )

:: --- 3. Compress and Install SmartDevice Library ---
echo.
echo [5/5] Packaging and Installing SmartDevice Library...
if not exist ".\Embedded" (
    echo ERROR: 'Embedded' folder not found! Cannot create library.
    goto :end
)
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '.\Embedded' -DestinationPath '.\SmartDevice.zip' -Force"
if %errorlevel% neq 0 ( echo ERROR: Failed to create SmartDevice.zip. & goto :end )

arduino-cli config set library.enable_unsafe_install true
arduino-cli lib install --zip-path ".\SmartDevice.zip"
if %errorlevel% neq 0 (
    echo ERROR: Failed to install SmartDevice.zip.
    del ".\SmartDevice.zip"
    goto :end
)

del ".\SmartDevice.zip"
echo    SmartDevice library installed successfully.
arduino-cli config set library.enable_unsafe_install false

echo.
echo ===================================================================
echo   Setup Complete!
echo ===================================================================
echo You can now open the Arduino IDE and find the template under:
echo File ^> Examples ^> SmartDevice ^> UniversalDeviceTemplate
echo.

:end
endlocal
echo Press any key to exit.
pause > nul
