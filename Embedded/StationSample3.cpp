// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>
#include <Adafruit_NeoPixel.h>

using namespace SmartHome; // NAMESPACE COMMITMENT, DO NOT REMOVE THIS LINE

// 2. VARIABLE DEFINITIONS
const int STRIP_PIN = PIN_D5;
const int NUM_LEDS = 5;
const int BULB_PIN = PIN_D6;

// 3. SERVER CONFIGURATION
const char *DEVICE_NAME = "Glowing_Emu"; // <-- CHANGE THIS!
const char *STRIP_TOPIC = "classroom/lights";
const char *BULB_TOPIC = "porch/light";

// 4. OBJECT DECLARATIONS
SmartDevice myDevice;
Adafruit_NeoPixel strip(NUM_LEDS, STRIP_PIN, NEO_GRB + NEO_KHZ800); // NeoPixel strip object

// 5. DEVICE FUNCTIONS
void setupDevice() // Runs ONCE at startup.
{
  strip.begin(); // Initialize the NeoPixel strip
  strip.show();  // Initialize all pixels to 'off'

  pinMode(BULB_PIN, OUTPUT);   // Set the bulb pin as output
  digitalWrite(BULB_PIN, LOW); // Ensure the bulb is off at startup

  // Subscribe to the topics for the strip and bulb to receive commands
  myDevice.subscribeTo(STRIP_TOPIC);
  myDevice.subscribeTo(BULB_TOPIC);
}

void readSensor() {} // Runs REPEATEDLY in the main loop.

void triggerActuator(String topic, String command) // Runs ON_DEMAND when a command is received from the Smart Hub.
{
  // Create Light struct to hold light data
  Light light = SmartDevice::commandToLight(command);

  if (topic == STRIP_TOPIC)
  {
    strip.fill(SmartDevice::getRGB(light)); // Convert Light to RGB and fill the strip
    strip.show();                           // Update the strip with the new color
  }

  else if (topic == BULB_TOPIC) {
    if (light.isOn)                                                  // Check if the light is meant to be on
      analogWrite(BULB_PIN, SmartDevice::getBrightnessValue(light)); // Set bulb brightness
    else
      digitalWrite(BULB_PIN, LOW); // Turn off the bulb if not on
  }
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