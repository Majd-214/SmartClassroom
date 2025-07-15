@echo off
setlocal enabledelayedexpansion

:: =================================================================
:: Desktop-Installer.bat (Final Version with File Creation Fix)
:: =================================================================

set "GITHUB_REPO_URL=https://github.com/Majd-214/SmartClassroom/archive/refs/heads/master.zip"
set "REPO_FOLDER_NAME=SmartClassroom-master"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"
set "PROJECT_FOLDER=%DESKTOP_PATH%\%REPO_FOLDER_NAME%"
set "HANDBOOK_FILE=%PROJECT_FOLDER%\Handbook.html"
set "TEMPLATE_FILE=%PROJECT_FOLDER%\Embedded\examples\UniversalDeviceTemplate\UniversalDeviceTemplate.ino"
set "NEW_SKETCH_FILE=%DESKTOP_PATH%\Station_Run.ino"

cls
echo.
echo =======================================================================
echo                McMaster Smart Classroom Installer
echo =======================================================================
echo.
echo [INFO] This script will now download and set up your project files.
echo.

:: --- Step 1: Download GitHub Repository ---
echo [STEP 1/4] Downloading project files...
powershell -Command "Invoke-WebRequest -Uri '%GITHUB_REPO_URL%' -OutFile '%DESKTOP_PATH%\repo.zip'"
if %errorlevel% neq 0 (
    echo [ERROR] Download failed.
    pause
    goto :cleanup
)

:: --- Step 2: Extract Repository ---
echo [STEP 2/4] Extracting files to your desktop...
powershell -Command "Expand-Archive -Path '%DESKTOP_PATH%\repo.zip' -DestinationPath '%DESKTOP_PATH%' -Force"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract files.
    pause
    goto :cleanup
)

:: --- FIX: Add a short delay to allow the file system to catch up ---
echo [INFO] Verifying files...
timeout /t 2 /nobreak > nul

:: --- Step 3: Create Sketch and Open Files ---
echo [STEP 3/4] Creating sketch and opening handbook...
if exist "%TEMPLATE_FILE%" (
    copy "%TEMPLATE_FILE%" "%NEW_SKETCH_FILE%" > nul
    start "" "%NEW_SKETCH_FILE%"
) else (
    echo [ERROR] Could not find the Arduino template file in the extracted folder.
    echo Path checked: %TEMPLATE_FILE%
)

if exist "%HANDBOOK_FILE%" (
    start "" "%HANDBOOK_FILE%"
)

:: --- Step 4: Final Cleanup and Self-Destruct ---
echo [STEP 4/4] Finalizing and cleaning up...
:cleanup
if exist "%DESKTOP_PATH%\repo.zip" del "%DESKTOP_PATH%\repo.zip" > nul

echo.
echo [SUCCESS] Setup is complete! This window will now close.
timeout /t 5 > nul

(goto) 2>nul & del "%~f0"