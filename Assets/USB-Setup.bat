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
[cite_start]echo [INFO] Initializing McMaster Smart Classroom setup... [cite: 21]
echo.

:: --- Step 1: Copy the hidden installer ---
[cite_start]echo [ACTION] Copying setup files... [cite: 21]
xcopy "%~dp0\%desktop_script_name%" "%desktop_path%\" /H /Y /I > nul
if %errorlevel% neq 0 (
    echo [ERROR] Could not copy installer. Please try again. Window will close in 10 seconds.
    timeout /t 10 > nul
    exit /b
)

:: --- Step 2: Create a VBScript to manage window focus ---
(
    echo Set WshShell = WScript.CreateObject("WScript.Shell"^)
    echo WScript.Sleep 500
    echo WshShell.AppActivate "McMaster USB Setup"
[cite_start]) > "%window_focus_script%" [cite: 22]

:: --- Step 3: Launch the main installer and run the focus script ---
title McMaster USB Setup
[cite_start]echo [ACTION] Launching the main installer... [cite: 22]
[cite_start]start "McMaster Smart Classroom Installer" /D "%desktop_path%" cmd /c ""%desktop_path%\%desktop_script_name%"" [cite: 22]

:: Use the VBScript to pull this window back to the front
[cite_start]cscript //nologo "%window_focus_script%" [cite: 22]

:: --- Step 4: Attempt Eject & Wait for Removal ---
[cite_start]echo [ACTION] Attempting to eject the USB drive automatically... [cite: 23]
[cite_start]powershell -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('%usb_drive%').InvokeVerb('Eject')" [cite: 23]

cls
echo.
[cite_start]echo ======================================================================= [cite: 23]
echo.
[cite_start]echo  [SUCCESS] The installer is running in a separate window. [cite: 23]
echo.
[cite_start]echo  [ACTION REQUIRED] Please UNPLUG the USB drive now. [cite: 24]
echo.
[cite_start]echo  (This window will close automatically once the drive is removed). [cite: 25]
echo.
[cite_start]echo ======================================================================= [cite: 26]

:waitForRemovalLoop
if exist %usb_drive%\ (
    timeout /t 2 /nobreak > nul
    goto :waitForRemovalLoop
[cite_start]) [cite: 26]

:: --- Final cleanup ---
[cite_start]del "%window_focus_script%" [cite: 27]
echo.
echo [INFO] USB drive successfully removed. [cite_start]This window will now close. [cite: 27]
timeout /t 2 /nobreak > nul
exit