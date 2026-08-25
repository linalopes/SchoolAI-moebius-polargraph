/*
  Moebius Polargraph - Two Stepper Motor Test

  This diagnostic sketch tests the two 28BYJ-48 stepper motors without
  using the pen-lift servo or the Polargraph geometry calculations.

  Each motor moves continuously between two opposite target positions.
  The different travel distances create a repeating motion pattern and
  make it easy to verify that both motors and both ULN2003 drivers work.

  Original Wall Drawing Machine project:
  https://github.com/shihaipeng03/Walldraw

  Required library: AccelStepper
  Install it from Arduino IDE > Tools > Manage Libraries.
*/

#include <AccelStepper.h>

// AccelStepper interface modes.
constexpr uint8_t FULL_STEP = 4;
constexpr uint8_t HALF_STEP = 8;

// Left motor (28BYJ-48 through a ULN2003 driver).
// Arduino pin 4 remains available for the original SD card interface.
constexpr uint8_t LEFT_IN1 = 2;
constexpr uint8_t LEFT_IN2 = 3;
constexpr uint8_t LEFT_IN3 = 5;
constexpr uint8_t LEFT_IN4 = 6;

// Right motor (28BYJ-48 through a ULN2003 driver).
constexpr uint8_t RIGHT_IN1 = 7;
constexpr uint8_t RIGHT_IN2 = 8;
constexpr uint8_t RIGHT_IN3 = 9;
constexpr uint8_t RIGHT_IN4 = 10;

// Initial travel distances from the original test sketch.
// Larger values produce larger movements. A greater difference between
// the two values produces a more complex repeating motion pattern.
constexpr long LEFT_TRAVEL_STEPS = 279;
constexpr long RIGHT_TRAVEL_STEPS = 673;

constexpr float MAX_SPEED = 800.0;
constexpr float ACCELERATION = 200.0;

// The 28BYJ-48 requires this pin order when driven in half-step mode.
AccelStepper leftMotor(
  HALF_STEP,
  LEFT_IN1,
  LEFT_IN3,
  LEFT_IN2,
  LEFT_IN4
);

AccelStepper rightMotor(
  HALF_STEP,
  RIGHT_IN1,
  RIGHT_IN3,
  RIGHT_IN2,
  RIGHT_IN4
);

void configureMotor(AccelStepper &motor, long firstTarget) {
  motor.setMaxSpeed(MAX_SPEED);
  motor.setAcceleration(ACCELERATION);
  motor.moveTo(firstTarget);
}

void reverseAtTarget(AccelStepper &motor) {
  if (motor.distanceToGo() == 0) {
    motor.moveTo(-motor.currentPosition());
  }
}

void setup() {
  Serial.begin(9600);

  configureMotor(leftMotor, LEFT_TRAVEL_STEPS);
  configureMotor(rightMotor, RIGHT_TRAVEL_STEPS);

  Serial.println(F("Moebius Polargraph - Two Stepper Motor Test"));
  Serial.println(F("Both motors should now move continuously."));
  Serial.println(F("This test runs until the Arduino is powered off or reset."));
}

void loop() {
  reverseAtTarget(leftMotor);
  reverseAtTarget(rightMotor);

  // run() must be called as frequently as possible for smooth acceleration.
  leftMotor.run();
  rightMotor.run();
}
