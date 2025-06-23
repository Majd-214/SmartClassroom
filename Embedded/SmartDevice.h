/*
====================================================================
  The Library Header (SmartDevice.h) - Version 7.0 (Modified for JSON)
====================================================================
*/

#ifndef SMARTDEVICE_H
#define SMARTDEVICE_H

#include <Arduino.h> // For String, Serial, etc.
#include <vector>    // For std::vector
#include <ArduinoJson.h> // IMPORTANT: Include the ArduinoJson library!

//  Safe Nodemcu ESP8266 GPIO Pin Definitions
// ===============================================================
// Use these friendly pin names in your sketch to avoid using essential pins.
#define PIN_D0 16 // GPIO ~ HIGH at boot, used to wake up from deep sleep
#define PIN_D1 5  // GPIO - Safe to use
#define PIN_D2 4  // GPIO - Safe to use
#define PIN_D3 0  // GPIO ~ Connected to Flash button, boot fails is pulled LOW
#define PIN_D4 2  // GPIO ~ HIGH at boot, boot fails if pulled LOW
#define PIN_D5 14 // GPIO - Safe to use
#define PIN_D6 12 // GPIO - Safe to use
#define PIN_D7 13 // GPIO - Safe to use
#define PIN_D8 15 // GPIO ~ Required for boot, boot fails if pulled HIGH

// Define a type for the message callback function
// This function will be called when an MQTT message is received
using MessageCallback = void (*)(String topic, String message);

// Declare the global callback function. This will be set by SmartDevice::onMessage
extern MessageCallback global_message_callback;

// --- MQTT Callback Function (Forward Declaration) ---
void mqttCallback(char *topic, byte *payload, unsigned int length);

// --- SmartHome Namespace for Light Struct ---
namespace SmartHome {

// Enum to define light types
enum LightType {
  DIMMABLE, // Only brightness control
  COLOR     // Brightness, Hue, Saturation control
};

// Struct to represent a light's state and type
struct Light {
  bool isOn;
  int brightness; // 0-100
  int hue;        // 0-360
  int saturation; // 0-100
  LightType type;

  // Method to generate a JSON payload string from the Light's state
  // IMPORTANT: Removed 'const' qualifier to allow modification of the JsonDocument
  String payload() {
    // Use DynamicJsonDocument for flexibility.
    // 128 bytes should be ample for a light's state (swi, bri, hue, sat).
    DynamicJsonDocument doc(128);

    doc["swi"] = isOn;
    doc["bri"] = brightness;

    if (type == COLOR) {
      doc["hue"] = hue;
      doc["sat"] = saturation;
    }

    String output;
    // serializeJson writes the JSON object to the output String
    serializeJson(doc, output);
    return output;
  }
};

} // namespace SmartHome


// --- SmartDevice Class Definition ---
class SmartDevice {
public:
  // Constructor
  SmartDevice();

  // Initialize the device (Wi-Fi, MQTT)
  void begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker);

  // Call this in your Arduino loop() function to maintain connection and process messages
  void update();

  // Publish a message to an MQTT topic
  void publishTo(String fullTopic, String payload);

  // Subscribe to an MQTT topic
  void subscribeTo(String fullTopic);

  // Register a callback function to handle incoming MQTT messages
  void onMessage(MessageCallback callback);

  // Check if MQTT client is connected
  bool isConnected();

  // Static helper function to convert an incoming command string (now JSON) to a Light struct
  static SmartHome::Light commandToLight(String command);

private:
  const char *_deviceName;
  std::vector<String> _subscribedTopics;

  // Private helper to reconnect to MQTT broker
  void _reconnect();
  // Private helper to resubscribe to topics after reconnection
  void _resubscribe();
};

#endif // SMARTDEVICE_H