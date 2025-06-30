/*
====================================================================
  I2C Door Controller for McMaster Smart Classroom
====================================================================
  - This code runs on a dedicated Arduino (e.g., Nano/Uno) which
    controls all hardware for the smart door.
  - It communicates with the main ESP8266 via the I2C protocol.
  - It manages:
    - A 3x4 matrix keypad for local input.
    - Two servos for the door locks.
    - A door contact switch for status.
    - A 3-color tower light for visual feedback.
  - I2C Address: 0x08
====================================================================
*/

#include <Wire.h>
#include <Keypad.h>
#include <Servo.h>

// --- I2C Configuration ---
const int I2C_ADDRESS = 0x08;

// --- Component Pin Definitions ---
const int SERVO_1_PIN = 9;
const int SERVO_2_PIN = 10;
const int DOOR_SWITCH_PIN = 2;
const int TOWER_LIGHT_RED_PIN = 5;
const int TOWER_LIGHT_WHITE_PIN = 6;
const int TOWER_LIGHT_BLUE_PIN = 7;

// --- Keypad Configuration ---
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
// These pins connect to the 7 pins of the keypad
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {13, 12, 11};

// --- Object Declarations ---
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo lock1;
Servo lock2;

// --- Global State Variables ---
bool isLocked = true;
bool isDoorOpen = false;
char lastKey = NO_KEY;

// --- I2C Command Definitions ---
// These are commands the ESP8266 can send TO this controller
enum I2C_COMMANDS
{
    CMD_UNLOCK = 1,
    CMD_LOCK = 2
};

// ====================================================================
//                             SETUP
// ====================================================================
void setup()
{
    Serial.begin(9600);
    Serial.println("I2C Door Controller Initializing...");

    // Initialize Servos
    lock1.attach(SERVO_1_PIN);
    lock2.attach(SERVO_2_PIN);
    updateServos(); // Set to initial 'locked' state

    // Initialize Door Switch Pin
    pinMode(DOOR_SWITCH_PIN, INPUT_PULLUP);

    // Initialize Tower Light Pins
    pinMode(TOWER_LIGHT_RED_PIN, OUTPUT);
    pinMode(TOWER_LIGHT_WHITE_PIN, OUTPUT);
    pinMode(TOWER_LIGHT_BLUE_PIN, OUTPUT);
    updateTowerLight(); // Set initial light state

    // Initialize I2C Communication
    Wire.begin(I2C_ADDRESS);
    Wire.onReceive(receiveEvent); // Register function to handle incoming commands
    Wire.onRequest(requestEvent); // Register function to handle data requests

    Serial.println("Door Controller Online.");
}

// ====================================================================
//                              LOOP
// ====================================================================
void loop()
{
    // 1. Check for local keypad input
    char key = customKeypad.getKey();
    if (key)
    {
        lastKey = key;
        Serial.print("Keypad pressed: ");
        Serial.println(key);
        // Simple lock/unlock logic based on '#' and '*'
        if (key == '#')
        {
            isLocked = true;
        }
        else if (key == '*')
        {
            isLocked = false;
        }
    }

    // 2. Read the door contact switch
    // (INPUT_PULLUP means LOW is closed, HIGH is open)
    isDoorOpen = digitalRead(DOOR_SWITCH_PIN);

    // 3. Update hardware based on current state
    updateServos();
    updateTowerLight();

    delay(20); // Small delay for stability
}

// ====================================================================
//                          HELPER FUNCTIONS
// ====================================================================

void updateServos()
{
    // Move servos to the position defined by the 'isLocked' state
    lock1.write(isLocked ? 180 : 0);
    lock2.write(isLocked ? 0 : 180); // Mirrored
}

void updateTowerLight()
{
    // Update the tower light based on the system state
    if (isDoorOpen && isLocked)
    {
        // SECURITY ALERT: Door is open but it's supposed to be locked!
        digitalWrite(TOWER_LIGHT_RED_PIN, (millis() / 500) % 2); // Flashing Red
        digitalWrite(TOWER_LIGHT_WHITE_PIN, LOW);
        digitalWrite(TOWER_LIGHT_BLUE_PIN, LOW);
    }
    else if (isLocked)
    {
        // Locked and secure
        digitalWrite(TOWER_LIGHT_RED_PIN, HIGH); // Solid Red
        digitalWrite(TOWER_LIGHT_WHITE_PIN, LOW);
        digitalWrite(TOWER_LIGHT_BLUE_PIN, LOW);
    }
    else
    {
        // Unlocked
        digitalWrite(TOWER_LIGHT_RED_PIN, LOW);
        digitalWrite(TOWER_LIGHT_WHITE_PIN, LOW);
        digitalWrite(TOWER_LIGHT_BLUE_PIN, HIGH); // Solid Blue
    }
}

// ====================================================================
//                        I2C EVENT HANDLERS
// ====================================================================

// This function is called whenever the ESP8266 sends a command
void receiveEvent(int numBytes)
{
    while (Wire.available())
    {
        byte command = Wire.read();
        Serial.print("Received I2C Command: ");
        Serial.println(command);

        if (command == CMD_UNLOCK)
        {
            isLocked = false;
        }
        else if (command == CMD_LOCK)
        {
            isLocked = true;
        }
    }
}

// This function is called whenever the ESP8266 requests data
void requestEvent()
{
    /*
     * We will send back a single byte with status information.
     * We use bit masking to pack two boolean values into one byte.
     * Bit 0: Door Open state (1 if open, 0 if closed)
     * Bit 1: Lock state (1 if locked, 0 if unlocked)
     */
    byte statusByte = 0;
    if (isDoorOpen)
    {
        statusByte |= (1 << 0); // Set bit 0
    }
    if (isLocked)
    {
        statusByte |= (1 << 1); // Set bit 1
    }

    Wire.write(statusByte);
    Serial.print("Sent Status Byte to ESP8266: ");
    Serial.println(statusByte, BIN);
}
