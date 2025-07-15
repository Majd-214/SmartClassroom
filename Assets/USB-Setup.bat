@echo off
setlocal

:: =================================================================
:: USB-Setup.bat
:: =================================================================

set "desktop_script_name=Desktop-Installer.bat"
set "desktop_path=%USERPROFILE%\Desktop"

echo.
echo [INFO] Starting the McMaster Smart Classroom setup...
echo.

:: Copy the desktop script from the USB to the user's desktop
echo [STEP 1] Preparing setup files...
copy "%~dp0\%desktop_script_name%" "%desktop_path%\%desktop_script_name%" > nul

:: Run the script that was just copied to the desktop
echo [STEP 2] Launching the main installer...
start "" /D "%desktop_path%" "%desktop_path%\%desktop_script_name%"

:: Eject the USB drive
echo [STEP 3] Ejecting the USB drive...
powershell.exe -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('%~d0').InvokeVerb('Eject')"

echo.
echo =============================================================
echo  [SUCCESS] The USB drive can now be safely removed.
echo =============================================================
echo.

pause
exit