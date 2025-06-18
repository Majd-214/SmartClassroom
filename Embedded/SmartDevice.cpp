/*
====================================================================
  The Library Implementation (SmartDevice.cpp) - Version 3.0
====================================================================
  - Simplified logic to support the new universal, beginner-friendly header.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SmartDevice.h"

// --- Global objects ---
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MessageCallback global_message_callback = nullptr; // The one and only callback

// --- Global MQTT Callback Function ---
// This function now calls the single, universal message handler.
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) { message += (char)payload[i]; }

  Serial.printf("Message arrived on topic: %s. Payload: %s\n", topic, message.c_str());

  if (global_message_callback != nullptr) {
    global_message_callback(String(topic), message);
  }
}

// --- SmartDevice Class Method Implementations ---

SmartDevice::SmartDevice() {}

void SmartDevice::begin(const char* deviceName, const char* wifi_ssid, const char* wifi_pass, const char* mqtt_broker) {
  _deviceName = deviceName;

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.print(wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT Client
  mqttClient.setServer(mqtt_broker, 1883);
  mqttClient.setCallback(mqttCallback);
}

void SmartDevice::reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(_deviceName)) {
      Serial.println("connected!");
      // NOTE: Subscriptions are now handled manually by the student's sketch using subscribeTo().
      // The library no longer subscribes to anything automatically.
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void SmartDevice::update() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}

void SmartDevice::publishTo(String fullTopic, String payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(fullTopic.c_str(), payload.c_str());
  }
}

void SmartDevice::subscribeTo(String fullTopic) {
  if (mqttClient.connected()) {
    mqttClient.subscribe(fullTopic.c_str());
    Serial.print("--> Manually subscribed to topic: ");
    Serial.println(fullTopic);
  }
}

void SmartDevice::onMessage(MessageCallback callback) {
  global_message_callback = callback;
}