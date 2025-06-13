@echo off
title Mosquitto Broker (MANUAL MODE)
echo =================================================================
echo  Starting Mosquitto Broker in Manual Mode...
echo =================================================================
echo.
echo  IMPORTANT: Do NOT close this command window.
echo  Closing this window will immediately stop the MQTT server.
echo.

:: Navigate to the Mosquitto installation directory
cd /d "C:\Program Files\mosquitto"

:: Run Mosquitto with the verbose flag so you can see the output
mosquitto.exe -c mosquitto.conf -v

echo.
echo =================================================================
echo  Mosquitto has stopped.
echo =================================================================
pause