/*
====================================================================
  The Library Implementation (SmartDevice.cpp) - Version 7.0
====================================================================
*/

#include "SmartDevice.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>

// --- Global objects ---
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MessageCallback global_message_callback = nullptr; // The one and only callback

// --- Global MQTT Callback Function ---
// This function now calls the single, universal message handler.
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.printf("Message arrived on topic: %s. Payload: %s\n", topic, message.c_str());

  if (global_message_callback != nullptr)
  {
    global_message_callback(String(topic), message);
  }
}

// --- SmartDevice Class Method Implementations ---

SmartDevice::SmartDevice()
{
  _subscribedTopics.clear();
}

void SmartDevice::begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker)
{
  _deviceName = deviceName;

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT Client
  mqttClient.setServer(mqtt_broker, 1883);
  mqttClient.setCallback(mqttCallback); // Register the global callback function.
}

void SmartDevice::_reconnect() // Private method with underscore
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(_deviceName))
    {
      Serial.println("connected!");
      _resubscribe(); // Calls private method with underscore
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void SmartDevice::update()
{
  if (!mqttClient.connected())
  {
    _reconnect(); // Calls private method with underscore
  }
  mqttClient.loop(); // Process incoming MQTT messages and maintain connection.
}

void SmartDevice::publishTo(String fullTopic, String payload)
{
  if (mqttClient.connected())
  {
    mqttClient.publish(fullTopic.c_str(), payload.c_str());
    Serial.printf("Published to %s: %s\n", fullTopic.c_str(), payload.c_str());
  }
  else
  {
    Serial.println("MQTT client not connected, cannot publish.");
  }
}

void SmartDevice::subscribeTo(String fullTopic)
{
  bool found = false;
  for (const String &topic : _subscribedTopics)
  {
    if (topic == fullTopic)
    {
      found = true;
      break;
    }
  }

  if (!found)
  {
    _subscribedTopics.push_back(fullTopic);
  }

  if (mqttClient.connected())
  {
    mqttClient.subscribe(fullTopic.c_str());
    Serial.printf("Subscribed to topic: %s\n", fullTopic.c_str());
  }
  else
  {
    Serial.println("MQTT client not connected, added to queue for re-subscription on connect.");
  }
}

void SmartDevice::_resubscribe() // Private method with underscore
{
  Serial.println("Re-subscribing to topics...");
  if (_subscribedTopics.empty())
  {
    Serial.println("No topics to re-subscribe to.");
    return;
  }
  for (const String &topic : _subscribedTopics)
  {
    if (mqttClient.subscribe(topic.c_str()))
    {
      Serial.printf("  Successfully re-subscribed to: %s\n", topic.c_str());
    }
    else
    {
      Serial.printf("  Failed to re-subscribe to: %s (MQTT rc: %d)\n", topic.c_str(), mqttClient.state());
    }
  }
}

void SmartDevice::onMessage(MessageCallback callback)
{
  global_message_callback = callback;
}

bool SmartDevice::isConnected()
{
  return mqttClient.connected();
}

// ===============================================================
//  Static Command Parsing Helper Function
//  Parses an incoming raw command string into a friendly Light struct.
// ===============================================================

// Helper to parse key-value pairs from command string (e.g., "KEY=VALUE")
String getValueForKey(String data, String key, char separator = ',')
{
  int keyIndex = data.indexOf(key + "=");
  if (keyIndex == -1)
    return ""; // Key not found

  int valueStart = keyIndex + key.length() + 1;
  int valueEnd = data.indexOf(separator, valueStart);

  if (valueEnd == -1)
  { // If separator not found, it's the last value
    return data.substring(valueStart);
  }
  else
  {
    return data.substring(valueStart, valueEnd);
  }
}

SmartHome::Light SmartDevice::commandToLight(String command)
{ // Renamed static method
  SmartHome::Light light;

  // Default values
  light.isOn = false;
  light.brightness = 0;
  light.hue = 0;
  light.saturation = 0;
  light.type = SmartHome::DIMMABLE; // Assume dimmable unless color keys are found

  // Parse ON/OFF state
  String switchVal = getValueForKey(command, "SWITCH");
  if (switchVal.isEmpty())
  { // Handle SWI for Python's JSON-like output (if it uses that)
    switchVal = getValueForKey(command, "SWI");
  }
  light.isOn = (switchVal.equalsIgnoreCase("ON") || switchVal.equalsIgnoreCase("true"));

  // Check for Color parameters first
  String hueVal = getValueForKey(command, "HUE");
  String satVal = getValueForKey(command, "SATURATION");
  String valVal = getValueForKey(command, "BRIGHTNESS"); // Python's 'BRIGHTNESS' maps to HSV 'V'

  if (!hueVal.isEmpty() || !satVal.isEmpty())
  { // If Hue or Saturation is explicitly in the command, it's a COLOR light
    light.type = SmartHome::COLOR;
    light.hue = hueVal.toInt();
    light.saturation = satVal.toInt();
    light.brightness = valVal.toInt(); // Brightness (V) for color comes from 'BRIGHTNESS' key

    // Ensure HSV values are within their valid ranges
    light.hue = constrain(light.hue, 0, 360);
    light.saturation = constrain(light.saturation, 0, 100);
    light.brightness = constrain(light.brightness, 0, 100);
  }
  else
  {
    // If no color parameters, treat as DIMMABLE
    light.type = SmartHome::DIMMABLE;
    // Brightness for dimmable also comes from 'BRIGHTNESS' key
    String brightnessVal = getValueForKey(command, "BRIGHTNESS");
    light.brightness = brightnessVal.toInt();
    if (light.brightness == 0 && (brightnessVal != "0" && brightnessVal != ""))
    {
      // If toInt() returned 0 but string wasn't "0" or empty, parsing error, default to 100
      light.brightness = 100;
    }
    light.brightness = constrain(light.brightness, 0, 100); // Ensure brightness is within 0-100
  }

  return light;
}