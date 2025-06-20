# ====================================================================
#   McMaster Engineering Summer Camp - Smart Classroom IoT Hub
#   PRODUCTION VERSION 2.0 - FULLY DYNAMIC ENGINE - MAJD ABURAS 2025
#   This library is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 
#   4.0 International License. To view a copy of this license, visit
#   https://creativecommons.org/licenses/by-nc-sa/4.0/
#
#   This script is now a generic engine. All logic for variable
#   mapping and message routing is loaded from config.yaml at runtime.
# ====================================================================

import paho.mqtt.client as mqtt
from arduino_iot_cloud import ArduinoCloudClient
import time
import logging
import yaml
import threading
import sys
import json # Added for parsing Color/Dimmed JSON payloads from local devices

# --- 1. LOGGING & GLOBAL SETUP ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s')
local_client = None
cloud_clients = {}  # Dictionary to hold all cloud clients
mqtt_to_cloud_map = {}  # Our dynamic routing table


# --- 2. DYNAMIC CALLBACK & WORKER CLASSES ---
class CloudWorker(threading.Thread):
    """A self-healing thread to manage a connection to the Arduino Cloud."""

    def __init__(self, account_name, client_instance):
        # Pass name to the super constructor for proper thread naming
        super().__init__(name=account_name)
        self.client = client_instance
        self.daemon = True

    def run(self):
        while True:
            try:
                logging.info(f"Starting connection loop for account '{self.name}'...")
                self.client.start()
            except Exception as e:
                logging.error(f"Connection loop for account '{self.name}' crashed: {e}", exc_info=True)
                time.sleep(15)


def generic_on_write_callback(client, value, variable_config):
    """
    A generic callback factory for all actuator variables.
    This function is called when a command is received from the Arduino Cloud.
    It formats the payload and publishes it to the local MQTT broker
    for the student devices to receive.
    """
    cloud_var_name = variable_config['name']
    base_topic = variable_config['topic']  # This is the base topic from config.yaml
    var_type = variable_config['type']
    direction = variable_config['direction'] # Get direction from config to determine correct command topic

    logging.info(f"Cloud command received for '{cloud_var_name}': {value}")

    # Format payload based on variable type
    payload = ""
    if var_type == "Boolean":
        payload = "ON" if value else "OFF"
    elif var_type == "Color":
        try:
            # Assuming 'value' for Color type is a dictionary like {"bri":"43","hue":"355","sat":"74","swi":"true"}
            # The 'swi' field indicates the switch state (on/off)
            switch_state = "ON" if str(value.get('swi', 'false')).lower() == 'true' else "OFF"
            brightness = str(value.get('bri', '100')) # Ensure string for payload
            hue = str(value.get('hue', '0'))
            saturation = str(value.get('sat', '0'))

            cmd = f"SWITCH={switch_state},BRIGHTNESS={brightness},HUE={hue},SATURATION={saturation}"
            payload = cmd
        except Exception:
            logging.error("Error parsing color command. Expected dictionary format with 'bri', 'hue', 'sat', 'swi'.", exc_info=True)
            return # Don't publish if payload formatting fails
    elif var_type == "Dimmed":
        try:
            # Assuming 'value' for Dimmed type is a dictionary like {"bri":"37","swi":"true"}
            switch_state = "ON" if str(value.get('swi', 'false')).lower() == 'true' else "OFF"
            brightness = str(value.get('bri', '100')) # Ensure string for payload

            cmd = f"SWITCH={switch_state},BRIGHTNESS={brightness}"
            payload = cmd
        except Exception:
            logging.error("Error parsing dimmed command. Expected dictionary format with 'bri', 'swi'.", exc_info=True)
            return # Don't publish if payload formatting fails
    else:  # Default to sending the raw value as a string
        payload = str(value)

    if local_client and local_client.is_connected():
        # Determine the correct command topic based on direction
        if direction == "FROM_CLOUD" or direction == "BIDIRECTIONAL":
            # For FROM_CLOUD, retrieve the base topic from config.yaml (e.g., classroom/curtain)
            logging.info(f"Publishing to local topic: '{base_topic}' with payload: '{payload}'")
            local_client.publish(base_topic, payload)
        else: # This callback should only be triggered for FROM_CLOUD or BIDIRECTIONAL variables
            logging.warning(f"Unexpected direction '{direction}' for cloud command callback for {cloud_var_name}. Not publishing.")
            return

    else:
        logging.warning(f"Local MQTT client not connected. Could not send command for '{cloud_var_name}'.")


# --- 3. LOCAL MQTT CLIENT SETUP ---
def on_connect_local(client, userdata, flags, rc, properties=None):
    """Callback for when the local MQTT client connects to Mosquitto."""
    if rc == 0:
        logging.info("Connection to Local Mosquitto Service SUCCEEDED.")
        # Dynamically subscribe to all topics defined in the config that are TO_CLOUD or BIDIRECTIONAL
        for topic in mqtt_to_cloud_map.keys():
            logging.info(f"Subscribing to local topic: {topic}")
            client.subscribe(topic)
    else:
        logging.error(f"Failed to connect to local Mosquitto service, return code {rc}")

def on_message_local(client, userdata, msg):
    """
    Handles messages from student devices (local MQTT) and routes them to the cloud.
    This function parses the incoming payload based on the configuration.
    """
    try:
        payload = msg.payload.decode()
        logging.info(f"Local message received: Topic: {msg.topic}, Payload: {payload}")

        # Look up the routing rule(s) in our map for this topic
        if msg.topic in mqtt_to_cloud_map:
            rules = mqtt_to_cloud_map[msg.topic]
            for rule in rules:
                try:
                    cloud_client = cloud_clients[rule['account']]
                    cloud_var_name = rule['name']
                    var_type = rule['type']

                    value_to_send = None

                    # With structured payloads removed, the entire payload is the value
                    # We just need to convert it based on the variable type
                    if var_type == "Boolean":
                        # Expecting "true" or "false" strings directly from Arduino
                        value_to_send = (payload.lower() == 'true')
                    elif var_type == "Float":
                        value_to_send = float(payload)
                    elif var_type == "Integer":
                        value_to_send = int(payload)
                    elif var_type == "Color" or var_type == "Dimmed":
                        # For Color and Dimmed, expect the payload to be a JSON string
                        try:
                            value_to_send = json.loads(payload)
                        except json.JSONDecodeError:
                            logging.error(f"Failed to decode JSON for {var_type} type from payload '{payload}' on topic '{msg.topic}'. Expected JSON string.", exc_info=True)
                            value_to_send = None # Or raise an error, depending on desired strictness
                    else: # Default to sending the raw value as a string for other types
                        value_to_send = payload

                    if value_to_send is not None:
                        logging.info(f"Pushing value '{value_to_send}' to cloud variable '{cloud_var_name}' for account '{rule['account']}'")
                        cloud_client[cloud_var_name] = value_to_send
                    else:
                        logging.warning(f"Could not determine value to send for '{cloud_var_name}' from payload '{payload}' on topic '{msg.topic}'.")

                except ValueError as ve:
                    logging.error(f"Data type conversion error for rule {rule} on topic {msg.topic}: {ve}. Payload: '{payload}'", exc_info=True)
                except Exception as e:
                    logging.error(f"Error processing rule {rule} for topic {msg.topic}: {e}. Payload: '{payload}'", exc_info=True)
        else:
            logging.info(f"No routing rule found for topic: {msg.topic}")

    except Exception as e:
        logging.error(f"Error processing main local message from topic '{msg.topic}': {e}", exc_info=True)


# --- 4. MAIN EXECUTION BLOCK ---
if __name__ == "__main__":
    logging.info("===================================================")
    logging.info("   McMaster Smart Classroom Hub (Dynamic) Starting   ")
    logging.info("===================================================")

    try:
        with open("config.yaml", 'r') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        logging.error("FATAL: config.yaml not found! Please ensure it's in the same directory. Exiting.")
        time.sleep(10)
        sys.exit(1)
    except yaml.YAMLError as ye:
        logging.error(f"FATAL: Error parsing config.yaml: {ye}. Please check YAML syntax. Exiting.")
        time.sleep(10)
        sys.exit(1)

    # --- Dynamically Setup Cloud Clients from Config ---
    for acc_name, acc_config in config.items():
        # Ensure it's a dictionary and enabled before processing as an account
        if isinstance(acc_config, dict) and acc_config.get('enabled', False):
            try:
                logging.info(f"Initializing cloud client for account '{acc_name}' (Device ID: {acc_config.get('device_id', 'N/A')})...")
                client = ArduinoCloudClient(
                    device_id=acc_config['device_id'],
                    username=acc_config['device_id'], # Username is typically the device_id
                    password=bytes(acc_config['secret_key'], 'utf-8')
                )

                for var_config in acc_config.get('variables', []):
                    # Register the variable with the Arduino Cloud Client
                    var_name = var_config['name']
                    direction = var_config['direction']
                    # Pass the full var_config to the callback factory so it has all necessary details
                    if direction in ["FROM_CLOUD", "BIDIRECTIONAL"]:
                        # When a variable is FROM_CLOUD or BIDIRECTIONAL, it means the cloud can WRITE to it.
                        # So, we register an on_write callback that calls our generic handler.
                        callback = lambda c, v, vc=var_config: generic_on_write_callback(c, v, vc)
                        client.register(var_name, on_write=callback)
                        logging.info(f"   Registered cloud variable '{var_name}' (Direction: {direction}, Type: {var_config['type']}) with on_write callback.")
                    else: # TO_CLOUD variables only send data, no cloud commands received
                        client.register(var_name)
                        logging.info(f"   Registered cloud variable '{var_name}' (Direction: {direction}, Type: {var_config['type']}).")

                    # Build the MQTT routing map for messages coming FROM local devices TO the cloud
                    if direction in ["TO_CLOUD", "BIDIRECTIONAL"]:
                        topic = var_config['topic']
                        logging.info(f"   Mapping local MQTT topic '{topic}' to cloud variable '{var_name}' for status updates.")

                        # Initialize list for topic if it doesn't exist
                        if topic not in mqtt_to_cloud_map:
                            mqtt_to_cloud_map[topic] = []
                        # Create a copy of var_config and add account name for routing
                        rule = var_config.copy()
                        rule['account'] = acc_name
                        mqtt_to_cloud_map[topic].append(rule)

                cloud_clients[acc_name] = client # Store the initialized client instance
                CloudWorker(acc_name, client).start() # Start a thread for each cloud client
            except KeyError as ke:
                logging.error(f"Configuration error for account '{acc_name}': Missing required key '{ke}'. Please check config.yaml.", exc_info=True)
            except Exception as e:
                logging.error(f"Failed to initialize cloud client for '{acc_name}': {e}", exc_info=True)

    # --- Setup and connect the Local MQTT Client (to Mosquitto) ---
    local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="SmartClassroomHub")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local

    # Main loop to keep the local MQTT client connected and running
    while True:
        try:
            if not local_client.is_connected():
                logging.info("Attempting to connect to local Mosquitto service...")
                local_client.connect("localhost", 1883, 60) # Connect to default Mosquitto port
                local_client.loop_start() # Start the MQTT client loop in a non-blocking way
            time.sleep(2) # Short delay before checking connection status again
        except KeyboardInterrupt:
            logging.info("Shutdown initiated by user. Exiting application.")
            break # Exit loop on Ctrl+C
        except Exception as e:
            logging.warning(f"Local Mosquitto connection failed or encountered an error: {e}. Retrying in 5s...", exc_info=True)
            time.sleep(5)

    # Clean up on shutdown
    if local_client.is_connected():
        local_client.loop_stop() # Stop the MQTT client loop
    local_client.disconnect() # Disconnect from Mosquitto
    logging.info("Disconnected from local Mosquitto service.")

    # Stop all cloud client threads (though daemon threads will exit with main)
    for acc_name, client in cloud_clients.items():
        try:
            client.stop() # Explicitly stop each Arduino Cloud client
            logging.info(f"Stopped cloud client for account '{acc_name}'.")
        except Exception as e:
            logging.error(f"Error stopping cloud client for '{acc_name}': {e}", exc_info=True)

    logging.info("Hub application has stopped successfully.")