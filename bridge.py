# ====================================================================
#  McMaster Engineering Summer Camp - Smart Classroom IoT Hub
#  PRODUCTION VERSION 6.0 - FINAL ROBUST VERSION
#
#  This script is designed for maximum stability. It assumes
#  the Mosquitto server is running as a background service and will
#  continuously try to connect to it, making it resilient to
#  server restarts or crashes.
# ====================================================================

import paho.mqtt.client as mqtt
from arduino_iot_cloud import ArduinoCloudClient
import time
import traceback
import logging
import yaml
import threading
import sys

# --- 1. LOGGING SETUP ---
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s'
)

# --- 2. GLOBAL CLIENT VARIABLES ---
local_client = None
cloud_client_A = None
cloud_client_B = None


# --- 3. CLOUD CLIENT WORKER THREAD ---
class CloudWorker(threading.Thread):
    """A self-healing thread to manage a connection to the Arduino Cloud."""

    def __init__(self, account_name, client_instance):
        super().__init__()
        self.name = account_name
        self.client = client_instance
        self.daemon = True

    def run(self):
        while True:
            try:
                logging.info(f"Starting connection loop...")
                self.client.start()
            except Exception as e:
                logging.error(f"Connection loop for {self.name} crashed: {e}")
                logging.info(f"Attempting to reconnect {self.name} in 15 seconds...")
                time.sleep(15)


# --- 4. MQTT CALLBACKS ---

# === Callbacks for commands FROM Arduino Cloud ===
def on_curtain_write(client, value):
    if local_client and local_client.is_connected():
        local_client.publish("classroom/curtain/command", "OPEN" if value else "CLOSE")


def on_kettle_write(client, value):
    if local_client and local_client.is_connected():
        local_client.publish("classroom/appliance/command", "ON" if value else "OFF")


def on_lights_write(client, value):
    try:
        if local_client and local_client.is_connected():
            cmd = f"SWITCH={'ON' if value['switch'] else 'OFF'},BRIGHTNESS={value.get('brightness', 100)},R={value['color']['r']},G={value['color']['g']},B={value['color']['b']}"
            local_client.publish("classroom/lighting/command", cmd)
    except Exception:
        logging.error("Error parsing 'light' command from cloud.", exc_info=True)


def on_door_lock_write(client, value):
    if local_client and local_client.is_connected():
        local_client.publish("classroom/door/command", "LOCK" if value else "UNLOCK")


# === Callbacks for messages FROM local student devices ===
def on_connect_local(client, userdata, flags, rc, properties=None):
    if rc == 0:
        logging.info("Connection to Local Mosquitto Service SUCCEEDED.")
        client.subscribe("classroom/+/status")
        client.subscribe("classroom/+/data")
    else:
        logging.error(f"Failed to connect to local Mosquitto service, return code {rc}")


def on_message_local(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        logging.info(f"Local message received: Topic: {msg.topic}, Payload: {payload}")

        # Route message to the correct cloud account client
        if msg.topic == "classroom/environment/data":
            parts = {p.split(':')[0].strip(): p.split(':')[1].strip() for p in payload.split(',')}
            if "temp" in parts and cloud_client_A:
                cloud_client_A["classroom_temperature"] = float(parts["temp"])
            if "motion" in parts and cloud_client_A:
                cloud_client_A["classroom_motion"] = (parts["motion"].lower() == 'true')

        elif msg.topic == "classroom/curtain/status" and cloud_client_A:
            cloud_client_A["classroom_curtain"] = (payload.upper() == "OPEN")

        elif msg.topic == "classroom/appliance/status" and cloud_client_A:
            cloud_client_A["kettle"] = (payload.upper() == "ON")

        elif msg.topic == "classroom/door/status":
            door_open_str, lock_str = payload.split(',')
            if cloud_client_B:
                cloud_client_B["door_sensor"] = (door_open_str.upper() == "OPEN")
                cloud_client_B["door_lock"] = (lock_str.upper() == "LOCKED")

    except Exception:
        logging.error(f"Error processing local message from topic '{msg.topic}'", exc_info=True)


# --- 5. MAIN EXECUTION BLOCK ---
if __name__ == "__main__":
    logging.info("========================================")
    logging.info("  McMaster Smart Classroom Hub Starting ")
    logging.info("========================================")

    try:
        with open("config.yaml", 'r') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        logging.error("FATAL: config.yaml not found! Please create it next to the application. Exiting.")
        time.sleep(10)
        sys.exit(1)

    # --- Setup Account A ---
    if config.get('account_a', {}).get('enabled', False):
        try:
            acc_a_config = config['account_a']
            cloud_client_A = ArduinoCloudClient(
                device_id=acc_a_config['device_id'],
                username=acc_a_config['device_id'],
                password=bytes(acc_a_config['secret_key'], 'utf-8')
            )
            cloud_client_A.register("classroom_lights", on_write=on_lights_write)
            cloud_client_A.register("classroom_curtain", on_write=on_curtain_write)
            cloud_client_A.register("kettle", on_write=on_kettle_write)
            cloud_client_A.register("classroom_temperature")
            cloud_client_A.register("classroom_motion")
            CloudWorker("Account A", cloud_client_A).start()
        except Exception:
            logging.error("Failed to initialize Account A client.", exc_info=True)

    # --- Setup Account B ---
    if config.get('account_b', {}).get('enabled', False):
        try:
            acc_b_config = config['account_b']
            cloud_client_B = ArduinoCloudClient(
                device_id=acc_b_config['device_id'],
                username=acc_b_config['device_id'],
                password=bytes(acc_b_config['secret_key'], 'utf-8')
            )
            cloud_client_B.register("door_lock", on_write=on_door_lock_write)
            cloud_client_B.register("door_sensor")
            CloudWorker("Account B", cloud_client_B).start()
        except Exception:
            logging.error("Failed to initialize Account B client.", exc_info=True)

    # --- Setup Local Client with a ROBUST, RECONNECTING loop ---
    local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="SmartClassroomHub")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local

    # This loop ensures the main thread keeps trying to connect to Mosquitto
    while not local_client.is_connected():
        try:
            logging.info("Attempting to connect to local Mosquitto service...")
            local_client.connect("localhost", 1883, 60)
            local_client.loop_start()  # Start a background thread for the local client

            # Brief wait to confirm connection before breaking the loop
            time.sleep(1)
            if not local_client.is_connected():
                # This case handles if connect() returns without error but fails to establish
                local_client.loop_stop()
                raise ConnectionError("Failed to establish connection, retrying.")

        except (ConnectionRefusedError, ConnectionError) as e:
            logging.warning(f"Local Mosquitto connection failed: {e}. Is the service running? Retrying in 5 seconds...")
            time.sleep(5)
        except Exception as e:
            logging.error(f"An unexpected error occurred while connecting to local server: {e}", exc_info=True)
            time.sleep(10)

    # --- Keep the main thread alive ---
    try:
        logging.info("Hub is fully running. Press Ctrl+C in this window to exit.")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logging.info("Shutdown initiated by user.")
    finally:
        if local_client.is_connected():
            local_client.loop_stop()
        logging.info("Hub application has stopped.")