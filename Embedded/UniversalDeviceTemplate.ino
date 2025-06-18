/*
 =================================================================
  McMaster Eng Camp - IoT Smart Classroom (Universal Template)
 =================================================================
  Welcome! This is your starting point for ALL stations.
  Your only job is to add code in the designated sections below.
*/

// Our custom library that handles all networking.
#include <SmartDevice.h>

// ===============================================================
// 1. CONFIGURE YOUR DEVICE
// ===============================================================
// Give your device a unique name to identify it on the network.
const char* DEVICE_NAME = "My_Station_Device"; // <-- CHANGE THIS!

// These "topics" are like addresses for sending and receiving messages.
// Your instructor will provide the correct topics for your station from the config.yaml.
const char* COMMAND_TOPIC = "classroom/some_device/command"; // <-- CHANGE for your station
const char* STATUS_TOPIC = "classroom/some_device/status";   // <-- CHANGE for your station

// Create an object for our library.
SmartDevice myDevice;

// This function is for setting up your hardware (e.g., pins, sensors). It runs once.
void setupMyHardware() {
  // EXAMPLE: pinMode(PIN_D7, OUTPUT);
}

// This function runs over and over. It's perfect for SENSORS that need to
// constantly check for new data and publish it to the cloud.
void loopMySensorLogic() {
  // ---> SENSOR LOGIC GOES HERE <---
  // EXAMPLE:
  // float temp = myTempSensor.read();
  // myDevice.publishTo(STATUS_TOPIC, String(temp));
}

// This function ONLY runs when a command is received from the cloud. It's perfect
// for ACTUATORS (lights, motors, etc.) that need to react to commands.
void handleMyActuatorLogic(String command) {
  // ---> ACTUATOR LOGIC GOES HERE <---
  // EXAMPLE:
  // if (command == "ON") {
  //   digitalWrite(PIN_D7, HIGH);
  //   myDevice.publishTo(STATUS_TOPIC, "ON"); // Always report back your status!
  // }
}


/*
===============================================================
|                                                             |
|   +-----------------------------------------------------+   |
|   |                                                     |   |
|   |   --->>    DO NOT EDIT THE CODE BELOW!    <<---      |   |
|   |         (This is the system's engine)               |   |
|   |                                                     |   |
|   +-----------------------------------------------------+   |
|                                                             |
===============================================================
*/

// ---- System Engine Below ----

const char* WIFI_SSID = "McMasterIoT-Camp";
const char* WIFI_PASSWORD = "Roomba2025";
const char* MQTT_BROKER_IP = "192.168.0.147";

// System-level message handler that calls your actuator logic.
void system_onMessage(String topic, String payload) {
  Serial.print("System received command: ");
  Serial.println(payload);
  handleMyActuatorLogic(payload); // Calls your function above
}

// Main setup function that initializes the entire system.
void setup() {
  Serial.begin(115200);
  Serial.println("System Engine: Booting device...");

  // Call your custom hardware setup function
  setupMyHardware();
  Serial.println("System Engine: Custom hardware setup complete.");

  // Initialize the SmartDevice library and connect to the network.
  myDevice.begin(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP);

  // Register the system-level message handler.
  myDevice.onMessage(system_onMessage);

  // Subscribe to the command topic to receive instructions.
  myDevice.subscribeTo(COMMAND_TOPIC);
  Serial.println("System Engine: Boot sequence complete. System is online.");
}

// Main loop that keeps the system running.
void loop() {
  myDevice.update(); // MUST be here to keep connection alive.
  loopMySensorLogic(); // Calls your custom sensor loop function.
  delay(1000); // A 1-second delay to prevent spamming the network. Adjust if needed.
}