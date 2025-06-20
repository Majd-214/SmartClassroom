/*
====================================================================
  The Library Implementation (SmartDevice.cpp) - Version 3.1 (Robust Subscriptions)
====================================================================
  - Simplified logic to support the new universal, beginner-friendly header.
  - Implements automatic re-subscription to topics after MQTT reconnection.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SmartDevice.h"
#include <vector> // NEW: Required for std::vector

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
  // Clear the list of subscribed topics upon object creation.
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

void SmartDevice::reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(_deviceName))
    {
      Serial.println("connected!");
      // NEW: Re-subscribe to all previously requested topics after a successful reconnection.
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
    reconnect();
  }
  mqttClient.loop(); // Process incoming MQTT messages and maintain connection.
}

void SmartDevice::publishTo(String fullTopic, String payload)
{
  if (mqttClient.connected())
  {
    mqttClient.publish(fullTopic.c_str(), payload.c_str());
  }
  else
  {
    Serial.println("MQTT client not connected, cannot publish.");
  }
}

void SmartDevice::subscribeTo(String fullTopic)
{
  // Check if the topic is already in our list to avoid duplicates.
  bool found = false;
  for (const String &topic : _subscribedTopics)
  {
    if (topic == fullTopic)
    {
      found = true;
      break;
    }
  }

  // If not found, add it to our internal list of topics to subscribe to.
  if (!found)
  {
    _subscribedTopics.push_back(fullTopic);
  }

  // Attempt to subscribe immediately if the MQTT client is currently connected.
  if (mqttClient.connected())
  {
    mqttClient.subscribe(fullTopic.c_str());
    Serial.printf("Subscribed to topic: %s\n", fullTopic.c_str());
  }
  else
  {
    // If not connected, the topic is stored and will be subscribed upon reconnection.
    Serial.println("MQTT client not connected, added to queue for re-subscription on connect.");
  }
}

// NEW: Private helper method to re-subscribe to all stored topics.
void SmartDevice::_resubscribe()
{
  Serial.println("Re-subscribing to topics...");
  if (_subscribedTopics.empty())
  {
    Serial.println("No topics to re-subscribe to.");
    return;
  }
  for (const String &topic : _subscribedTopics)
  {
    // Attempt to subscribe with QoS 0 (most common for simple devices)
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
  // Set the global message callback function to the one provided by the sketch.
  global_message_callback = callback;
}