# SmartDevice Class API Reference (v2.0)

McMaster Smart Classroom - Arduino IoT Library © 2025 by Majd Aburas

## Introduction

Welcome, McMaster Engineering campers!

The `SmartDevice` library is a custom tool designed to make connecting your ESP8266 projects to the central Smart Classroom Hub incredibly simple. It handles all the complex Wi-Fi and networking code for you, so you can focus on the fun part: writing the logic for your unique sensor or actuator.

This document is your guide to using the library.

---

## Getting Started

To use the library in your Arduino sketch, you need to do two simple things after installing it.

1. **Include the library:** At the top of your `.ino` file, add this line:

    ```C++
    #include <SmartDevice.h>
    ```

2. **Create a device object:** Just like you would for a sensor or servo, create a global object for your `SmartDevice`. You will use this object to access all of the library's functions.

    ```cpp
    SmartDevice myDevice;
    ```

---

## Safe Pin Definitions

To prevent errors and make wiring easier, the `SmartDevice` library includes definitions for the safe-to-use GPIO pins on your ESP8266 NodeMCU board. You should use these names (e.g., `PIN_D1`) instead of raw numbers in your code.

| Safe Name | GPIO Number |
| :-------- | :---------- |
| `PIN_D1`  | `5`         |
| `PIN_D2`  | `4`         |
| `PIN_D5`  | `14`        |
| `PIN_D6`  | `12`        |
| `PIN_D7`  | `13`        |

**Example:**

```C++
// Correct way to set pin mode
pinMode(PIN_D5, INPUT);
```

## Class Reference

This section details every function you can use from the SmartDevice library.

### void `begin()`

The main setup function for your device. It connects to Wi-Fi and the classroom hub. Call this once in your `setup()` function.

```C++
void begin(const char* deviceName, DeviceType deviceType, const char* wifi_ssid, const char* wifi_pass, const char* mqtt_broker);
```

* `deviceName`: A unique name for your device (e.g., "Main_Door").
* `deviceType`: The type of station you are building (e.g., STATION_DOOR).
* `wifi_ssid`: The name of the Wi-Fi network.
* `wifi_pass`: The password for the Wi-Fi network.
* `mqtt_broker`: The IP address of the hub computer.

### `void update()`

The heartbeat of the library. You must call this at the beginning of every `loop()` to keep the device connected and check for new messages.

```C++
void update();
```

Used by most stations to send data. It automatically builds the correct topic path based on your device's type.

### `void publish()`

```C++
void publish(String subtopic, String payload);
```

* `subtopic`: The final part of the topic path (e.g., `"status"` or `"data"`).
* `payload`: The data you want to send (e.g., `"ON"` or `"temp:22.5"`).

```C++
// If this is a STATION_DOOR device, this publishes to "classroom/door/status"
myDevice.publish("status", "LOCKED");
void onCommand()
```

For simple actuators. Registers a function that will be called only when a command is sent to your device's default command topic. Your function will receive the message payload as a String.

### `void onCommand(CommandCallback callback);`

```C++
// Define your handler function
void handleHvacCommands(String command) {
  if (command == "ON") { /*...*/ }
}

void setup() {
  // Register it
  myDevice.onCommand(handleHvacCommands);
  // ...
}
```

### Advanced Functions (For Master Control Panel)

These functions give you more control and are essential for the Station 6 - Master Control Panel mission.

### `void publishTo()`

Publishes a message to any topic you specify manually. This is perfect for a control panel that needs to send commands to many different devices.

```C++
void publishTo(String fullTopic, String payload);
```

```C++
// Directly send a command to the curtain's command topic
myDevice.publishTo("classroom/curtain/command", "OPEN");
```

### `void subscribeTo()`

Subscribes to any topic you specify manually. The control panel will use this to listen to the status topics of other devices to know their current state.

```C++
void subscribeTo(String fullTopic);
```

```C++
void setup() {
  // ...
  // Tell the hub we want to receive updates from the curtain
  myDevice.subscribeTo("classroom/curtain/status");
}
```

### `void onMessage()`

An advanced callback for devices listening to multiple topics. Your function will receive both the topic and the payload, so you can write logic to handle messages from different sources.

```C++
void onMessage(MessageCallback callback);
```

```C++
// Define your advanced handler function
void handlePanelMessages(String topic, String payload) {
  if (topic == "classroom/curtain/status") {
    // Update the curtain's state variable and LED
  } else if (topic == "classroom/hvac/status") {
    // Update the HVAC's state variable and LED
  }
}

void setup() {
  // Register the advanced handler
  myDevice.onMessage(handlePanelMessages);
  // ...
}
```

The `DeviceType` Enum
When you call `myDevice.begin()`, you must specify your station's type.

#### Available Types

* `STATION_LIGHTING`
* `STATION_CURTAIN`
* `STATION_ENVIRONMENT`
* `STATION_APPLIANCE` (Note: The HVAC station should use STATION_HVAC)
* `STATION_DOOR`
* `STATION_HVAC`
* `STATION_PANEL`

## Licensing

This library is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. To view a copy of this license, visit <https://creativecommons.org/licenses/by-nc-sa/4.0/>
