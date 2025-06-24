# SmartDevice Class API Reference (v2.4.0)

McMaster Smart Classroom - Arduino IoT Library © 2025 by Majd Aburas

## Introduction

Welcome, McMaster Engineering campers\!

The `SmartDevice` library is a custom tool designed to make connecting your ESP8266 projects to the central Smart Classroom Hub incredibly simple. It handles all the complex Wi-Fi and networking code for you, so you can focus on the fun part: writing the logic for your unique sensor or actuator.

This document is your guide to using the library.

-----

## Getting Started

To use the library in your Arduino sketch, you need to do two simple things after installing it.

1. **Include the library:** At the top of your `.ino` file, add this line:

    ```c++
    #include <SmartDevice.h>
    ```

2. **Create a device object:** Just like you would for a sensor or servo, create a global object for your `SmartDevice`. You will use this object to access all of the library's functions.

    ```cpp
    SmartDevice myDevice;
    ```

-----

## Safe Pin Definitions

To prevent errors and make wiring easier, the `SmartDevice` library includes definitions for the safe-to-use GPIO pins on your ESP8266 NodeMCU board. You should use these names (e.g., `PIN_D1`) instead of raw numbers in your code.

**IMPORTANT:** Some pins have special functions during boot-up. The pins listed below are generally safe for general-purpose input/output (GPIO) without causing boot issues.

| Safe Name | ESP8266 GPIO Number | Notes |
| :-------- | :------------------ | :---- |
| `PIN_D1`  | `5`                 | Safe to use. |
| `PIN_D2`  | `4`                 | Safe to use. |
| `PIN_D5`  | `14`                | Safe to use. |
| `PIN_D6`  | `12`                | Safe to use. |
| `PIN_D7`  | `13`                | Safe to use. |

**Example:**

```c++
// Correct way to set pin mode
pinMode(PIN_D5, INPUT);
```

-----

## SmartHome Namespace

The `SmartHome` namespace contains data structures (structs and enums) used for defining IoT variable types. These structures simplify receiving complex data, especially for devices like smart lights.

### `enum LightType`

Defines the type of light being represented by a `SmartHome::Light` struct.

* `DIMMABLE`: For lights that can only control brightness (0-100).
* `COLOR`: For lights that can control brightness and color (Hue:0-360, Saturation:0-100).

### `struct Light`

Represents the state of a smart light, including its type, on/off state, brightness, hue, and saturation. This struct is primarily used when *receiving* light commands from the Smart Hub.

| Member     | Type           | Description                                                        |
| :--------- | :------------- | :----------------------------------------------------------------- |
| `isOn`     | `bool`         | `true` if ON, `false` if OFF.                                      |
| `brightness` | `int`          | Brightness level (0-100).                                          |
| `hue`      | `int`          | Hue value (0-360), relevant for `COLOR` lights.                    |
| `saturation` | `int`          | Saturation value (0-100), relevant for `COLOR` lights.             |
| `type`     | `LightType`    | Specifies if the light is `DIMMABLE` or `COLOR`.                   |

-----

## SmartDevice Class

The `SmartDevice` class provides all the essential functionalities for your ESP8266 to connect to Wi-Fi, communicate with the MQTT broker, and handle incoming/outgoing messages.

### Constructor

`SmartDevice()`

Initializes a new instance of the `SmartDevice` class. You will typically declare this as a global object in your sketch.

```c++
SmartDevice myDevice; // Declare a SmartDevice object
```

### Public Methods

#### `void begin(const char *deviceName, const char *wifi_ssid, const char *wifi_pass, const char *mqtt_broker)`

The main setup function for your device. It connects to the specified Wi-Fi network and establishes a connection to the MQTT Hub. This function should be called once in your `setup()` function.

* `deviceName`: A unique name for your device (e.g., "Station1\_Lights").
* `wifi_ssid`: The Wi-Fi network SSID (e.g., "McMasterIoT-Camp").
* `wifi_pass`: The Wi-Fi network password (e.g., "Roomba2025").
* `mqtt_broker`: The IP address of the MQTT Broker (e.g., "192.168.0.147").

<!-- end list -->

```c++
myDevice.begin(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP);
```

#### `void update()`

The heartbeat of the library. You must call this at the beginning of every `loop()` to keep the device connected and check for new messages.

```c++
void loop() {
  myDevice.update(); // Keep the device online and process messages
  readSensor();      // Your custom sensor reading function (if any)
}
```

#### `void publishTo(String fullTopic, String payload)`

Publishes a message to any topic you specify manually. This is perfect for sending sensor readings, status updates, or for a Master Control Panel that needs to send commands to many different devices.

* `fullTopic`: The complete MQTT topic path (e.g., `"classroom/thermostat"` or `"classroom/curtain"`).
* `payload`: The data you want to send as a string. For structured data (like a device's current light state), you might need to construct a JSON string manually before publishing.

<!-- end list -->

```c++
// Example: Publishing a temperature reading
myDevice.publishTo("classroom/thermostat", String(currentTemp));

// Example: Publishing a device's current state as JSON (if needed for complex states)
// DynamicJsonDocument doc(64);
// doc["swi"] = true;
// doc["bri"] = 80;
// String output;
// serializeJson(doc, output);
// myDevice.publishTo("classroom/some_device/status", output);
```

#### `void subscribeTo(String fullTopic)`

Subscribes your device to a specific MQTT topic path, allowing it to receive messages published on that topic. The library automatically remembers and re-subscribes to these topics if the connection drops and re-establishes.

* `fullTopic`: The complete MQTT topic path to subscribe to (e.g., `"classroom/lighting"` for light commands, or `"classroom/master/broadcast"` for a global message).

<!-- end list -->

```c++
void setupDevice() { // Your custom setup function
  // ...
  // Subscribe to the topic where this device expects commands
  myDevice.subscribeTo(LIGHTING_TOPIC);
  myDevice.subscribeTo("classroom/master/broadcast"); // Subscribe to a global topic
}
```

#### `void onMessage(MessageCallback callback)`

Registers a function to handle all incoming MQTT messages received by your device. The registered `callback` function will be executed whenever a message arrives on any topic your device is subscribed to. Your callback function will receive both the topic and the payload, allowing you to write flexible logic for different messages.

* `callback`: A pointer to a function with the signature `void func(String topic, String payload)`.

<!-- end list -->

```c++
// Define your message handler function (e.g., triggerActuator in the template)
void handleIncomingMessages(String topic, String payload) {
  if (topic == LIGHTING_TOPIC) {
    // Process the light command using SmartDevice::commandToLight
    // ...
  } else if (topic == "classroom/curtain") {
    // Process curtain commands
    // ...
  }
}

void setup() {
  // ...
  // Register your main message handler in setup()
  myDevice.onMessage(handleIncomingMessages);
  // ...
}
```

#### `bool isConnected()`

Checks if the MQTT client is currently connected to the central hub.

* **Returns:** `bool` - `true` if connected, `false` otherwise.

<!-- end list -->

```c++
if (myDevice.isConnected()) {
  Serial.println("Device is online.");
} else {
  Serial.println("Device is offline, attempting reconnect...");
}
```

### Static Helper Functions (Accessed directly from `SmartDevice` class)

These functions do not require a `myDevice` object and can be called directly using `SmartDevice::`.

#### `static SmartHome::Light commandToLight(String command)`

This function is critical for processing commands from the Smart Hub, especially for lights. It parses the incoming JSON string (`command`) into an easy-to-use `SmartHome::Light` struct.

* `command`: The incoming JSON string payload (e.g., `{"bri":100,"hue":309,"sat":69,"swi":true}`).

* **Returns:** `SmartHome::Light` - A `Light` struct populated with the parsed command details (`isOn`, `brightness`, `hue`, `saturation`, `type`).

<!-- end list -->

```c++
void triggerActuator(String topic, String command) {
  if (topic == LIGHTING_TOPIC) {
    SmartHome::Light lightCommand = SmartDevice::commandToLight(command);

    if (lightCommand.isOn) {
      // Use the parsed lightCommand to control your physical light
      // e.g., Set NeoPixel color using SmartDevice::getRGB(lightCommand)
    } else {
      // Turn off the light
    }
  }
}
```

#### `static uint32_t getRGB(SmartHome::Light lightCommand)`

This helper function simplifies setting colors for NeoPixel (WS2812B) LEDs. It takes a `SmartHome::Light` struct (which contains brightness, hue, and saturation) and converts it into a `uint32_t` RGB color value directly compatible with `Adafruit_NeoPixel::setPixelColor()`.

* `lightCommand`: A `SmartHome::Light` struct containing the desired light state.

* **Returns:** `uint32_t` - A 32-bit integer representing the RGB color (e.g., `0xFF0000` for red). If `lightCommand.type` is `DIMMABLE`, it returns a grayscale color based on brightness.

<!-- end list -->

```c++
// Assuming 'strip' is your Adafruit_NeoPixel object
void triggerActuator(String topic, String command) {
  if (topic == LIGHTING_TOPIC) {
    SmartHome::Light lightCommand = SmartDevice::commandToLight(command);

    if (lightCommand.isOn) {
      uint32_t color = SmartDevice::getRGB(lightCommand); // Get the NeoPixel color
      strip.setPixelColor(0, color); // Set your NeoPixel
    } else {
      strip.clear(); // Turn off
    }
    strip.show(); // Update the NeoPixel
  }
}
```

#### `static uint8_t getBrightnessValue(SmartHome::Light lightCommand)`

This helper function provides the raw brightness value (0-255) for dimmable lights, directly mapped from the 0-100 `brightness` in the `Light` struct. Useful if you're controlling a simple dimmer or an analog LED.

* `lightCommand`: A `SmartHome::Light` struct.

* **Returns:** `uint8_t` - The brightness value mapped to a 0-255 range.

<!-- end list -->

```c++
// Example for an analog dimmable LED (using PWM, e.g., on an ESP8266 GPIO)
void triggerActuator(String topic, String command) {
  if (topic == DIMMABLE_LIGHT_TOPIC) { // Assuming a different topic for dimmable
    SmartHome::Light lightCommand = SmartDevice::commandToLight(command);

    if (lightCommand.isOn && lightCommand.type == SmartHome::DIMMABLE) {
      uint8_t brightnessPWM = SmartDevice::getBrightnessValue(lightCommand);
      analogWrite(DIMMER_PIN, brightnessPWM); // Use analogWrite (PWM)
    } else {
      analogWrite(DIMMER_PIN, 0); // Turn off
    }
  }
}
```

-----

## Licensing

This library is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. To view a copy of this license, visit [https://creativecommons.org/licenses/by-nc-sa/4.0/](https://creativecommons.org/licenses/by-nc-sa/4.0/)
