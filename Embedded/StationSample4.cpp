// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>
#include <Servo.h>

using namespace SmartHome; // NAMESPACE COMMITMENT, DO NOT REMOVE THIS LINE

// 2. VARIABLE DEFINITIONS
const int KEYPAD_PIN = A0;     // To receive State from Keypad
const int LOCK_1_PIN = PIN_D5; // First Servo Lock Pin
const int LOCK_2_PIN = PIN_D6; // Second Servo Lock Pin
const int DOOR_PIN = PIN_D7;   // Contact Sensor Pin
bool local = false;   // Flag to track if the command came from the keypad

// 3. SERVER CONFIGURATION
const char *DEVICE_NAME = "Fort_Knox"; // <-- CHANGE THIS!
const char *LOCK_TOPIC = "entrance/lock";
const char *DOOR_TOPIC = "entrance/door";

// 4. OBJECT DECLARATIONS
SmartDevice myDevice;
Servo lock1;
Servo lock2;
enum KeypadState { STANDBY, LOCK, UNLOCK } request;

// 5. DEVICE FUNCTIONS
void setupDevice() // Runs ONCE at startup.
{
  // Set keypad and contact sensor as inputs
  pinMode(KEYPAD_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP); // Use internal pull-up resistor for contact sensor

  // Attach servos to their respective pins
  lock1.attach(LOCK_1_PIN);
  lock2.attach(LOCK_2_PIN);

  // Subscribe to the lock topic to receive commands
  myDevice.subscribeTo(LOCK_TOPIC);
}

// NEW Function to set the lock state as locked[true] or unlocked[false].
void setLock(bool state, bool fromKeypad)
{
  lock1.write(state ? 180 : 0); // Lock or unlock the first servo
  lock2.write(state ? 0 : 180); // Mirror first servo because its position is inverted

  if (!fromKeypad) return; // Exit if not from keypad

  myDevice.publishTo(DOOR_TOPIC, state ? "true" : "false");
  this.local = true;
}

void readSensor() // Runs REPEATEDLY in the main loop.
{
  // Read door status from the contact sensor
  bool doorClosed = !digitalRead(DOOR_PIN); // Inverted because the sensor is active LOW

  // Publish the door status to the MQTT topic
  myDevice.publishTo(DOOR_TOPIC, doorClosed ? "true" : "false");

  // Read keypad
  request = map(analogRead(KEYPAD_PIN), 0, 1023, 0, 2);
  if (request != STANDBY) setLock(request == LOCK, true); // Lock or unlock based on keypad input and inform server
}

void triggerActuator(String topic, String command) // Runs ON_DEMAND when a command is received from the Smart Hub.
{
  if (topic == LOCK_TOPIC)
    if (local) local = false; // Reset Flag for next use
    else setLock(command == "true", false); // Lock or unlock based on command, do not inform server
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