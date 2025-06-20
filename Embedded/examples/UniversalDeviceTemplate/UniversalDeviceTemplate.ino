// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h> // This now includes the new structs and helpers

using namespace SmartHome;

// 2. VARIABLE DEFINITIONS

// 3. CONFIGURATION
const char *DEVICE_NAME = "My_Station_Device";    // <-- CHANGE THIS!
const char *BASE_TOPIC = "classroom/some_device"; // <-- UPDATE THIS! This single topic handles both commands and status

// 4. DECLARATIONS
SmartDevice myDevice;

void setupDevice() // Runs ONCE at startup.
{
  // ---> INITIALIZE YOUR DEVICE HARDWARE HERE <---
}

void readSensor() // Runs REPEATEDLY in the main loop.
{
  // ---> READ YOUR SENSORS AND PUBLISH DATA HERE <---
}

void triggerActuator(String topic, String command) // Runs ON_DEMAND when a command is received from the Smart Hub.
{
  // ---> PROCESS INCOMING COMMANDS AND CONTROL ACTUATORS HERE <---
}

/*
===============================================================
|                                                             |
|   +-----------------------------------------------------+   |
|   |                                                     |   |
|   |   --->>    DO NOT EDIT THE CODE BELOW!    <<---     |   |
|   |           (This is the system's engine)             |   |
|   |                                                     |   |
|   +-----------------------------------------------------+   |
|                                                             |
===============================================================
*/

// ---- System Engine Below ----

const char *WIFI_SSID = "McMasterIoT-Camp";
const char *WIFI_PASSWORD = "Roomba2025";
const char *MQTT_BROKER_IP = "192.168.0.147";

// System-level message handler that calls your actuator logic.
void system_onMessage(String topic, String payload) // Signature matches MessageCallback
{
  Serial.print("System received command on topic: ");
  Serial.print(topic);
  Serial.print(". Payload: ");
  Serial.println(payload);
  triggerActuator(topic, payload); // Calls your function above with topic and payload
}

// Main setup function that initializes the entire system.
void setup()
{
  Serial.begin(115200);
  Serial.println("System Engine: Booting device...");

  // Call your custom hardware setup function
  setupDevice();
  Serial.println("System Engine: Custom hardware setup complete.");

  // Initialize the SmartDevice library and connect to the network.
  myDevice.begin(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP);

  // Register the system-level message handler.
  myDevice.onMessage(system_onMessage);

  // Subscribe to the base topic to receive commands for this device.
  myDevice.subscribeTo(BASE_TOPIC); // Now subscribing to the base topic for commands
  Serial.println("System Engine: Boot sequence complete. System is online.");
}

// Main loop that keeps the system running.
void loop()
{
  myDevice.update(); // MUST be here to keep connection alive.
  readSensor();      // Call your sensor reading and publishing logic.
  delay(1000);       // A 1-second delay to prevent spamming the network. Adjust if needed.
}