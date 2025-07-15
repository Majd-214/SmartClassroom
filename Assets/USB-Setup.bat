@echo off
setlocal

:: =================================================================
:: USB-Setup.bat (Updated)
:: Purpose: Copies a hidden installer to the desktop and executes it.
:: The desktop script will handle the USB ejection.
:: =================================================================

set "desktop_script_name=Desktop-Installer.bat"
set "desktop_path=%USERPROFILE%\Desktop"

echo.
echo [INFO] Initializing McMaster Smart Classroom setup...
echo.

:: Use xcopy with /H to ensure hidden files are copied
xcopy "%~dp0\%desktop_script_name%" "%desktop_path%\" /H /Y /I > nul
if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy the setup script to the desktop.
    echo Please ensure the USB drive is readable and try again.
    pause
    exit /b
)

:: Launch the desktop script in a new process and pass the USB drive letter to it.
:: This allows this script to terminate, freeing the lock on the USB drive.
start "Installer" /D "%desktop_path%" cmd /c ""%desktop_path%\%desktop_script_name%" %~d0"

exit