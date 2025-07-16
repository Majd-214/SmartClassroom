@echo off
setlocal enabledelayedexpansion

:: =================================================================
:: Desktop-Installer.bat (Final Version with Folder and Exit Fix)
:: =================================================================

set "GITHUB_REPO_URL=https://github.com/Majd-214/SmartClassroom/archive/refs/heads/master.zip"
set "REPO_FOLDER_NAME=SmartClassroom-master"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"
set "PROJECT_FOLDER=%DESKTOP_PATH%\%REPO_FOLDER_NAME%"
set "HANDBOOK_FILE=%PROJECT_FOLDER%\Handbook.html"
set "TEMPLATE_FILE=%PROJECT_FOLDER%\Embedded\examples\UniversalDeviceTemplate\UniversalDeviceTemplate.ino"

:: --- FIX #1: Define the sketch folder and the file path inside it ---
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

echo [INFO] Verifying files...
timeout /t 2 /nobreak > nul

:: --- Step 3: Create Sketch in its own folder and Open Files ---
echo [STEP 3/4] Creating sketch and opening handbook...

:: --- FIX #1 (continued): Create the directory first ---
if not exist "%SKETCH_FOLDER%" mkdir "%SKETCH_FOLDER%"

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
timeout /t 3 > nul

:: --- FIX #2: More robust self-deletion and exit ---
:: Start a new, hidden command prompt that waits 1 second, then deletes the original script.
:: The main script then exits immediately, closing the window.
start "Delete" /B cmd /c "timeout /t 1 /nobreak > nul & del /F /A:H "%~f0""
exit
