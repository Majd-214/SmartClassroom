# ====================================================================
#  McMaster Engineering Summer Camp - Smart Classroom IoT Hub
#  PRODUCTION VERSION 7.0 - DYNAMIC & CONFIG-DRIVEN
#
#  This script is now a generic engine. All logic for variable
#  mapping and message routing is loaded from config.yaml at runtime.
# ====================================================================

import paho.mqtt.client as mqtt
from arduino_iot_cloud import ArduinoCloudClient
import time
import traceback
import logging
import yaml
import threading
import sys
from datetime import datetime

# --- 1. LOGGING & GLOBAL SETUP ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s')
local_client = None
cloud_clients = {}  # Dictionary to hold all cloud clients
mqtt_to_cloud_map = {}  # Our dynamic routing table


# --- 2. DYNAMIC CALLBACK & WORKER CLASSES ---
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
                logging.error(f"Connection loop crashed: {e}")
                time.sleep(15)


def generic_on_write_callback(client, value, variable_config):
    """A generic callback factory for all actuator variables."""
    cloud_var_name = variable_config['name']
    local_topic = variable_config['topic']
    var_type = variable_config['type']

    logging.info(f"Cloud command received for '{cloud_var_name}': {value}")

    # Format payload based on variable type
    payload = ""
    if var_type == "Boolean":
        payload = "ON" if value else "OFF"
        if "lock" in cloud_var_name: payload = "LOCK" if value else "UNLOCK"
    elif var_type == "Color":
        try:
            cmd = f"SWITCH={'ON' if value['switch'] else 'OFF'},BRIGHTNESS={value.get('brightness', 100)},R={value['color']['r']},G={value['color']['g']},B={value['color']['b']}"
            payload = cmd
        except Exception:
            logging.error("Error parsing color command", exc_info=True)
    else:  # Default to sending the raw value
        payload = str(value)

    if local_client and local_client.is_connected():
        command_topic = f"{local_topic}/command"
        local_client.publish(command_topic, payload)


# --- 3. LOCAL MQTT CLIENT SETUP ---
def on_connect_local(client, userdata, flags, rc, properties=None):
    if rc == 0:
        logging.info("Connection to Local Mosquitto Service SUCCEEDED.")
        # Dynamically subscribe to all topics defined in the config
        for topic in mqtt_to_cloud_map.keys():
            logging.info(f"Subscribing to local topic: {topic}")
            client.subscribe(topic)
    else:
        logging.error(f"Failed to connect to local Mosquitto service, return code {rc}")


def on_message_local(client, userdata, msg):
    """Handles messages from student devices and routes them to the cloud."""
    try:
        payload = msg.payload.decode()
        logging.info(f"Local message received: Topic: {msg.topic}, Payload: {payload}")

        # Look up the routing rule in our map
        if msg.topic in mqtt_to_cloud_map:
            rules = mqtt_to_cloud_map[msg.topic]
            for rule in rules:
                try:
                    cloud_client = cloud_clients[rule['account']]
                    cloud_var_name = rule['name']
                    key = rule.get('key_in_payload')
                    var_type = rule['type']

                    value_to_send = None
                    if key:  # Payload is structured, e.g., "temp:25,motion:true"
                        parts = {p.split(':')[0].strip(): p.split(':')[1].strip() for p in payload.split(',')}
                        if key in parts:
                            raw_value = parts[key]
                            if var_type == "Boolean":
                                value_to_send = (raw_value.lower() == 'true')
                            elif var_type == "Float":
                                value_to_send = float(raw_value)
                            elif var_type == "Integer":
                                value_to_send = int(raw_value)
                            else:
                                value_to_send = raw_value
                    else:  # Payload is a simple value, e.g., "OPEN"
                        if var_type == "Boolean":
                            value_to_send = (payload.upper() in ["ON", "OPEN", "LOCKED"])
                        else:
                            value_to_send = payload

                    if value_to_send is not None:
                        logging.info(f"Pushing value '{value_to_send}' to cloud variable '{cloud_var_name}'")
                        cloud_client[cloud_var_name] = value_to_send

                except Exception:
                    logging.error(f"Error processing rule for {msg.topic}", exc_info=True)

        # --- Handle Special Kiosk Requests ---
        elif msg.topic == "classroom/kiosk/request":
            if payload == "CURRENT_TIME":
                now = datetime.now().strftime("%I:%M %p")  # e.g., "02:30 PM"
                cloud_clients['account_b']['kiosk_display_line1'] = "Current Time"
                cloud_clients['account_b']['kiosk_display_line2'] = now
            # Add logic for other kiosk requests here...

    except Exception:
        logging.error(f"Error processing main local message from topic '{msg.topic}'", exc_info=True)


# --- 4. MAIN EXECUTION BLOCK ---
if __name__ == "__main__":
    logging.info("=====================================================")
    logging.info("  McMaster Smart Classroom Hub (Dynamic) Starting  ")
    logging.info("=====================================================")

    try:
        with open("dist/config.yaml", 'r') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        logging.error("FATAL: config.yaml not found! Exiting.")
        time.sleep(10);
        sys.exit(1)

    # --- Dynamically Setup Cloud Clients from Config ---
    for acc_name, acc_config in config.items():
        if isinstance(acc_config, dict) and acc_config.get('enabled', False):
            try:
                logging.info(f"Initializing cloud client for '{acc_name}'...")
                client = ArduinoCloudClient(
                    device_id=acc_config['device_id'],
                    username=acc_config['device_id'],
                    password=bytes(acc_config['secret_key'], 'utf-8')
                )

                for var_config in acc_config.get('variables', []):
                    # Register the variable
                    var_name = var_config['name']
                    direction = var_config['direction']

                    if direction in ["FROM_CLOUD", "BIDIRECTIONAL"]:
                        # Create a callback with the specific variable config
                        callback = lambda c, v, vc=var_config: generic_on_write_callback(c, v, vc)
                        client.register(var_name, on_write=callback)
                    else:
                        client.register(var_name)

                    # Build the MQTT routing map
                    if direction in ["TO_CLOUD", "BIDIRECTIONAL"]:
                        topic = var_config['topic']
                        # For BIDIRECTIONAL, the status topic is topic + "/status"
                        if direction == "BIDIRECTIONAL": topic += "/status"

                        if topic not in mqtt_to_cloud_map: mqtt_to_cloud_map[topic] = []
                        rule = var_config.copy()
                        rule['account'] = acc_name
                        mqtt_to_cloud_map[topic].append(rule)

                cloud_clients[acc_name] = client
                CloudWorker(acc_name, client).start()
            except Exception:
                logging.error(f"Failed to initialize {acc_name} client.", exc_info=True)

    # --- Setup and connect the Local Client ---
    local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="SmartClassroomHub")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local

    while True:
        try:
            if not local_client.is_connected():
                logging.info("Attempting to connect to local Mosquitto service...")
                local_client.connect("localhost", 1883, 60)
                local_client.loop_start()
            time.sleep(2)
        except KeyboardInterrupt:
            logging.info("Shutdown initiated by user.")
            break
        except Exception:
            logging.warning("Local Mosquitto connection failed. Retrying in 5s...", exc_info=True)
            time.sleep(5)

    if local_client.is_connected(): local_client.loop_stop()
    logging.info("Hub application has stopped.")
