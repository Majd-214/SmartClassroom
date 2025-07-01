/*
====================================================================
  I2C Stepper Curtain Controller for McMaster Smart Classroom
====================================================================
  - This code runs on a dedicated Arduino (e.g., Nano/Uno) to
    manage a stepper motor for the smart curtain.
  - It uses the AccelStepper library for smooth motion.
  - It calibrates its movement range using two photointerrupters
    as top and bottom limit switches.
  - It receives percentage-based commands (0-100) from the ESP8266
    via I2C to set the curtain position.
  - I2C Address: 0x09
====================================================================
*/

#include <Wire.h>
#include <AccelStepper.h>

// --- I2C Configuration ---
const int I2C_ADDRESS = 0x09;

// --- Stepper Motor Driver Pin Definitions (for stepper driver) ---
const int STEP_PIN = 3;
const int DIR_PIN = 4;

// --- Limit Switch Pin Definitions ---
const int TOP_LIMIT_PIN = 2;    // Photo-interrupter at the top
const int BOTTOM_LIMIT_PIN = 5; // Photo-interrupter at the bottom

// --- Stepper Motor Configuration ---
const int STEPS_PER_REVOLUTION = 200;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- State and Calibration Variables ---
bool isCalibrated = false;
long topPosition = 0;
long bottomPosition = 0;
int currentPercent = 0;

// --- I2C Command Definitions ---
enum I2C_COMMANDS
{
    CMD_CALIBRATE = 'C',
    CMD_SET_PERCENT = 'P'
};

// ====================================================================
//                             SETUP
// ====================================================================
void setup()
{
    Serial.begin(9600);
    Serial.println("I2C Stepper Controller Initializing...");

    // Configure limit switch pins
    pinMode(TOP_LIMIT_PIN, INPUT_PULLUP);
    pinMode(BOTTOM_LIMIT_PIN, INPUT_PULLUP);

    // Configure Stepper
    stepper.setMaxSpeed(500);
    stepper.setAcceleration(200);

    // Initialize I2C
    Wire.begin(I2C_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    Serial.println("Waiting for Calibration command from ESP8266...");
}

// ====================================================================
//                              LOOP
// ====================================================================
void loop()
{
    // The AccelStepper library requires run() to be called as often
    // as possible to generate step pulses.
    if (isCalibrated)
    {
        stepper.run();
    }
}

// ====================================================================
//                        CALIBRATION & MOVEMENT
// ====================================================================

void calibrate()
{
    Serial.println("Starting calibration cycle...");
    isCalibrated = false;

    // 1. Move down to find the bottom limit
    stepper.setSpeed(-400); // Negative speed for 'down'
    while (digitalRead(BOTTOM_LIMIT_PIN) == HIGH)
    {
        stepper.runSpeed();
    }
    bottomPosition = stepper.currentPosition();
    Serial.print("Bottom limit found at: ");
    Serial.println(bottomPosition);

    // 2. Move up to find the top limit
    stepper.setSpeed(400); // Positive speed for 'up'
    while (digitalRead(TOP_LIMIT_PIN) == HIGH)
    {
        stepper.runSpeed();
    }
    stepper.setCurrentPosition(0); // Set this top position as our '0' reference
    topPosition = 0;
    Serial.println("Top limit found at: 0");

    // 3. Calculate the full range
    long totalRange = bottomPosition - topPosition;
    bottomPosition = totalRange; // Now bottomPosition is relative to top

    Serial.print("Calibration complete. Total range: ");
    Serial.println(totalRange);
    isCalibrated = true;

    // Move to 0% (top) to finish
    moveToPercent(0);
}

void moveToPercent(int percent)
{
    if (!isCalibrated)
        return;

    currentPercent = constrain(percent, 0, 100);
    long targetPosition = map(currentPercent, 0, 100, topPosition, bottomPosition);

    stepper.moveTo(targetPosition);
    Serial.print("Moving to ");
    Serial.print(currentPercent);
    Serial.print("% (Position: ");
    Serial.print(targetPosition);
    Serial.println(")");
}

// ====================================================================
//                        I2C EVENT HANDLERS
// ====================================================================

void receiveEvent(int numBytes)
{
    if (Wire.available() < 1)
        return;

    byte command = Wire.read(); // Read the command character

    if (command == CMD_CALIBRATE)
    {
        calibrate();
    }
    else if (command == CMD_SET_PERCENT)
    {
        if (Wire.available() > 0)
        {
            byte percent = Wire.read(); // Read the percentage value
            moveToPercent(percent);
        }
    }
}

void requestEvent()
{
    // When the ESP asks for data, send back the current percentage
    Wire.write((byte)currentPercent);
}
