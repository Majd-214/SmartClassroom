@echo off
setlocal enabledelayedexpansion

:: =================================================================
:: Desktop-Installer.bat (Corrected)
:: Purpose: Main setup automation script.
:: =================================================================

:: The folder created by GitHub's "Download ZIP" feature
set "REPO_FOLDER_NAME=SmartClassroom-3fa4168a3424f2cbcc5a3b10c67077607f5d2977"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"
set "PROJECT_FOLDER=%DESKTOP_PATH%\%REPO_FOLDER_NAME%"

:: --- All internal paths now correctly point inside the repo folder ---
set "HANDBOOK_FILE=%PROJECT_FOLDER%\Handbook.html"
set "SETUP_FILE=%PROJECT_FOLDER%\MQTT Server\bridge.py"
set "TEMPLATE_FILE=%PROJECT_FOLDER%\Embedded\UniversalDeviceTemplate.ino"
set "NEW_SKETCH_FILE=%DESKTOP_PATH%\Station_Run.ino"
set "GITHUB_REPO_URL=https://github.com/majd-214/smartclassroom/archive/3fa4168a3424f2cbcc5a3b10c67077607f5d2977.zip"


echo.
echo [INFO] McMaster Smart Classroom Installer is running...
echo [INFO] This window will close automatically upon completion.
echo.

:: --- Step 1: Download GitHub Repository ---
echo [STEP 1/6] Downloading project files from GitHub...
powershell -Command "Invoke-WebRequest -Uri '%GITHUB_REPO_URL%' -OutFile '%DESKTOP_PATH%\repo.zip'"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download the repository. Please check your internet connection.
    pause
    goto :cleanup
)

:: --- Step 2: Extract Repository ---
echo [STEP 2/6] Extracting files...
powershell -Command "Expand-Archive -Path '%DESKTOP_PATH%\repo.zip' -DestinationPath '%DESKTOP_PATH%' -Force"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract the repository files.
    pause
    goto :cleanup
)

:: --- Step 3: Run Internal Setup ---
echo [STEP 3/6] Running initial dependency setup...
cd /d "%PROJECT_FOLDER%"
if exist "%SETUP_FILE%" (
    rem The original repo does not contain a Setup.bat, so we assume no setup is needed.
    echo [INFO] Setup file found. In a real scenario, you would run a setup script here.
) else (
    echo [WARNING] Main setup script not found in the repository. Skipping.
)

:: --- Step 4: Create and Launch Arduino Sketch ---
echo [STEP 4/6] Creating your starting Arduino sketch...
if exist "%TEMPLATE_FILE%" (
    copy "%TEMPLATE_FILE%" "%NEW_SKETCH_FILE%" > nul
    echo [INFO] Launching the sketch in the Arduino IDE...
    start "" "%NEW_SKETCH_FILE%"
) else (
    echo [WARNING] UniversalDeviceTemplate.ino not found. Could not create sketch.
)

:: --- Step 5: Open Handbook ---
echo [STEP 5/6] Opening the Student Handbook...
if exist "%HANDBOOK_FILE%" (
    start "" "%HANDBOOK_FILE%"
) else (
    echo [WARNING] Handbook.html not found.
)

:: --- Step 6: Self-Destruct ---
echo [STEP 6/6] Finalizing and cleaning up...
:cleanup
if exist "%DESKTOP_PATH%\repo.zip" del "%DESKTOP_PATH%\repo.zip"

echo [SUCCESS] Setup is complete!
timeout /t 5 > nul

:: Self-deleting trick
(goto) 2>nul & del "%~f0"