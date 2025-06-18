// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>

// 2. VARIABLE DEFINITIONS


// 3. CONFIGURATION
const char* DEVICE_NAME = "My_Station_Device"; // <-- CHANGE THIS!
const char* COMMAND_TOPIC = "classroom/some_device/command"; // <-- UPDATE THIS!
const char* STATUS_TOPIC = "classroom/some_device/status";   // <-- UPDATE THIS!

// 4. DECLARATIONS
SmartDevice myDevice;


void setupMyHardware() { // Runs ONCE
  // ---> SETUP LOGIC GOES HERE <---

}

void loopMySensorLogic() { // Runs REPEATEDLY
  // ---> SENSOR LOGIC GOES HERE <---

}

void handleMyActuatorLogic(String command) { // Executes ON_DEMAND
  // ---> ACTUATOR LOGIC GOES HERE <---

}


/*
===============================================================
|                                                             |
|   +-----------------------------------------------------+   |
|   |                                                     |   |
|   |   --->>    DO NOT EDIT THE CODE BELOW!    <<---     |   |
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
