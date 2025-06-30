# ====================================================================
#   McMaster Engineering Summer Camp - Smart Classroom IoT Hub
#   PRODUCTION VERSION 2.5.1 - REFINED LOGIC - MAJD ABURAS 2025
#
#   This script is a generic engine. All logic for variable
#   mapping and message routing is loaded from config.yaml at runtime.
#
#   This script is licensed under the Creative Commons
#   Attribution-NonCommercial-ShareAlike 4.0 International License.
# ====================================================================

import sys
import time
import logging
import threading
import yaml
import json
from arduino_iot_cloud import ArduinoCloudClient, ColoredLight, DimmedLight
import paho.mqtt.client as mqtt

# --- 1. LOGGING & GLOBAL SETUP ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s')
local_client = None
cloud_clients = {}
mqtt_to_cloud_map = {}

DEFAULT_MQTT_BROKER_HOST = "localhost"
DEFAULT_MQTT_BROKER_PORT = 1883
VALID_VAR_TYPES = ["String", "Float", "Integer", "Boolean", "Dimmed", "Color"]
VALID_DIRECTIONS = ["TO_CLOUD", "FROM_CLOUD", "BIDIRECTIONAL"]


# --- 2. DYNAMIC CALLBACK & WORKER CLASSES ---
class CloudWorker(threading.Thread):
    """A self-healing thread to manage a connection to the Arduino Cloud."""
    def __init__(self, account_name, client_instance):
        super().__init__(name=account_name)
        self.client = client_instance
        self.daemon = True

    def run(self):
        while True:
            try:
                logging.info(f"Starting connection loop for account '{self.name}'...")
                self.client.start()
            except Exception as e:
                logging.error(f"Connection loop for '{self.name}' crashed: {e}", exc_info=True)
                time.sleep(15)

def generic_on_write_callback(client, value, variable_config):
    """Generic callback for commands received from the Arduino Cloud."""
    cloud_var_name = variable_config['name']
    base_topic = variable_config['topic']
    var_type = variable_config['type']

    logging.info(f"Cloud command for '{cloud_var_name}': {value} (Type: {var_type})")
    payload = ""
    try:
        if var_type == "Boolean":
            payload = "true" if value else "false"
        elif var_type == "Color":
            payload_dict = {"swi": value.swi, "bri": int(value.bri), "hue": int(value.hue), "sat": int(value.sat)}
            payload = json.dumps(payload_dict)
        elif var_type == "Dimmed":
            payload_dict = {"swi": value.swi, "bri": int(value.bri)}
            payload = json.dumps(payload_dict)
        else:
            payload = str(value)
    except Exception as e:
        logging.error(f"Error formatting payload for '{cloud_var_name}': {e}", exc_info=True)
        return

    if local_client and local_client.is_connected():
        logging.info(f"Publishing to local topic: '{base_topic}' with payload: '{payload}'")
        local_client.publish(base_topic, payload)
    else:
        logging.warning(f"Local MQTT client not connected. Could not send command for '{cloud_var_name}'.")

# --- 3. LOCAL MQTT CLIENT SETUP ---
def on_connect_local(client, userdata, flags, rc, properties=None):
    if rc == 0:
        logging.info("Connected to Local Mosquitto Service.")
        for topic in mqtt_to_cloud_map.keys():
            logging.info(f"Subscribing to local topic: {topic}")
            client.subscribe(topic)
    else:
        logging.error(f"Failed to connect to local Mosquitto, return code {rc}")

def on_disconnect_local(client, userdata, rc, properties=None):
    logging.warning(f"Local MQTT client disconnected unexpectedly (RC: {rc}). Reconnecting...")

def on_message_local(client, userdata, msg):
    """Handles messages from devices (local MQTT) and routes them to the cloud."""
    try:
        payload = msg.payload.decode()
        logging.info(f"Local message received: Topic: {msg.topic}, Payload: {payload}")

        if msg.topic in mqtt_to_cloud_map:
            rules = mqtt_to_cloud_map[msg.topic]
            for rule in rules:
                try:
                    cloud_client = cloud_clients[rule['account']]
                    cloud_var_name = rule['name']
                    var_type = rule['type']
                    value_to_send = None

                    if var_type == "Boolean": value_to_send = (payload.lower() == 'true')
                    elif var_type == "Float": value_to_send = float(payload)
                    elif var_type == "Integer": value_to_send = int(payload)
                    elif var_type in ["Color", "Dimmed"]: value_to_send = json.loads(payload)
                    else: value_to_send = payload

                    if value_to_send is not None:
                        logging.info(f"Pushing value '{value_to_send}' to cloud variable '{cloud_var_name}'")
                        cloud_client[cloud_var_name] = value_to_send
                except Exception as e:
                    logging.error(f"Error processing rule {rule} for topic {msg.topic}: {e}", exc_info=True)
    except Exception as e:
        logging.error(f"Error in on_message_local from topic '{msg.topic}': {e}", exc_info=True)

# --- 4. MAIN EXECUTION BLOCK ---
if __name__ == "__main__":
    logging.info("===================================================")
    logging.info("   McMaster Smart Classroom Hub (Dynamic) Starting   ")
    logging.info("===================================================")

    try:
        with open("config.yaml", 'r') as f:
            full_config = yaml.safe_load(f)
    except Exception as e:
        logging.error(f"FATAL: Could not load or parse config.yaml: {e}. Exiting.", exc_info=True)
        sys.exit(1)

    mqtt_broker_host = full_config.get('local_mqtt_broker', {}).get('host', DEFAULT_MQTT_BROKER_HOST)
    mqtt_broker_port = full_config.get('local_mqtt_broker', {}).get('port', DEFAULT_MQTT_BROKER_PORT)
    logging.info(f"Local MQTT Broker: {mqtt_broker_host}:{mqtt_broker_port}")

    for acc_name, acc_config in full_config.items():
        if acc_name == 'local_mqtt_broker' or not (isinstance(acc_config, dict) and acc_config.get('enabled', False)):
            continue
        try:
            logging.info(f"Initializing cloud client for account '{acc_name}'...")
            client = ArduinoCloudClient(device_id=acc_config['device_id'], username=acc_config['device_id'], password=acc_config['secret_key'].encode('utf-8'))
            
            for var_config in acc_config.get('variables', []):
                var_name = var_config.get('name')
                direction = var_config.get('direction')
                var_type = var_config.get('type')
                topic = var_config.get('topic')

                if not all([var_name, direction, var_type, topic]) or direction not in VALID_DIRECTIONS or var_type not in VALID_VAR_TYPES:
                    logging.error(f"Skipping invalid variable config in '{acc_name}': {var_config}")
                    continue
                
                callback = (lambda c, v, vc=var_config: generic_on_write_callback(c, v, vc)) if direction in ["FROM_CLOUD", "BIDIRECTIONAL"] else None

                if var_type == "Color": client.register(ColoredLight(var_name, on_write=callback))
                elif var_type == "Dimmed": client.register(DimmedLight(var_name, on_write=callback))
                else: client.register(var_name, on_write=callback)
                
                logging.info(f"   Registered cloud var '{var_name}' (Type: {var_type}, Dir: {direction})")

                if direction in ["TO_CLOUD", "BIDIRECTIONAL"]:
                    if topic not in mqtt_to_cloud_map: mqtt_to_cloud_map[topic] = []
                    rule = var_config.copy()
                    rule['account'] = acc_name
                    mqtt_to_cloud_map[topic].append(rule)
                    logging.info(f"   Mapped local topic '{topic}' to cloud var '{var_name}'")

            cloud_clients[acc_name] = client
            CloudWorker(acc_name, client).start()
        except Exception as e:
            logging.error(f"Failed to initialize cloud client for '{acc_name}': {e}", exc_info=True)

    local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="SmartClassroomHub")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local
    local_client.on_disconnect = on_disconnect_local

    while True:
        try:
            if not local_client.is_connected():
                logging.info(f"Attempting to connect to local Mosquitto at {mqtt_broker_host}:{mqtt_broker_port}...")
                local_client.connect(mqtt_broker_host, mqtt_broker_port, 60)
                local_client.loop_start()
            time.sleep(5)
        except KeyboardInterrupt:
            logging.info("Shutdown initiated.")
            break
        except Exception as e:
            logging.warning(f"Local Mosquitto connection error: {e}. Retrying in 10s...")
            time.sleep(10)
    
    if local_client.is_connected():
        local_client.loop_stop()
        local_client.disconnect()
    logging.info("Disconnected from local Mosquitto.")

    for acc_name, client in cloud_clients.items():
        try:
            client.stop()
            logging.info(f"Stopped cloud client for '{acc_name}'.")
        except Exception as e:
            logging.error(f"Error stopping cloud client for '{acc_name}': {e}", exc_info=True)

    logging.info("Hub application has stopped.")
