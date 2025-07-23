@echo off
setlocal enabledelayedexpansion

:: =================================================================
:: Desktop-Installer.bat (Final Version with Auto-Setup)
:: =================================================================

set "GITHUB_REPO_URL=https://github.com/Majd-214/SmartClassroom/archive/refs/heads/master.zip"
set "REPO_FOLDER_NAME=SmartClassroom-master"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"
set "PROJECT_FOLDER=%DESKTOP_PATH%\%REPO_FOLDER_NAME%"

:: --- Define all file paths ---
set "SETUP_BAT_FILE=%PROJECT_FOLDER%\Setup.bat"
set "HANDBOOK_FILE=%PROJECT_FOLDER%\Handbook.html"
set "TEMPLATE_FILE=%PROJECT_FOLDER%\Embedded\examples\UniversalDeviceTemplate\UniversalDeviceTemplate.ino"
set "SKETCH_FOLDER=%DESKTOP_PATH%\Station"
set "NEW_SKETCH_FILE=%SKETCH_FOLDER%\Station.ino"

cls
echo.
echo =======================================================================
echo                McMaster Smart Classroom Installer
echo =======================================================================
echo.
echo [INFO] This script will now download and set up your project files.
echo.

:: --- Step 1: Download GitHub Repository ---
echo [STEP 1/5] Downloading project files...
powershell -Command "Invoke-WebRequest -Uri '%GITHUB_REPO_URL%' -OutFile '%DESKTOP_PATH%\repo.zip'"
if %errorlevel% neq 0 (
    echo [ERROR] Download failed.
    pause
    goto :cleanup
)

:: --- Step 2: Extract Repository ---
echo [STEP 2/5] Extracting files to your desktop...
powershell -Command "Expand-Archive -Path '%DESKTOP_PATH%\repo.zip' -DestinationPath '%DESKTOP_PATH%' -Force"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract files.
    pause
    goto :cleanup
)

echo [INFO] Verifying files...
timeout /t 2 /nobreak > nul

:: --- NEW STEP 3: Run the setup script from within the repository ---
echo [STEP 3/5] Running internal dependency setup...
if exist "%SETUP_BAT_FILE%" (
    :: 'call' runs the other script and waits for it to finish
    call "%SETUP_BAT_FILE%"
) else (
    echo [INFO] No internal Setup.bat found. Skipping.
)

:: --- Step 4: Create Sketch in its own folder and Open Files ---
echo [STEP 4/5] Creating sketch and opening handbook...
if not exist "%SKETCH_FOLDER%" mkdir "%SKETCH_FOLDER%"

if exist "%TEMPLATE_FILE%" (
    copy "%TEMPLATE_FILE%" "%NEW_SKETCH_FILE%" > nul
) else (
    echo [ERROR] Could not find the Arduino template file in the extracted folder.
    echo Path checked: %TEMPLATE_FILE%
)

if exist "%HANDBOOK_FILE%" (
    start "" "%HANDBOOK_FILE%"
)

:: --- Step 5: Final Cleanup, IDE Launch, and Self-Destruct ---
echo [STEP 5/5] Finalizing and cleaning up...
:cleanup
if exist "%DESKTOP_PATH%\repo.zip" del "%DESKTOP_PATH%\repo.zip" > nul

echo.
echo [SUCCESS] Setup is complete! Launching IDE and exiting...
timeout /t 2 > nul

:: This command launches the IDE and self-deletes the installer in the background
start "Launching IDE" /B cmd /c "start \"\" \"%NEW_SKETCH_FILE%\" & del /F /A:H \"%~f0\""
exit
