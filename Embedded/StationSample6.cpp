// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>

using namespace SmartHome; // NAMESPACE COMMITMENT, DO NOT REMOVE THIS LINE

// 2. VARIABLE DEFINITIONS
const int *BUTTON_PIN = {PIN_D1, PIN_D2, PIN_D3, PIN_D4}; // Button pins for the curtain control
const int *LED_PIN = {PIN_D5, PIN_D6, PIN_D7, PIN_D8};

// 3. SERVER CONFIGURATION
const char *DEVICE_NAME = "Remote_Detonators"; // <-- CHANGE THIS!
const char *LIGHT_TOPIC = "classroom/lights";
const char *KETTLE_TOPIC = "classroom/kettle";
const char *PORCH_TOPIC = "porch/light";
const char *HVAC_TOPIC = "classroom/hvac";

// 4. OBJECT DECLARATIONS
SmartDevice myDevice;

// 5. DEVICE FUNCTIONS
void setupDevice() // Runs ONCE at startup.
{
  for (int i = 0; i < 4; i++) // Initialize button pins
  {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP); // Set button pins as input with pull-up resistor
    pinMode(LED_PIN[i], OUTPUT);          // Set LED pins as output
  }
  myDevice.subscribeTo(LIGHT_TOPIC);  // Subscribe to the light topic
  myDevice.subscribeTo(KETTLE_TOPIC); // Subscribe to the kettle topic
  myDevice.subscribeTo(PORCH_TOPIC);  // Subscribe to the porch light topic
  myDevice.subscribeTo(HVAC_TOPIC);   // Subscribe to the HVAC topic
}

void readSensor() // Runs REPEATEDLY in the main loop.
{
  myDevice.publish(LIGHT_TOPIC, digitalRead(BUTTON_PIN[0]) ? "true" : "false");  // Publish the state of the first button
  myDevice.publish(KETTLE_TOPIC, digitalRead(BUTTON_PIN[1]) ? "true" : "false"); // Publish the state of the second button
  myDevice.publish(PORCH_TOPIC, digitalRead(BUTTON_PIN[2]) ? "true" : "false");  // Publish the state of the third button
  myDevice.publish(HVAC_TOPIC, digitalRead(BUTTON_PIN[3]) ? "true" : "false");   // Publish the state of the fourth button
}

void triggerActuator(String topic, String command) // Runs ON_DEMAND when a command is received from the Smart Hub.
{
  if (topic == LIGHT_TOPIC)
    digitalWrite(LED_PIN[0], command == "true");
  else if (topic == KETTLE_TOPIC)
    digitalWrite(LED_PIN[1], command == "true");
  else if (topic == PORCH_TOPIC)
    digitalWrite(LED_PIN[2], command == "true");
  else if (topic == HVAC_TOPIC)
    digitalWrite(LED_PIN[3], command == "true");
  else
    Serial.println("Unknown topic: " + topic); // Handle unknown topics gracefully
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