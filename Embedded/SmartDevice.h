/*
====================================================================
  Smart Device Library Header (SmartDevice.h) - Version 2.4.0
====================================================================
*/
#ifndef SmartDevice_h
#define SmartDevice_h

#include <Arduino.h>
#include <ArduinoJson.h>       // Make sure this is included for DynamicJsonDocument
#include <Adafruit_NeoPixel.h> // Include if getRGB will be a public method using it

//  Safe ESP8266 GPIO Pin Definitions
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

// Define your Light struct within the SmartHome namespace
namespace SmartHome
{
  enum LightType
  {
    DIMMABLE,
    COLOR
  };

  struct Light
  {
    bool isOn;
    int brightness; // 0-100
    int hue;        // 0-360
    int saturation; // 0-100
    LightType type;
  };
}

typedef void (*MessageCallback)(String topic, String payload);

class SmartDevice
{
public:
  SmartDevice();
  void begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker);
  void update();
  void publishTo(String fullTopic, String payload);
  void subscribeTo(String fullTopic);
  void onMessage(MessageCallback callback);
  bool isConnected();

  // Static helper function to parse command string into Light struct
  static SmartHome::Light commandToLight(String command);

  // NEW: Abstraction helper for RGB conversion
  static uint32_t getRGB(SmartHome::Light lightCommand); // For color/dimmable lights
  // NEW: Abstraction helper for getting raw brightness (0-255)
  static uint8_t getBrightnessValue(SmartHome::Light lightCommand);

private:
  const char *_deviceName;
  void _reconnect();
  void _resubscribe();
  std::vector<String> _subscribedTopics;
};

#endif