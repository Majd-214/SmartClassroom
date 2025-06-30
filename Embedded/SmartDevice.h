/**
 * @file SmartDevice.h
 * @brief Smart Device Library for IoT applications.
 * This library provides functionalities for connecting to Wi-Fi, MQTT, and controlling smart home devices.
 * It includes support for light control, device management, and message handling.
 * @version 2.5.0
 * @date 2025-06-30
 *
====================================================================
  Smart Device Library Header (SmartDevice.h) - Version 2.5.1
====================================================================
*/
#ifndef SmartDevice_h
#define SmartDevice_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

/**
 * @brief Pin definitions for the smart device.
 * These pins are used for various functionalities such as LED control, button inputs, and other peripherals
 */
#define PIN_D0 16
#define PIN_D1 5
#define PIN_D2 4
#define PIN_D3 0
#define PIN_D4 2
#define PIN_D5 14
#define PIN_D6 12
#define PIN_D7 13
#define PIN_D8 15

// Namespace for smart home data types
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

// Callback function signature for handling incoming messages
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

  // Static helper functions for simplified light control
  static SmartHome::Light commandToLight(String command);
  static uint32_t getRGB(SmartHome::Light lightCommand);
  static uint8_t getBrightnessValue(SmartHome::Light lightCommand);

private:
  const char *_deviceName;
  void _reconnect();
  void _resubscribe();
  std::vector<String> _subscribedTopics;
};

#endif
