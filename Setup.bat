@echo off
setlocal

echo ===================================================================
[cite_start]echo   McMaster Smart Classroom - Full Project Setup [cite: 1]
echo ===================================================================
[cite_start]echo This script will install the Arduino CLI (if needed), all required [cite: 1]
[cite_start]echo libraries, and install the SmartDevice library from this repo. [cite: 1]
echo.

:: --- 1. Find or Install Arduino CLI ---
[cite_start]echo [+] Searching for arduino-cli.exe... [cite: 2]
[cite_start]set "ARDUINO_CLI_PATH=" [cite: 2]
where arduino-cli >nul 2>nul
if %errorlevel%==0 (
    [cite_start]echo    Found arduino-cli in system PATH. [cite: 2]
    goto :cli_found
)

[cite_start]if exist "%ProgramFiles(x86)%\Arduino\arduino-cli.exe" set "ARDUINO_CLI_PATH=%ProgramFiles(x86)%\Arduino" [cite: 2]
[cite_start]if exist "%ProgramFiles%\Arduino\arduino-cli.exe" set "ARDUINO_CLI_PATH=%ProgramFiles%\Arduino" [cite: 2]
[cite_start]if exist "%LOCALAPPDATA%\Arduino15\arduino-cli.exe" set "ARDUINO_CLI_PATH=%LOCALAPPDATA%\Arduino15" [cite: 2]

if defined ARDUINO_CLI_PATH (
    [cite_start]echo    Found arduino-cli at: %ARDUINO_CLI_PATH% [cite: 2]
    [cite_start]set "PATH=%PATH%;%ARDUINO_CLI_PATH%" [cite: 2]
    goto :cli_found
)

if exist ".\arduino_cli\arduino-cli.exe" (
    [cite_start]echo    Found local CLI in .\arduino_cli [cite: 3]
    [cite_start]set "PATH=%PATH%;%CD%\arduino_cli" [cite: 3]
) else (
    [cite_start]echo    Arduino CLI not found. Attempting to download... [cite: 3]
    [cite_start]powershell -NoProfile -ExecutionPolicy Bypass -Command "& { mkdir -p '.\arduino_cli'; [cite: 3] $url = 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip'; $zipPath = '.\arduino_cli\arduino-cli.zip'; Write-Host 'Downloading from ' $url; Invoke-WebRequest -Uri $url -OutFile $zipPath; [cite_start]Write-Host 'Extracting...'; [cite: 4] Expand-Archive -Path $zipPath -DestinationPath '.\arduino_cli' -Force; Remove-Item $zipPath; [cite_start]}" [cite: 5]
    if exist ".\arduino_cli\arduino-cli.exe" (
        [cite_start]echo    Download and extraction complete. [cite: 5]
        [cite_start]set "PATH=%PATH%;%CD%\arduino_cli" [cite: 5]
    ) else (
        echo.
        [cite_start]echo ERROR: Download or extraction failed. Please check internet connection. [cite: 5]
        goto :fail
    )
)

:cli_found
:: --- 2. Configure and Install Cores/Libraries ---
[cite_start]set "ESP_URL=http://arduino.esp8266.com/stable/package_esp8266com_index.json" [cite: 6]
echo.
[cite_start]echo [1/5] Configuring Arduino IDE for ESP8266 Boards... [cite: 6]
arduino-cli config init --overwrite > nul
arduino-cli config set board_manager.additional_urls %ESP_URL% > nul
arduino-cli core update-index --additional-urls %ESP_URL% > nul
[cite_start]if %errorlevel% neq 0 ( echo ERROR: Failed to update core index. [cite: 6] & goto :fail )

echo.
[cite_start]echo [2/5] Installing ESP8266 Board Support Package... [cite: 6]
arduino-cli core install esp8266:esp8266 --additional-urls %ESP_URL% > nul
[cite_start]if %errorlevel% neq 0 ( echo ERROR: Failed to install ESP8266 boards. [cite: 6] & goto :fail )

echo.
[cite_start]echo [3/5] Installing Core Libraries... [cite: 7]
arduino-cli lib install "PubSubClient" "ArduinoJson" "Adafruit NeoPixel" > nul
[cite_start]if %errorlevel% neq 0 ( echo ERROR: Failed to install core libraries. [cite: 7] & goto :fail )

echo.
[cite_start]echo [4/5] Installing Station-Specific Libraries... [cite: 8]
arduino-cli lib install "Adafruit Unified Sensor" "DHT sensor library" > nul
[cite_start]if %errorlevel% neq 0 ( echo ERROR: Failed to install station-specific libraries. [cite: 8] & goto :fail )

:: --- 3. Compress and Install SmartDevice Library ---
echo.
[cite_start]echo [5/5] Packaging and Installing SmartDevice Library... [cite: 9]
if not exist ".\Embedded" (
    [cite_start]echo ERROR: 'Embedded' folder not found! Cannot create library. [cite: 9]
    goto :fail
)
[cite_start]powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '.\Embedded' -DestinationPath '.\SmartDevice.zip' -Force" [cite: 9]
[cite_start]if %errorlevel% neq 0 ( echo ERROR: Failed to create SmartDevice.zip. [cite: 9] & goto :fail )

[cite_start]arduino-cli config set library.enable_unsafe_install true [cite: 9]
arduino-cli lib install --zip-path ".\SmartDevice.zip" > nul
if %errorlevel% neq 0 (
    [cite_start]echo ERROR: Failed to install SmartDevice.zip. [cite: 9]
    del ".\SmartDevice.zip" > nul
    goto :fail
)

del ".\SmartDevice.zip" > nul
[cite_start]echo    SmartDevice library installed successfully. [cite: 9]
[cite_start]arduino-cli config set library.enable_unsafe_install false [cite: 10]

echo.
echo ===================================================================
[cite_start]echo   Setup Complete! [cite: 10]
echo ===================================================================
[cite_start]echo You can now open the Arduino IDE and find the template under: [cite: 11]
[cite_start]echo File ^> Examples ^> SmartDevice ^> UniversalDeviceTemplate [cite: 11]
echo.
goto :end

:fail
echo.
echo [FATAL] An error occurred. This window will close in 10 seconds.
timeout /t 10 > nul
exit /b 1

:end
endlocal
exit /b 0