# McMaster Smart Classroom - IoT Hub README

## 1. Project Overview

Welcome to the McMaster Smart Classroom IoT Hub!

This document is the complete Standard Operating Procedure (SOP) for setting up and running the central hub application. The hub serves as the bridge between the student-built ESP8266 devices on a local network and the Arduino Cloud service, enabling a real-time web dashboard and Google Home voice control.

This software package contains three essential files for operation:

1. **`bridge.py`** (or `SmartClassroomHub.exe` if compiled): The main Python application that runs the bridge.
2. **`config.yaml`**: The configuration file where you must enter your cloud credentials and define local MQTT broker settings.
3. **`run_mosquitto_broker.bat`**: A script to easily start the local MQTT server (Mosquitto).

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
    * Connect the hub computer to the **dedicated router** using an **Ethernet cable**. This connection creates the private local network for your IoT devices.
    * Determine the hub computer's local IP address on this private network by running `ipconfig` in a Command Prompt and looking for the "Ethernet adapter" IPv4 Address. This IP address (e.g., `192.168.0.147`) must be noted down and will be used in the `config.yaml` and provided to students for their device code.

### 2.2 Student Computer Setup

Each computer the students will use must be prepared with the necessary software and libraries.

1. **Install Arduino IDE:** Install the [latest stable version of the Arduino IDE](https://www.arduino.cc/en/software).
2. **Install ESP8266 Board Support:**
    * Open the Arduino IDE.
    * Go to `File > Preferences`.
    * In the "Additional Boards Manager URLs:" field, add: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
        *(If there are other URLs, separate them with a comma and space)*
    * Click "OK".
    * Go to `Tools > Board > Boards Manager...`.
    * Search for "`esp8266`".
    * Select "`esp8266 by ESP8266 Community`" and click "Install".
    * Close the Boards Manager.
3. **Install the Custom `SmartDevice` Library (and its dependencies):**
    * The `SmartDevice` library (provided as `SmartDevice.zip`) is the core tool that simplifies networking and JSON parsing for the students.
    * In the Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...` and select the `SmartDevice.zip` file.
    * The Arduino IDE will detect the required dependencies (`ArduinoJson`, `Adafruit NeoPixel`, `PubSubClient`). A notification will appear (usually at the bottom right) prompting you to install them. Click "**Install all**".
4. **Access Starting Template:** Students can access the starting template for their devices via `File > Examples > SmartDevice > UniversalDeviceTemplate`.

---

## 3. Phase 2: Hub Operation (During Camp)

This is the operational guide for running the hub during the workshop.

### Step 1: Configure `config.yaml` (CRITICAL STEP)

Before launching the hub application, you **must** configure the `config.yaml` file. This file must be in the same folder as the `bridge.py` (or `SmartClassroomHub.exe`).

Open `config.yaml` with a text editor.

1. **Local MQTT Broker Configuration:**
    * Find the `local_mqtt_broker` section.
    * Set `host` to the IP address of your hub computer on the private `McMasterIoT-Camp` network (e.g., `192.168.0.147`).
    * The `port` should remain `1883` unless you've changed your Mosquitto configuration.

    ```yaml
    local_mqtt_broker:
      host: "YOUR_HUB_COMPUTER_IP" # e.g., "192.168.0.147"
      port: 1883
    ```

2. **Arduino Cloud Credentials:**
    * You will see sections like `account_a` and `account_b`.
    * **`enabled`**: Set to `true` to activate this Arduino Cloud account, or `false` to ignore it.
    * **`device_id`**: Paste the **Device ID** you obtained from your Arduino Cloud dashboard.
    * **`secret_key`**: Paste the **Secret Key** you obtained from your Arduino Cloud dashboard.

    **Example Configuration (fill in your actual credentials):**

    ```yaml
    # --- Arduino Cloud Credentials (Account A: Ambiance & Environment) ---
    account_a:
      enabled: true
      device_id: "YOUR_ACCOUNT_A_DEVICE_ID"
      secret_key: "YOUR_ACCOUNT_A_SECRET_KEY"
      # Variables for this account are defined below within the variables list

    # --- Arduino Cloud Credentials (Account B: Security & Access) ---
    account_b:
      enabled: false # Example: You might disable this account if not used for certain sessions
      device_id: "YOUR_ACCOUNT_B_DEVICE_ID"
      secret_key: "YOUR_ACCOUNT_B_SECRET_KEY"
      # Variables for this account are defined below within the variables list
    ```

    *Note: The `variables` list under each account in `config.yaml` defines the mapping between Arduino Cloud variables and local MQTT topics. Ensure these match your Arduino Cloud setup and student device topics.*

Save the file after entering your credentials.

### Step 2: Run the Hub Software

The hub requires two separate windows (command prompts) to be running simultaneously.

#### **Window 1: Start the MQTT Broker**

1. Navigate to the folder containing `run_mosquitto_broker.bat`.
2. Double-click the **`run_mosquitto_broker.bat`** script.
3. A command prompt window will appear with the title "Mosquitto Broker (MANUAL MODE)". It will show live log messages from the server.
4. **Keep this window open.** Closing it will immediately stop the MQTT broker.

#### **Window 2: Start the Hub Application**

1. Navigate to the folder containing `bridge.py` (or `SmartClassroomHub.exe`).
2. **If running from Python source:** Open a command prompt in this folder and run `python bridge.py`.
3. **If running from compiled executable:** Double-click **`SmartClassroomHub.exe`**.
4. A second command prompt window will open. It will first attempt to connect to the local Mosquitto broker, then proceed to connect to the configured Arduino Cloud accounts.
5. A successful launch will show logging output similar to this:

    ```
    INFO:root:===================================================
    INFO:root:   McMaster Smart Classroom Hub (Dynamic) Starting
    INFO:root:===================================================
    INFO:root:Local MQTT Broker configured as: 192.168.0.147:1883
    INFO:root:Initializing cloud client for account 'account_a' (Device ID: a5ae4bba-2de1-4550-a15a-ee4d37d8a75b)...
    INFO:root:   Registered cloud variable 'lights' (Direction: FROM_CLOUD, Type: Color) with on_write callback.
    ... (other variable registrations) ...
    INFO:root:Starting connection loop for account 'account_a'...
    INFO:root:Attempting to connect to local Mosquitto service at 192.168.0.147:1883...
    INFO:root:Connection to Local Mosquitto Service SUCCEEDED.
    INFO:root:Hub application has stopped successfully. (This message appears on shutdown, not startup)
    ```

    *Note: If you see "Ignoring variable init for swi, bri, hue" messages from the Arduino Cloud client, this is normal behavior for structured variables and can be disregarded.*

The hub is now online and ready to bridge data from the student devices to the Arduino Cloud and vice-versa.

## 4. Shutting Down

To shut down the entire system, simply **close both command prompt windows** (the Mosquitto Broker and the SmartClassroomHub application).
