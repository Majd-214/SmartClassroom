/*
  ====================================================================
  I2C Door Controller for McMaster Smart Classroom (v2)
  ====================================================================
  - This code runs on a dedicated Arduino (e.g., Nano/Uno) which
    controls all hardware for the smart door.
  - It communicates with the main ESP8266 via the I2C protocol.
  - It manages:
    - A 3x4 matrix keypad for local input.
    - Two servos for the door locks.
    - A door contact switch for status.
  - I2C Address: 0x08
  ====================================================================
*/

#include <Wire.h>
#include <Keypad.h>
#include <Servo.h>

// --- I2C Configuration ---
const int I2C_ADDRESS = 0x08;

// --- Component Pin Definitions ---
const int SERVO_1_PIN = 3;
const int SERVO_2_PIN = 11;
const int DOOR_SWITCH_PIN = 13;

// --- Keypad Configuration ---
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {8, 9, 10};

// --- Object Declarations ---
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo lock1;
Servo lock2;

// --- Global State Variables ---
volatile bool isLocked = true;
bool isDoorOpen = false;

// --- I2C Command Definitions ---
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

    lock1.attach(SERVO_1_PIN);
    lock2.attach(SERVO_2_PIN);
    updateServos(); // Set to initial 'locked' state

    pinMode(DOOR_SWITCH_PIN, INPUT_PULLUP);

    Wire.begin(I2C_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    Serial.println("Door Controller Online.");
}

// ====================================================================
//                              LOOP
// ====================================================================
void loop()
{
    char key = customKeypad.getKey();
    if (key)
    {
        if (key == '#')
            isLocked = true;
        else if (key == '*')
            isLocked = false;
    }

    isDoorOpen = digitalRead(DOOR_SWITCH_PIN);
    updateServos();
    delay(20);
}

// ====================================================================
//                          HELPER FUNCTIONS
// ====================================================================

void updateServos()
{
    lock1.write(isLocked ? 180 : 0);
    lock2.write(isLocked ? 0 : 180); // Mirrored
}

// ====================================================================
//                        I2C EVENT HANDLERS
// ====================================================================

void receiveEvent(int numBytes)
{
    while (Wire.available())
    {
        byte command = Wire.read();
        if (command == CMD_UNLOCK)
            isLocked = false;
        else if (command == CMD_LOCK)
            isLocked = true;
    }
}

void requestEvent()
{
    byte statusByte = 0;
    if (isDoorOpen)
        statusByte |= (1 << 0);
    if (isLocked)
        statusByte |= (1 << 1);
    Wire.write(statusByte);
}