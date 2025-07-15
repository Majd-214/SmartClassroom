@echo off
setlocal

:: =================================================================
:: USB-Setup.bat (Final Version with Window Focus)
:: =================================================================

set "desktop_script_name=Desktop-Installer.bat"
set "desktop_path=%USERPROFILE%\Desktop"
set "usb_drive=%~d0"
set "window_focus_script=%TEMP%\setfocus.vbs"

cls
echo.
echo [INFO] Initializing McMaster Smart Classroom setup...
echo.

:: --- Step 1: Copy the hidden installer ---
echo [ACTION] Copying setup files...
copy "%~dp0\%desktop_script_name%" "%desktop_path%\" > nul
if %errorlevel% neq 0 (
    echo [ERROR] Could not copy installer. Please try again.
    pause
    exit /b
)

:: --- Step 2: Create a VBScript to manage window focus ---
(
    echo Set WshShell = WScript.CreateObject("WScript.Shell"^)
    echo WScript.Sleep 500
    echo WshShell.AppActivate "McMaster USB Setup"
) > "%window_focus_script%"

:: --- Step 3: Launch the main installer and run the focus script ---
title McMaster USB Setup
echo [ACTION] Launching the main installer...
start "McMaster Smart Classroom Installer" /D "%desktop_path%" cmd /c ""%desktop_path%\%desktop_script_name%""

:: Use the VBScript to pull this window back to the front
cscript //nologo "%window_focus_script%"

:: --- Step 4: Attempt Eject & Wait for Removal ---
echo [ACTION] Attempting to eject the USB drive automatically...
powershell -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('%usb_drive%').InvokeVerb('Eject')"

cls
echo.
echo =======================================================================
echo.
echo  [SUCCESS] The installer is running in a separate window.
echo.
echo  [ACTION REQUIRED] Please UNPLUG the USB drive now.
echo.
echo  (This window will close automatically once the drive is removed).
echo.
echo =======================================================================

:waitForRemovalLoop
if exist %usb_drive%\ (
    timeout /t 2 /nobreak > nul
    goto :waitForRemovalLoop
)

:: --- Final cleanup ---
del "%window_focus_script%"
echo.
echo [INFO] USB drive successfully removed. This window will now close.
timeout /t 2 /nobreak > nul
exit