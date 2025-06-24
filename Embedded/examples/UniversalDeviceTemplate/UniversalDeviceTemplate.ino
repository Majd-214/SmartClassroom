// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>

using namespace SmartHome; // NAMESPACE COMMITMENT, DO NOT REMOVE THIS LINE

// 2. VARIABLE DEFINITIONS

// 3. SERVER CONFIGURATION
const char *DEVICE_NAME = "My_Station_Device";    // <-- CHANGE THIS!
const char *SOME_TOPIC = "classroom/some_device"; // <-- CHANGE THIS!

// 4. OBJECT DECLARATIONS
SmartDevice myDevice;

// 5. DEVICE FUNCTIONS
void setupDevice() // Runs ONCE at startup.
{
  // ---> INITIALIZE YOUR DEVICE HARDWARE HERE <---

  myDevice.subscribeTo(SOME_TOPIC); // Subscribe to the topics
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
|   |    --->>    DO NOT EDIT THE CODE BELOW!    <<---    |   |
|   |            (This is the system's engine)            |   |
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

  // Initialize the SmartDevice library and connect to the network.
  myDevice.begin(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP);

  // Register the system-level message handler.
  myDevice.onMessage(system_onMessage);

  // Call your custom hardware setup function
  setupDevice();
  Serial.println("System Engine: Custom hardware setup complete.");

  Serial.println("System Engine: Boot sequence complete. System is online.");
}

// Main loop that keeps the system running.
void loop()
{
  myDevice.update(); // MUST be here to keep connection alive.
  readSensor();      // Call your sensor reading and publishing logic.
  delay(1000);       // A 1-second delay to prevent spamming the network. Adjust if needed.
}