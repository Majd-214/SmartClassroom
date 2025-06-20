/*
====================================================================
  The Library Header File (SmartDevice.h) - Version 3.1 (Robust Subscriptions)
====================================================================
  - This version adds internal handling for re-subscribing to topics
    after an MQTT client reconnection, ensuring reliable message reception.
*/

#ifndef SmartDevice_h
#define SmartDevice_h

#include "Arduino.h"
#include <vector> // Required for std::vector<String>

// ===============================================================
//  Safe Nodemcu ESP8266 GPIO Pin Definitions
// ===============================================================
// Use these friendly pin names in your sketch to avoid using essential pins.
#define PIN_D1 5
#define PIN_D2 4
#define PIN_D5 14
#define PIN_D6 12
#define PIN_D7 13

// ===============================================================
//  Type Definition for the Universal Message Callback
// ===============================================================
// Your sketch will use this to react to any message from the cloud.
// It receives the topic the message came on and the message content (payload).
typedef void (*MessageCallback)(String topic, String payload);

// ===============================================================
//  SmartDevice Class Definition
// ===============================================================
class SmartDevice
{
public:
  SmartDevice();

  // Initializes the device and connects to Wi-Fi and the Hub.
  void begin(const char* deviceName, const char* wifi_ssid, const char* wifi_pass, const char* mqtt_broker);

  // Keeps the device connected and processes incoming messages. MUST be called in every loop().
  void update();

  // Publishes a message to a specific topic path.
  void publishTo(String fullTopic, String payload);

  // Subscribes to a specific topic path to listen for messages.
  // This version also stores the subscription for automatic re-subscription on reconnect.
  void subscribeTo(String fullTopic);

  // Registers a function to handle all incoming messages.
  void onMessage(MessageCallback callback);

private:
  const char* _deviceName;
  void reconnect(); // Handles MQTT client reconnection.
  
  // New: Stores a list of topics this device is currently subscribed to.
  std::vector<String> _subscribedTopics; 
  // New: Private helper to re-subscribe to all stored topics after a connection is re-established.
  void _resubscribe(); 
};

#endif
