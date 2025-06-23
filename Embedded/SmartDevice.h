/*
====================================================================
  The Library Header File (SmartDevice.h) - Version 7.0
====================================================================
*/

#ifndef SmartDevice_h
#define SmartDevice_h

#include "Arduino.h"
#include <vector>
#include <ArduinoJson.h>

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

//  Structs for IoT Variable Types (No Objects, Only Data Structures)
// ===============================================================
namespace SmartHome
{ // All new structs will live in this namespace

  // Defines the type of light being represented (dimmable white or full color)
  enum LightType
  {
    DIMMABLE,
    COLOR
  };

  // Represents the full state of a light device.
  struct Light
  {
    bool isOn;      // true for ON, false for OFF
    int brightness; // 0-100% (This is also the 'V' component for COLOR)
    int hue;        // 0-360 degrees (only relevant if type is COLOR)
    int saturation; // 0-100% (only relevant if type is COLOR)
    LightType type; // Indicates if this Light struct represents a DIMMABLE or COLOR light

    // Method to generate the string payload for this Light state
    // This generates JSON to be 100% compatible with bridge.py status expectations.
    String payload() const
    {
      // Determine the buffer size based on whether it's a color light or just dimmable
      const size_t CAPACITY = (type == COLOR)
                                  ? JSON_OBJECT_SIZE(4)  // swi, bri, hue, sat
                                  : JSON_OBJECT_SIZE(2); // swi, bri

      StaticJsonDocument<CAPACITY> doc;

      doc["swi"] = isOn;       // "swi" for switch (on/off)
      doc["bri"] = brightness; // "bri" for brightness (0-100)

      if (type == COLOR)
      {
        doc["hue"] = hue;        // "hue" for hue (0-360)
        doc["sat"] = saturation; // "sat" for saturation (0-100)
      }

      String output;
      serializeJson(doc, output);
      return output;
    }
  };

} // End namespace SmartHome

//  Type Definition for the Universal Message Callback
// ===============================================================
// Your sketch will use this to react to any message from the cloud.
// It receives the topic the message came on and the message content (payload).
typedef void (*MessageCallback)(String topic, String payload);

//  SmartDevice Class Definition
// ===============================================================
class SmartDevice
{
public:
  SmartDevice();

  // Initializes the device and connects to Wi-Fi and the Hub.
  void begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker);

  // Keeps the device connected and processes incoming messages. MUST be called in every loop().
  void update();

  // Publishes a raw string message to a specific topic path.
  // For Light messages, use Light.payload() as the 'payload' parameter.
  void publishTo(String fullTopic, String payload);

  // Subscribes to a specific topic path to listen for messages.
  // This version also stores the subscription for automatic re-subscription on reconnect.
  void subscribeTo(String fullTopic);

  // Registers a function to handle all incoming messages.
  void onMessage(MessageCallback callback);

  // Checks if the MQTT client is currently connected.
  bool isConnected();

  //  Command Parsing Helper Function (Static - accessible via SmartDevice::)
  //  Parses an incoming raw command string into a friendly Light struct.
  // ===============================================================
  static SmartHome::Light commandToLight(String command);

private:
  const char *_deviceName;
  void _reconnect(); // Private method with underscore

  std::vector<String> _subscribedTopics;
  void _resubscribe(); // Private method with underscore
};

#endif