@echo off
setlocal

:: ===================================================================
::   McMaster Smart Classroom - Project Launcher v4.0
::   (Place this file on the root of your USB drive)
:: ===================================================================

echo Starting the Smart Classroom setup process...
echo This will download the latest project from GitHub to your Desktop.
echo.

:: --- 1. Define Paths and URLs ---
set "REPO_URL=https://github.com/Majd-214/SmartClassroom/archive/refs/heads/master.zip"
set "ZIP_FILE=%USERPROFILE%\Desktop\SmartClassroom-master.zip"
set "EXTRACT_DIR=%USERPROFILE%\Desktop"
set "PROJECT_FOLDER=%USERPROFILE%\Desktop\SmartClassroom-master"
set "TEMPLATE_INO=%PROJECT_FOLDER%\Embedded\UniversalDeviceTemplate\UniversalDeviceTemplate.ino"
set "DESKTOP_INO=%USERPROFILE%\Desktop\Station_run.ino"

:: --- 2. Download and Unzip the GitHub Repository to the Desktop ---
echo [+] Downloading latest version from GitHub to your Desktop...
powershell -NoProfile -ExecutionPolicy Bypass -Command "& { Write-Host 'Downloading from %REPO_URL%'; Invoke-WebRequest -Uri '%REPO_URL%' -OutFile '%ZIP_FILE%'; Write-Host 'Extracting to %EXTRACT_DIR%'; Expand-Archive -Path '%ZIP_FILE%' -DestinationPath '%EXTRACT_DIR%' -Force; }"

:: Check if extraction was successful
if not exist "%PROJECT_FOLDER%\" (
    echo.
    echo ERROR: Failed to download or extract the repository.
    echo Please check your internet connection and try again.
    goto :end
)
echo    [INFO] Repository downloaded and extracted successfully.
echo.

:: --- 3. Run the Setup Script ---
echo [+] Navigating into the project folder and running setup...
cd /D "%PROJECT_FOLDER%"
call Setup.bat

:: Check if Setup.bat was successful
if %errorlevel% neq 0 (
    echo.
    echo ERROR: The setup script failed. Please review the errors above.
    cd /D "%EXTRACT_DIR%"
    goto :cleanup_and_end
)
echo    [INFO] Setup script completed successfully!
echo.

:: --- 4. Copy Template and Launch ---
echo [+] Preparing sketch and launching applications...

:: Copy the template to the desktop with the new name
echo    [INFO] Creating your personal sketch file on the Desktop...
copy "%TEMPLATE_INO%" "%DESKTOP_INO%" >nul
if %errorlevel% neq 0 (
    echo ERROR: Could not create Station_run.ino on the Desktop.
    goto :cleanup_and_end
)

:: Launch the handbook
echo    [INFO] Opening Handbook.html...
start "" "%PROJECT_FOLDER%\Handbook.html"

:: Find and launch Arduino IDE with the new sketch
set "ARDUINO_EXE_PATH="
if exist "%ProgramFiles(x86)%\Arduino IDE\arduino.exe" set "ARDUINO_EXE_PATH=%ProgramFiles(x86)%\Arduino IDE\arduino.exe"
if exist "%ProgramFiles%\Arduino IDE\arduino.exe" set "ARDUINO_EXE_PATH=%ProgramFiles%\Arduino IDE\arduino.exe"
if exist "%ProgramFiles(x86)%\Arduino\arduino.exe" set "ARDUINO_EXE_PATH=%ProgramFiles(x86)%\Arduino\arduino.exe"
if exist "%ProgramFiles%\Arduino\arduino.exe" set "ARDUINO_EXE_PATH=%ProgramFiles%\Arduino\arduino.exe"

if defined ARDUINO_EXE_PATH (
    echo    [INFO] Opening your Station_run.ino sketch...
    start "Arduino" "%ARDUINO_EXE_PATH%" "%DESKTOP_INO%"
) else (
    echo WARNING: Could not find arduino.exe. Please open your sketch manually.
    echo          The file is named Station_run.ino and is on your Desktop.
)

:: --- 5. Cleanup and Eject ---
:cleanup_and_end
cd /D "%EXTRACT_DIR%"
del "%ZIP_FILE%" >nul 2>nul
echo.
echo ===================================================================
echo   Process Finished. The USB drive will now be safely ejected.
echo ===================================================================
echo.

:: Eject the USB drive this script is running from
set "USBDrive=%~d0"
echo [+] Ejecting USB Drive (%USBDrive%). Please remove it once the light stops flashing.
powershell -NoProfile -ExecutionPolicy Bypass -Command "$driveEject = New-Object -comObject Shell.Application; $driveEject.Namespace(17).ParseName('%USBDrive%').InvokeVerb('Eject')"

:: A final pause to allow the eject command to process
timeout /t 4 /nobreak >nul
goto :final_end

:end
endlocal
echo Press any key to exit.
pause > nul

:final_end