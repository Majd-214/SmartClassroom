/*
====================================================================
  I2C Stepper Curtain Controller for McMaster Smart Classroom
====================================================================
  - This code runs on a dedicated Arduino (e.g., Nano/Uno) to
    manage a stepper motor for the smart curtain.
  - It uses the AccelStepper library for smooth motion.
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
const int STEP_PIN = 2;
const int DIR_PIN = 3;
const int EN_PIN = 4;
const int LM_PIN = 11;

// --- Stepper Motor Configuration ---
int calibration = 0;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- State and Calibration Variables ---
int pos = 0;

// ====================================================================
//                             SETUP
// ====================================================================
void setup()
{
  Serial.begin(9600);
  Serial.println("I2C Stepper Controller Initializing...");

  // Configure Stepper
  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, true, true);
  stepper.setMaxSpeed(2400);
  stepper.setAcceleration(5600);

  // Configure Limit Switch
  pinMode(LM_PIN, INPUT);

  // Calibrate Curtain
  Serial.println("Calibrating...");
  calibrate();
  Serial.println("Calibration Complete!");

  // Initialize I2C
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);

  // Enable Curtain Motor
  stepper.enableOutputs();

  Serial.println("Waiting for I2C commands from ESP8266...");
}

// ====================================================================
//                              LOOP
// ====================================================================
void loop()
{
  stepper.run();
}

// ====================================================================
//                        I2C EVENT HANDLERS
// ====================================================================

void receiveEvent(int numBytes)
{
  if (!Wire.available()) return;

  pos = constrain((int)Wire.read(), 0, 100); // Read the command character

  Serial.print("Setting Curtain Position to: ");
  Serial.print(pos);
  Serial.println("%");

  stepper.moveTo(calibration * pos / 100);
}

// ====================================================================
//                            CALIBRATE
// ====================================================================
void calibrate()
{
  // Roll curtain up until limit switch no longer sees it
  stepper.moveTo(10000);
  while (!digitalRead(LM_PIN)) stepper.run();

  // Record motor position
  calibration = stepper.currentPosition();

  // Print calibration value
  Serial.print("Calibrated position to: ");
  Serial.print(calibration);
  Serial.println(" motor steps.");

  // Reset curatin to closed position
  stepper.moveTo(0);
  while (stepper.distanceToGo()) stepper.run();

  // Turn OFF the motor
  stepper.disableOutputs();
}
