# ====================================================================
#   McMaster Engineering Summer Camp - Smart Classroom IoT Hub
#   PRODUCTION VERSION 2.5.0 - INTEGRATED LIGHT OBJECTS - MAJD ABURAS 2025
#
#   This script is a generic engine. All logic for variable
#   mapping and message routing is loaded from config.yaml at runtime.
#
#   This script is licensed under the Creative Commons
#   Attribution-NonCommercial-ShareAlike 4.0 International License.
#   To view a copy of this license, visit:
#   https://creativecommons.org/licenses/by-nc-sa/4.0/
# ====================================================================

# Import Communication Libraries
from arduino_iot_cloud import ArduinoCloudClient, ColoredLight, DimmedLight
import paho.mqtt.client as mqtt

# Import System Libraries
import sys
import time
import logging
import threading

# Import Data Libraries
import yaml
import json

# --- 1. LOGGING & GLOBAL SETUP ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s')
local_client = None
cloud_clients = {}  # Dictionary to hold all cloud clients
mqtt_to_cloud_map = {}  # Our dynamic routing table

# Default MQTT Broker Configuration (can be overridden by config.yaml)
DEFAULT_MQTT_BROKER_HOST = "localhost"
DEFAULT_MQTT_BROKER_PORT = 1883

# Valid variable types and directions for configuration validation
VALID_VAR_TYPES = ["String", "Float", "Integer", "Boolean", "Dimmed", "Color"]
VALID_DIRECTIONS = ["TO_CLOUD", "FROM_CLOUD", "BIDIRECTIONAL"]


# --- 2. DYNAMIC CALLBACK & WORKER CLASSES ---
class CloudWorker(threading.Thread):
    """A self-healing thread to manage a connection to the Arduino Cloud."""

    def __init__(self, account_name, client_instance):
        # Pass name to the super constructor for proper thread naming
        super().__init__(name=account_name)
        self.client = client_instance
        self.daemon = True # Daemon threads exit automatically when the main program exits

    def run(self):
        while True:
            try:
                logging.info(f"Starting connection loop for account '{self.name}'...")
                self.client.start() # This blocks until client.stop() is called or connection fails
            except Exception as e:
                logging.error(f"Connection loop for account '{self.name}' crashed: {e}", exc_info=True)
                time.sleep(15) # Wait before attempting to restart connection


def generic_on_write_callback(client, value, variable_config):
    """
    A generic callback factory for all actuator variables.
    This function is called when a command is received from the Arduino Cloud.
    It formats the payload and publishes it to the local MQTT broker
    for the student devices to receive.
    """
    cloud_var_name = variable_config['name']
    base_topic = variable_config['topic'] # This is the base topic from config.yaml
    var_type = variable_config['type']
    direction = variable_config['direction'] # Get direction from config to determine correct command topic

    logging.info(f"Cloud command received for '{cloud_var_name}': {value} (Type: {var_type})")

    payload = ""
    try:
        # Format payload based on the variable type expected by the local device
        if var_type == "Boolean":
            payload = "true" if value else "false" # Send "true"/"false" strings for C++ bool parsing
        elif var_type == "Color":
            # The 'value' is a ColoredLight object. Create a dictionary from its properties.
            if value.swi is None or value.bri is None or value.hue is None or value.sat is None:
                logging.warning(f"Incomplete ColoredLight data for '{cloud_var_name}'. Not publishing.")
                return
            payload_dict = {"swi": value.swi, "bri": int(value.bri), "hue": int(value.hue), "sat": int(value.sat)}
            payload = json.dumps(payload_dict)
        elif var_type == "Dimmed":
            # The 'value' is a DimmedLight object. Create a dictionary from its properties.
            if value.swi is None or value.bri is None:
                logging.warning(f"Incomplete DimmedLight data for '{cloud_var_name}'. Not publishing.")
                return
            payload_dict = {"swi": value.swi, "bri": int(value.bri)}
            payload = json.dumps(payload_dict)
        else:  # Default to sending the raw value as a string for other types (Float, Integer, String)
            payload = str(value)
    except Exception as e:
        logging.error(f"Error formatting payload for '{cloud_var_name}': {e}", exc_info=True)
        return

    # Publish to local MQTT if connected and direction allows
    if local_client and local_client.is_connected():
        if direction in ["FROM_CLOUD", "BIDIRECTIONAL"]:
            logging.info(f"Publishing to local topic: '{base_topic}' with payload: '{payload}'")
            local_client.publish(base_topic, payload)
        else: # This callback should only be triggered for FROM_CLOUD or BIDIRECTIONAL variables
            logging.warning(f"Unexpected direction '{direction}' for cloud command callback for {cloud_var_name}. Not publishing to local MQTT.")
            return
    else:
        logging.warning(f"Local MQTT client not connected. Could not send command for '{cloud_var_name}'.")


# --- 3. LOCAL MQTT CLIENT SETUP ---
def on_connect_local(client, userdata, flags, rc, properties=None):
    """Callback for when the local MQTT client connects to Mosquitto."""
    if rc == 0:
        logging.info("Connection to Local Mosquitto Service SUCCEEDED.")
        # Dynamically subscribe to all topics defined in the config that are TO_CLOUD or BIDIRECTIONAL
        # These are topics the bridge needs to listen to for incoming device data.
        for topic in mqtt_to_cloud_map.keys():
            logging.info(f"Subscribing to local topic: {topic}")
            client.subscribe(topic)
    else:
        logging.error(f"Failed to connect to local Mosquitto service, return code {rc}")

def on_disconnect_local(client, userdata, rc):
    """Callback for when the local MQTT client disconnects from Mosquitto."""
    if rc == 0:
        logging.info("Disconnected from local Mosquitto service cleanly.")
    else:
        logging.warning(f"Local MQTT client disconnected unexpectedly (RC: {rc}).")
        # The loop_start() method will automatically attempt to reconnect

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

                    # Convert the incoming payload based on the variable type defined in config.yaml
                    if var_type == "Boolean":
                        value_to_send = (payload.lower() == 'true')
                    elif var_type == "Float":
                        value_to_send = float(payload)
                    elif var_type == "Integer":
                        value_to_send = int(payload)
                    elif var_type == "Color" or var_type == "Dimmed":
                        # Payload is a JSON string, convert to a dictionary
                        try:
                            value_to_send = json.loads(payload)
                            if not isinstance(value_to_send, dict):
                                logging.error(f"Payload for {var_type} on topic '{msg.topic}' is not a JSON object: '{payload}'")
                                value_to_send = None
                        except json.JSONDecodeError:
                            logging.error(f"Failed to decode JSON for {var_type} type from payload '{payload}' on topic '{msg.topic}'.", exc_info=True)
                            value_to_send = None
                    elif var_type == "String":
                        value_to_send = payload
                    else:
                        logging.warning(f"Unsupported variable type '{var_type}'. Sending raw payload as string.")
                        value_to_send = payload

                    # Now, push the value to the cloud
                    if value_to_send is not None:
                        if var_type == "Color" or var_type == "Dimmed":
                            # For light objects, update properties from the dictionary
                            light_obj = cloud_client[cloud_var_name]
                            logging.info(f"Updating cloud light object '{cloud_var_name}' with data: {value_to_send}")
                            if 'swi' in value_to_send:
                                light_obj.swi = bool(value_to_send['swi'])
                            if 'bri' in value_to_send:
                                light_obj.bri = int(value_to_send['bri'])
                            if var_type == "Color":
                                if 'hue' in value_to_send:
                                    light_obj.hue = int(value_to_send['hue'])
                                if 'sat' in value_to_send:
                                    light_obj.sat = int(value_to_send['sat'])
                        else:
                            # For simple types, just assign the value
                            logging.info(f"Pushing value '{value_to_send}' to cloud variable '{cloud_var_name}' for account '{rule['account']}'")
                            cloud_client[cloud_var_name] = value_to_send
                    else:
                        logging.warning(f"Could not determine valid value to send for '{cloud_var_name}' from payload '{payload}' on topic '{msg.topic}'.")

                except ValueError as ve:
                    logging.error(f"Data type conversion error for rule {rule} on topic {msg.topic}: {ve}. Payload: '{payload}'", exc_info=True)
                except Exception as e:
                    logging.error(f"Error processing rule {rule} for topic {msg.topic}: {e}. Payload: '{payload}'", exc_info=True)
        else:
            logging.debug(f"No routing rule found for topic: {msg.topic}. Message ignored.")

    except Exception as e:
        logging.error(f"Error processing main local message from topic '{msg.topic}': {e}", exc_info=True)


# --- 4. MAIN EXECUTION BLOCK ---
if __name__ == "__main__":
    logging.info("===================================================")
    logging.info("   McMaster Smart Classroom Hub (Dynamic) Starting   ")
    logging.info("===================================================")

    # --- Load Configuration ---
    try:
        with open("config.yaml", 'r') as f:
            full_config = yaml.safe_load(f)
            if not isinstance(full_config, dict):
                raise ValueError("Config file content is not a valid dictionary.")
    except FileNotFoundError:
        logging.error("FATAL: config.yaml not found! Please ensure it's in the same directory. Exiting.")
        time.sleep(10)
        sys.exit(1)
    except yaml.YAMLError as ye:
        logging.error(f"FATAL: Error parsing config.yaml: {ye}. Please check YAML syntax. Exiting.")
        time.sleep(10)
        sys.exit(1)
    except ValueError as ve:
        logging.error(f"FATAL: Invalid config.yaml format: {ve}. Exiting.")
        time.sleep(10)
        sys.exit(1)


    # --- Get Local MQTT Broker Configuration ---
    mqtt_broker_host = full_config.get('local_mqtt_broker', {}).get('host', DEFAULT_MQTT_BROKER_HOST)
    mqtt_broker_port = full_config.get('local_mqtt_broker', {}).get('port', DEFAULT_MQTT_BROKER_PORT)
    logging.info(f"Local MQTT Broker configured as: {mqtt_broker_host}:{mqtt_broker_port}")


    # --- Dynamically Setup Cloud Clients from Config ---
    # Iterate through all top-level keys in the config, treating them as accounts
    for acc_name, acc_config in full_config.items():
        # Skip the 'local_mqtt_broker' key as it's not an account configuration
        if acc_name == 'local_mqtt_broker':
            continue

        # Ensure it's a dictionary and enabled before processing as an account
        if isinstance(acc_config, dict) and acc_config.get('enabled', False):
            try:
                # Basic validation for essential keys
                if not acc_config.get('device_id') or not acc_config.get('secret_key'):
                    logging.error(f"Skipping account '{acc_name}': 'device_id' or 'secret_key' is missing or empty in config.yaml. Please provide both for enabled accounts.")
                    continue

                logging.info(f"Initializing cloud client for account '{acc_name}' (Device ID: {acc_config.get('device_id', 'N/A')})...")
                client = ArduinoCloudClient(
                    device_id=acc_config['device_id'],
                    username=acc_config['device_id'], # Username is typically the device_id
                    password=bytes(acc_config['secret_key'], 'utf-8')
                )

                for var_config in acc_config.get('variables', []):
                    var_name = var_config.get('name')
                    direction = var_config.get('direction')
                    var_type = var_config.get('type')
                    topic = var_config.get('topic')

                    # Validate critical variable configuration parameters
                    if not all([var_name, direction, var_type, topic]):
                        logging.error(f"Skipping variable in account '{acc_name}': Missing 'name', 'direction', 'type', or 'topic' in variable config: {var_config}")
                        continue
                    if direction not in VALID_DIRECTIONS:
                        logging.error(f"Skipping variable '{var_name}' in account '{acc_name}': Invalid 'direction' '{direction}'. Must be one of {VALID_DIRECTIONS}.")
                        continue
                    if var_type not in VALID_VAR_TYPES:
                        logging.error(f"Skipping variable '{var_name}' in account '{acc_name}': Invalid 'type' '{var_type}'. Must be one of {VALID_VAR_TYPES}.")
                        continue

                    # Create the on_write callback (used for FROM_CLOUD and BIDIRECTIONAL)
                    callback = None
                    if direction in ["FROM_CLOUD", "BIDIRECTIONAL"]:
                        # When a variable can be written from the cloud, we need a callback.
                        # The lambda captures the variable's specific configuration.
                        callback = lambda c, v, vc=var_config: generic_on_write_callback(c, v, vc)

                    # Register the variable based on its type
                    if var_type == "Color":
                        client.register(ColoredLight(var_name, on_write=callback))
                        logging.info(f"   Registered cloud variable '{var_name}' as ColoredLight (Direction: {direction}).")
                    elif var_type == "Dimmed":
                        client.register(DimmedLight(var_name, on_write=callback))
                        logging.info(f"   Registered cloud variable '{var_name}' as DimmedLight (Direction: {direction}).")
                    else: # For all other types (String, Float, Integer, Boolean)
                        if callback:
                            client.register(var_name, on_write=callback)
                            logging.info(f"   Registered cloud variable '{var_name}' (Direction: {direction}, Type: {var_type}) with on_write callback.")
                        else: # TO_CLOUD only
                            client.register(var_name)
                            logging.info(f"   Registered cloud variable '{var_name}' (Direction: {direction}, Type: {var_type}).")

                    # Build the MQTT routing map for messages coming FROM local devices TO the cloud
                    if direction in ["TO_CLOUD", "BIDIRECTIONAL"]:
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
                logging.error(f"Configuration error for account '{acc_name}': Missing required key '{ke}'. Please check config.yaml syntax.", exc_info=True)
            except Exception as e:
                logging.error(f"Failed to initialize cloud client for '{acc_name}': {e}", exc_info=True)

    # --- Setup and connect the Local MQTT Client (to Mosquitto) ---
    local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="SmartClassroomHub")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local
    local_client.on_disconnect = on_disconnect_local # Add the disconnect callback

    # Main loop to keep the local MQTT client connected and running
    while True:
        try:
            if not local_client.is_connected():
                logging.info(f"Attempting to connect to local Mosquitto service at {mqtt_broker_host}:{mqtt_broker_port}...")
                local_client.connect(mqtt_broker_host, mqtt_broker_port, 60) # Connect using dynamic host/port
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