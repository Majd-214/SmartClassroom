# McMaster Smart Classroom - IoT Hub

## 1. Overview

Welcome to the McMaster Smart Classroom IoT Hub!

This application serves as the central bridge between the student-built ESP8266 devices on a local network and the Arduino Cloud service. It uses a robust, self-healing architecture to ensure maximum uptime during the workshop.

This package contains three key files:
1.  **`SmartClassroomHub.exe`**: The main application that connects to the cloud.
2.  **`config.yaml`**: The configuration file for your cloud credentials.
3.  **`run_broker.bat`**: A script to manually start the local MQTT server.

## 2. System Requirements

To run the hub, the host computer must have the following software installed:

* **Windows Operating System** (Windows 10/11)
* **Mosquitto MQTT Broker:** Installed to the default location (`C:\Program Files\mosquitto`).
* A working internet connection.

## 3. One-Time Setup

Before running the hub for the first time, you must complete two simple setup steps.

### Step 1: Configure Mosquitto

1.  Navigate to the Mosquitto installation directory (`C:\Program Files\mosquitto`).
2.  Create a new text file named **`mosquitto.conf`**.
3.  Open the file and add the following two lines to allow devices on your local network to connect:
    ```
    listener 1883
    allow_anonymous true
    ```
4.  Save the file.

### Step 2: Configure the Hub Application

1.  Open the **`config.yaml`** file with a text editor.
2.  Fill in the `device_id` and `secret_key` for `account_a` and `account_b` using the credentials you downloaded from the Arduino Cloud.
    ```yaml
    # --- Arduino Cloud Credentials (Account A) ---
    account_a:
      enabled: true
      device_id: "PASTE_ACCOUNT_A_DEVICE_ID_HERE"
      secret_key: "PASTE_ACCOUNT_A_SECRET_KEY_HERE"

    # --- Arduino Cloud Credentials (Account B) ---
    account_b:
      enabled: true
      device_id: "PASTE_ACCOUNT_B_DEVICE_ID_HERE"
      secret_key: "PASTE_ACCOUNT_B_SECRET_KEY_HERE"
    ```
3.  Save the file.

---

## 4. How to Run the Hub (During Camp)

The hub requires two separate windows to be running simultaneously.

### Window 1: Start the MQTT Broker

1.  Double-click the **`run_broker.bat`** script.
2.  A command prompt window will appear with the title "Mosquitto Broker (MANUAL MODE)". It will show live log messages from the server.
3.  **Keep this window open.** Closing it will immediately stop the server.

### Window 2: Start the Hub Application

1.  With the broker running, double-click **`SmartClassroomHub.exe`**.
2.  A second command prompt window will open. It will patiently wait until it can connect to the broker, then connect to the Arduino Cloud.
3.  A successful launch will show the following output:
    ```
    INFO:root:Attempting to connect to local Mosquitto service...
    INFO:root:Connection to Local Mosquitto Service SUCCEEDED.
    INFO:root:Cloud Client (Account A) worker thread started.
    INFO:root:Hub is fully running. Press Ctrl+C in this window to exit.
    ```

The hub is now online and actively bridging data.

---

## 5. Shutting Down

To shut down the entire system, simply **close both command prompt windows**.