# McMaster Smart Classroom - IoT Hub README

## 1. Project Overview

Welcome to the McMaster Smart Classroom IoT Hub!

This document is the complete Standard Operating Procedure (SOP) for setting up and running the central hub application. The hub serves as the bridge between the student-built ESP8266 devices on a local network and the Arduino Cloud service, enabling a real-time web dashboard and Google Home voice control.

This software package contains three essential files for operation:

1. **`SmartClassroomHub.exe`**: The main application that runs the bridge.
2. **`config.yaml`**: The configuration file where you must enter your cloud credentials.
3. **`run_broker.bat`**: A script to manually start the local MQTT server.

---

## 2. Phase 1: Pre-Camp Preparation

This phase should be completed by the instructor before the day of the camp. It involves setting up the hub computer, the student computers, and the physical network.

### 2.1 Network & Router Setup

The project uses a private Wi-Fi network to ensure stability and avoid conflicts with the main campus network.

1. **Configure the Dedicated Router:**
    * Power on your dedicated Wi-Fi router.
    * Set the **SSID** (Network Name) to `McMasterIoT-Camp`.
    * Set a simple **Password** (e.g., `Roomba2025`).
    * Ensure the router's **DHCP server** is enabled.
2. **Connect the Hub Computer:**
    * Connect the hub computer to the **school's internet** using its **Wi-Fi adapter**.
    * Connect the hub computer to the **dedicated router** using an **Ethernet cable**. This connection creates the private local network.
    * Determine the hub computer's local IP address on this private network by running `ipconfig` in a Command Prompt and looking for the "Ethernet adapter" IPv4 Address. This IP address (e.g., `192.168.0.147`) must be written on the classroom whiteboard.

### 2.2 Student Computer Setup

Each computer the students will use must be prepared with the necessary software and libraries.

1. **Install Arduino IDE:** Install the latest version.
2. **Install ESP8266 Board Support:** In the IDE, go to `Tools > Board > Boards Manager...` and install the `esp8266` package.
3. **Install Required Libraries:** Go to `Tools > Manage Libraries...` and install:
    * `PubSubClient` by Nick O'Leary
    * `DHT sensor library` by Adafruit
    * `Adafruit NeoPixel` by Adafruit
    * `Servo` (usually included with the IDE)
4. **Install the Custom `SmartDevice` Library:**
    * The `SmartDevice` library (provided as `SmartDevice.zip`) is the core tool that simplifies networking for the students. It provides a simple API for them to use.
    * In the Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...` and select the `SmartDevice.zip` file.
    * After installation, students can access the starting template via `File > Examples > SmartDevice > UniversalDeviceTemplate`.

---

## 3. Phase 2: Hub Operation (During Camp)

This is the operational guide for running the hub during the workshop.

### Step 1: Configure `config.yaml` (CRITICAL STEP)

Before launching the hub application, you **must** provide your unique Arduino Cloud credentials in the `config.yaml` file. This file must be in the same folder as `SmartClassroomHub.exe`.

Open `config.yaml` with a text editor. You will see two sections: `account_a` and `account_b`.

* **`enabled`**: Set to `true` to activate this account, or `false` to ignore it.
* **`device_id`**: Paste the **Device ID** you received from the Arduino Cloud.
* **`secret_key`**: Paste the **Secret Key** you received from the Arduino Cloud.

**Example Configuration:**

```yaml
# --- Arduino Cloud Credentials (Account A: Ambiance & Environment) ---
account_a:
  enabled: true
  device_id: "a5ae4bba-2de1-4550-a15a-ee4d37d8a75b"
  secret_key: "?bWig6rFqOJquyU@IgClVQeNL"
  # Variables for this account:
  # - classroom_lights, classroom_curtain, classroom_motion,
  # - classroom_temperature, kettle

# --- Arduino Cloud Credentials (Account B: Security & Access) ---
account_b:
  enabled: true
  device_id: "ANOTHER_UNIQUE_DEVICE_ID_HERE"
  secret_key: "ANOTHER_UNIQUE_SECRET_KEY_HERE"
  # Variables for this account:
  # - door_sensor, door_lock
```

Save the file after entering your credentials.

### Step 2: Run the Hub Software

The hub requires two separate windows to be running simultaneously.

#### **Window 1: Start the MQTT Broker**

1. Double-click the **`run_broker.bat`** script.
2. A command prompt window will appear with the title "Mosquitto Broker (MANUAL MODE)". It will show live log messages from the server.
3. **Keep this window open.** Closing it will immediately stop the server.

#### **Window 2: Start the Hub Application**

1. With the broker running, double-click **`SmartClassroomHub.exe`**.
2. A second command prompt window will open. It will patiently wait until it can connect to the broker, then connect to the Arduino Cloud accounts.
3. A successful launch will show the following output:

    ```
    INFO:root:Attempting to connect to local Mosquitto service...
    INFO:root:Connection to Local Mosquitto Service SUCCEEDED.
    INFO:root:Cloud Client (Account A) worker thread started.
    INFO:root:Hub is fully running. Press Ctrl+C in this window to exit.
    ```

The hub is now online and ready to bridge data from the student devices.

---

## 4. Shutting Down

To shut down the entire system, simply **close both command prompt windows** (the Mosquitto Broker and the SmartClassroomHub).
