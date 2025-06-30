/*
====================================================================
  Smart Device Library Implementation (SmartDevice.cpp) - Version 2.5.1
====================================================================
*/

#include "SmartDevice.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>

// --- Global objects ---
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MessageCallback global_message_callback = nullptr;

// --- Global MQTT Callback Function ---
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
  // Constructor
}

void SmartDevice::begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker)
{
  _deviceName = deviceName;

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

  mqttClient.setServer(mqtt_broker, 1883);
  mqttClient.setCallback(mqttCallback);
}

void SmartDevice::_reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(_deviceName))
    {
      Serial.println("connected!");
      _resubscribe();
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
    _reconnect();
  }
  mqttClient.loop();
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
  _subscribedTopics.push_back(fullTopic);
  if (mqttClient.connected())
  {
    mqttClient.subscribe(fullTopic.c_str());
    Serial.printf("Subscribed to topic: %s\n", fullTopic.c_str());
  }
}

void SmartDevice::_resubscribe()
{
  Serial.println("Re-subscribing to topics...");
  for (const String &topic : _subscribedTopics)
  {
    if (mqttClient.subscribe(topic.c_str()))
    {
      Serial.printf("  Successfully re-subscribed to: %s\n", topic.c_str());
    }
    else
    {
      Serial.printf("  Failed to re-subscribe to: %s\n", topic.c_str());
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
//  Static Command Parsing Helper Function (JSON)
// ===============================================================
SmartHome::Light SmartDevice::commandToLight(String command)
{
  SmartHome::Light light;
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, command);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    light.isOn = false;
    return light;
  }

  light.isOn = doc["swi"] | false;

  if (!light.isOn)
  {
    light.brightness = 0;
    light.hue = 0;
    light.saturation = 0;
    return light;
  }

  // Check for color parameters to determine type
  if (doc.containsKey("hue") || doc.containsKey("sat"))
  {
    light.type = SmartHome::COLOR;
    light.brightness = doc["bri"] | 100;
    light.hue = doc["hue"] | 0;
    light.saturation = doc["sat"] | 100;
  }
  else
  {
    light.type = SmartHome::DIMMABLE;
    light.brightness = doc["bri"] | 100;
    light.hue = 0;
    light.saturation = 0;
  }

  // Constrain values to valid ranges
  light.brightness = constrain(light.brightness, 0, 100);
  light.hue = constrain(light.hue, 0, 360);
  light.saturation = constrain(light.saturation, 0, 100);

  return light;
}

// ===============================================================
//  Abstraction Helpers for Light Control
// ===============================================================
uint32_t SmartDevice::getRGB(SmartHome::Light lightCommand)
{
  if (!lightCommand.isOn)
    return 0; // Return black/off

  if (lightCommand.type == SmartHome::COLOR)
  {
    // Map 0-100 brightness/saturation and 0-360 hue to NeoPixel ranges
    return Adafruit_NeoPixel::ColorHSV(
        map(lightCommand.hue, 0, 360, 0, 65535),
        map(lightCommand.saturation, 0, 100, 0, 255),
        map(lightCommand.brightness, 0, 100, 0, 255));
  }
  else
  { // DIMMABLE
    uint8_t dim_val = map(lightCommand.brightness, 0, 100, 0, 255);
    return Adafruit_NeoPixel::Color(dim_val, dim_val, dim_val);
  }
}

uint8_t SmartDevice::getBrightnessValue(SmartHome::Light lightCommand)
{
  if (!lightCommand.isOn)
    return 0;
  return map(lightCommand.brightness, 0, 100, 0, 255);
}
