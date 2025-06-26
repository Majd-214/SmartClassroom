// 1. LIBRARY INCLUSIONS
#include <SmartDevice.h>
#include <IRremote.h>
#include <Servo.h>

using namespace SmartHome; // NAMESPACE COMMITMENT, DO NOT REMOVE THIS LINE

// 2. VARIABLE DEFINITIONS
const int SERVO_PIN = PIN_D5;  // First Servo Lock Pin
const int REMOTE_PIN = PIN_D6; // IR Remote Pin

// 3. SERVER CONFIGURATION
const char *DEVICE_NAME = "Fort_Knox"; // <-- CHANGE THIS!
const char *CURTAIN_TOPIC = "classroom/curtain";

// 4. OBJECT DECLARATIONS
SmartDevice myDevice;
Servo servo;

// 5. DEVICE FUNCTIONS
void setupDevice() // Runs ONCE at startup.
{
  servo.attach(SERVO_PIN);      // Attach the servo to the specified pin
  IrReceiver.begin(REMOTE_PIN); // Initialize the IR receiver on the specified pin
  servo.write(0);               // Start with the curtain open

  myDevice.subscribeTo(CURTAIN_TOPIC); // Subscribe to the curtain topic
}

// Tell Students to COPY-and-PASTE this function into their code!
int mapCodeToButton(unsigned long code)
{
  if ((code & 0x0000FFFF) == 0x0000BF00)
  {
    code >>= 16;
    if (((code >> 8) ^ (code & 0x00FF)) == 0x00FF)
      return code & 0xFF;
  }
  return -1;
}

// Tell Students to COPY-and-PASTE this function into their code!
int readInfrared()
{
  int result = -1;
  if (IrReceiver.decode())
  {
    result = mapCodeToButton(IrReceiver.decodedIRData.decodedRawData);
    IrReceiver.resume();
  }
  return result;
}

// NEW Helper Function
void curtainClosed(bool state, bool informServer)
{
  if (state)
    servo.write(180);
  else
    servo.write(0);
  if (informServer)
    myDevice.publishTo(CURTAIN_TOPIC, state ? "true" : "false");
}

void readSensor() // Runs REPEATEDLY in the main loop.
{
  int code = readInfrared(); // Read the IR remote code
  if (code == 8)
    curtainClosed(true, true);
  else if (code == 10)
    curtainClosed(false, true);
}

void triggerActuator(String topic, String command) // Runs ON_DEMAND when a command is received from the Smart Hub.
{
  curtainClosed(command == "true", false); // If the command is to close the curtain
}

/*
===============================================================
|                                                             |
|   +-----------------------------------------------------+   |
|   |                                                     |   |
|   |    --->>    DO NOT EDIT THE CODE BELOW!    <<---    |   |
|   |            (This is the system's engine)            |   |
|   |                                                     |   |
|   +-----------------------------------------------------+   |
|                                                             |
===============================================================
*/

// ---- System Engine Below ----

const char *WIFI_SSID = "McMasterIoT-Camp";
const char *WIFI_PASSWORD = "Roomba2025";
const char *MQTT_BROKER_IP = "192.168.0.147";

// System-level message handler that calls your actuator logic.
void system_onMessage(String topic, String payload) // Signature matches MessageCallback
{
  Serial.print("System received command on topic: ");
  Serial.print(topic);
  Serial.print(". Payload: ");
  Serial.println(payload);
  triggerActuator(topic, payload); // Calls your function above with topic and payload
}

// Main setup function that initializes the entire system.
void setup()
{
  Serial.begin(115200);
  Serial.println("System Engine: Booting device...");

  // Initialize the SmartDevice library and connect to the network.
  myDevice.begin(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP);

  // Register the system-level message handler.
  myDevice.onMessage(system_onMessage);

  // Call your custom hardware setup function
  setupDevice();
  Serial.println("System Engine: Custom hardware setup complete.");

  Serial.println("System Engine: Boot sequence complete. System is online.");
}

// Main loop that keeps the system running.
void loop()
{
  myDevice.update(); // MUST be here to keep connection alive.
  readSensor();      // Call your sensor reading and publishing logic.
  delay(1000);       // A 1-second delay to prevent spamming the network. Adjust if needed.
}