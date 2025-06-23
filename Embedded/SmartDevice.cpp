/*
====================================================================
  The Library Implementation (SmartDevice.cpp) - Version 7.0 (Modified for JSON)
====================================================================
*/

#include "SmartDevice.h" // Assuming SmartDevice.h now includes ArduinoJson.h and defines SmartHome::Light with payload()

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <ArduinoJson.h> // Ensure ArduinoJson library is installed and included

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
//  Parses an incoming raw command string (now JSON) into a friendly Light struct.
// ===============================================================

SmartHome::Light SmartDevice::commandToLight(String command)
{
  SmartHome::Light light;

  // Default values
  light.isOn = false;
  light.brightness = 0;
  light.hue = 0;
  light.saturation = 0;
  light.type = SmartHome::DIMMABLE; // Assume dimmable unless color keys are found

  // Use DynamicJsonDocument to parse the incoming command string
  // A capacity of 256 bytes should be sufficient for these light commands
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, command);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    // Return a default or error state light if parsing fails
    return light;
  }

  // Parse ON/OFF state
  // Check for "swi" (Arduino Cloud naming convention) or "SWITCH" (your original)
  if (doc.containsKey("swi")) {
    light.isOn = doc["swi"].as<bool>();
  } else if (doc.containsKey("SWITCH")) {
    light.isOn = doc["SWITCH"].as<String>().equalsIgnoreCase("ON") || doc["SWITCH"].as<String>().equalsIgnoreCase("true");
  }


  // Check for Color parameters first
  bool hasHue = doc.containsKey("hue");
  bool hasSaturation = doc.containsKey("sat");

  if (hasHue || hasSaturation)
  { // If Hue or Saturation is explicitly in the command, it's a COLOR light
    light.type = SmartHome::COLOR;
    light.hue = doc["hue"].as<int>();
    light.saturation = doc["sat"].as<int>();
    light.brightness = doc["bri"].as<int>(); // Brightness (V) for color comes from 'bri' key

    // Ensure HSV values are within their valid ranges
    light.hue = constrain(light.hue, 0, 360);
    light.saturation = constrain(light.saturation, 0, 100);
    light.brightness = constrain(light.brightness, 0, 100);
  }
  else
  {
    // If no color parameters, treat as DIMMABLE
    light.type = SmartHome::DIMMABLE;
    // Brightness for dimmable also comes from 'bri' key
    if (doc.containsKey("bri")) {
        light.brightness = doc["bri"].as<int>();
    } else {
        light.brightness = 0; // Default brightness if not found
    }
    light.brightness = constrain(light.brightness, 0, 100); // Ensure brightness is within 0-100
  }

  return light;
}