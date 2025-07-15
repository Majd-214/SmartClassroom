@echo off
setlocal

:: =================================================================
:: USB-Setup.bat
:: Purpose: Initiates the setup from a USB drive.
:: 1. Copies the main desktop script to the user's desktop.
:: 2. Executes the desktop script.
:: 3. Ejects the USB drive.
:: =================================================================

:: Get the drive letter of the USB stick this script is running from
set "usb_drive=%~d0"
set "desktop_script_name=Desktop-Installer.bat"
set "desktop_path=%USERPROFILE%\Desktop"

echo.
echo [INFO] Starting the McMaster Smart Classroom setup...
echo [INFO] Please do not remove the USB drive until prompted.
echo.

:: Copy the desktop script from the USB to the user's desktop
:: Assumes Desktop-Installer.bat is in the same directory as this script
echo [STEP 1] Preparing setup files...
copy "%~dp0\%desktop_script_name%" "%desktop_path%\%desktop_script_name%" > nul
if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy the setup script to the desktop.
    pause
    exit /b
)

:: Run the script that was just copied to the desktop
echo [STEP 2] Launching the main installer...
start "" /D "%desktop_path%" "%desktop_path%\%desktop_script_name%"

:: Eject the USB drive using a PowerShell command
echo [STEP 3] Ejecting the USB drive...
powershell.exe -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('%usb_drive%').InvokeVerb('Eject')"

echo.
echo =============================================================
echo  [SUCCESS] The USB drive can now be safely removed.
echo  The installation will continue in a new window.
echo =============================================================
echo.

pause
exit