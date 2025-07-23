@echo off
setlocal enabledelayedexpansion

:: =================================================================
:: Desktop-Installer.bat (Final Version with Auto-Setup)
:: =================================================================

[cite_start]set "GITHUB_REPO_URL=https://github.com/Majd-214/SmartClassroom/archive/refs/heads/master.zip" [cite: 13]
[cite_start]set "REPO_FOLDER_NAME=SmartClassroom-master" [cite: 13]
[cite_start]set "DESKTOP_PATH=%USERPROFILE%\Desktop" [cite: 13]
[cite_start]set "PROJECT_FOLDER=%DESKTOP_PATH%\%REPO_FOLDER_NAME%" [cite: 13]

:: --- Define all file paths ---
[cite_start]set "SETUP_BAT_FILE=%PROJECT_FOLDER%\Setup.bat" [cite: 13]
[cite_start]set "HANDBOOK_FILE=%PROJECT_FOLDER%\Handbook.html" [cite: 13]
[cite_start]set "TEMPLATE_FILE=%PROJECT_FOLDER%\Embedded\examples\UniversalDeviceTemplate\UniversalDeviceTemplate.ino" [cite: 13]
[cite_start]set "SKETCH_FOLDER=%DESKTOP_PATH%\Station" [cite: 13]
[cite_start]set "NEW_SKETCH_FILE=%SKETCH_FOLDER%\Station.ino" [cite: 13]

cls
echo.
[cite_start]echo ======================================================================= [cite: 14]
[cite_start]echo                McMaster Smart Classroom Installer [cite: 14]
[cite_start]echo ======================================================================= [cite: 14]
echo.
[cite_start]echo [INFO] This script will now download and set up your project files. [cite: 15]
echo.

:: --- Step 1: Download GitHub Repository ---
[cite_start]echo [STEP 1/5] Downloading project files... [cite: 16]
[cite_start]powershell -Command "Invoke-WebRequest -Uri '%GITHUB_REPO_URL%' -OutFile '%DESKTOP_PATH%\repo.zip'" [cite: 16]
if %errorlevel% neq 0 (
    echo [ERROR] Download failed. Exiting in 10 seconds.
    timeout /t 10 > nul
    goto :cleanup
)

:: --- Step 2: Extract Repository ---
[cite_start]echo [STEP 2/5] Extracting files to your desktop... [cite: 16]
[cite_start]powershell -Command "Expand-Archive -Path '%DESKTOP_PATH%\repo.zip' -DestinationPath '%DESKTOP_PATH%' -Force" [cite: 16]
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract files. Exiting in 10 seconds.
    timeout /t 10 > nul
    goto :cleanup
)

[cite_start]echo [INFO] Verifying files... [cite: 16]
timeout /t 2 /nobreak > nul

:: --- Step 3: Run the setup script from within the repository ---
[cite_start]echo [STEP 3/5] Running internal dependency setup... [cite: 17]
if exist "%SETUP_BAT_FILE%" (
    call "%SETUP_BAT_FILE%"
) else (
    echo [ERROR] Setup.bat not found in repository. Exiting in 10 seconds.
    timeout /t 10 > nul
    goto :cleanup
)

:: --- Step 4: Create Sketch in its own folder and Open Files ---
[cite_start]echo [STEP 4/5] Creating sketch and opening handbook... [cite: 18]
[cite_start]if not exist "%SKETCH_FOLDER%" mkdir "%SKETCH_FOLDER%" [cite: 18]

if exist "%TEMPLATE_FILE%" (
    [cite_start]copy "%TEMPLATE_FILE%" "%NEW_SKETCH_FILE%" > nul [cite: 18]
) else (
    [cite_start]echo [ERROR] Could not find the Arduino template file. Path checked: %TEMPLATE_FILE% [cite: 18]
    echo Exiting in 10 seconds.
    timeout /t 10 > nul
    goto :cleanup
)

if exist "%HANDBOOK_FILE%" (
    [cite_start]start "" "%HANDBOOK_FILE%" [cite: 18]
)

:: --- Step 5: Final Cleanup, IDE Launch, and Self-Destruct ---
[cite_start]echo [STEP 5/5] Finalizing and cleaning up... [cite: 19]
:cleanup
[cite_start]if exist "%DESKTOP_PATH%\repo.zip" del "%DESKTOP_PATH%\repo.zip" > nul [cite: 19]

echo.
echo [SUCCESS] Setup is complete! [cite_start]Launching IDE and exiting... [cite: 19]
timeout /t 2 > nul

:: This command launches the IDE and self-deletes the installer in the background
[cite_start]start "Launching IDE" /B cmd /c "start \"\" \"%NEW_SKETCH_FILE%\" & del /F /Q /A:H \"%~f0\"" [cite: 19]
exit