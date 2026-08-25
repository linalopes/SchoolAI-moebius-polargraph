/*
  Moebius Polargraph - Drawing Gallery

  A standalone gallery firmware for the calibrated Moebius Polargraph.
  It preserves the validated geometry, motor mapping, movement coordination,
  manual positioning, logical zero workflow, and pen angles.

  Gallery commands:
    1 - Heart
    2 - Butterfly curve
    3 - Ellipse
    4 - Rotated rectangle
    G - Four-drawing gallery

  This firmware is derived from the Wall Drawing Machine project:
  https://github.com/shihaipeng03/Walldraw

  Based on Walldraw, Copyright (c) 2021 shihaipeng03 (MIT License).
  Modifications Copyright (c) 2026 Lina Lopes.
  SPDX-License-Identifier: MIT
*/

#include <TinyStepper_28BYJ_48.h>
#include <Servo.h>

#define STEPS_PER_TURN  2048
#define SPOOL_DIAMETER  35
#define SPOOL_CIRC      (SPOOL_DIAMETER * 3.1416)
#define TPS             (SPOOL_CIRC / STEPS_PER_TURN)

#define STEP_DELAY_US   1

// Validated motor directions for this machine.
#define M1_REEL_OUT      1
#define M1_REEL_IN      -1
#define M2_REEL_OUT     -1
#define M2_REEL_IN       1

// Calibrated machine geometry in millimeters.
#define X_SEPARATION    820
#define LIMXMAX         ( X_SEPARATION * 0.5)
#define LIMXMIN         (-X_SEPARATION * 0.5)
#define LIMYMIN         520

// Calibrated pen-lift angles.
#define PEN_UP_ANGLE    90
#define PEN_DOWN_ANGLE  60

#define BAUD            115200
#define RAW_JOG_STEPS   20
#define PEN_SETTLE_MS   500

// Sampling resolution used by the parametric drawings.
#define CURVE_STEP      (PI / 90.0)

TinyStepper_28BYJ_48 m1;
TinyStepper_28BYJ_48 m2;
Servo pen;

static long laststep1 = 0;
static long laststep2 = 0;
static float posx = 0.0;
static float posy = 0.0;
static bool zeroIsSet = false;
static bool penIsDown = false;


// Convert a logical XY position to left and right cord lengths in steps.
// Logical +X points right and logical +Y points up toward the anchors.
void IK(float x, float y, long &l1, long &l2) {
  float dy = y - LIMYMIN;
  float dx = x - LIMXMIN;
  l1 = round(sqrt(dx * dx + dy * dy) / TPS);

  dx = x - LIMXMAX;
  l2 = round(sqrt(dx * dx + dy * dy) / TPS);
}


// Change the virtual position without moving the motors.
static void teleport(float x, float y) {
  posx = x;
  posy = y;

  long l1;
  long l2;
  IK(posx, posy, l1, l2);
  laststep1 = l1;
  laststep2 = l2;
}


// Move to one XY target using the validated cord directions and the original
// Bresenham-style coordination between the two motors.
void moveto(float x, float y) {
  long l1;
  long l2;
  IK(x, y, l1, l2);

  long d1 = l1 - laststep1;
  long d2 = l2 - laststep2;
  long ad1 = abs(d1);
  long ad2 = abs(d2);

  int dir1 = d1 > 0 ? M1_REEL_OUT : M1_REEL_IN;
  int dir2 = d2 > 0 ? M2_REEL_OUT : M2_REEL_IN;
  long over = 0;

  if (ad1 > ad2) {
    for (long i = 0; i < ad1; ++i) {
      m1.moveRelativeInSteps(dir1);
      over += ad2;

      if (over >= ad1) {
        over -= ad1;
        m2.moveRelativeInSteps(dir2);
      }

      delayMicroseconds(STEP_DELAY_US);
    }
  } else {
    for (long i = 0; i < ad2; ++i) {
      m2.moveRelativeInSteps(dir2);
      over += ad1;

      if (over >= ad2) {
        over -= ad2;
        m1.moveRelativeInSteps(dir1);
      }

      delayMicroseconds(STEP_DELAY_US);
    }
  }

  laststep1 = l1;
  laststep2 = l2;
  posx = x;
  posy = y;
}


// Split a Cartesian line into short targets so that it remains straight in XY
// space after the Polargraph inverse-kinematics conversion.
static void line_safe(float x, float y) {
  float dx = x - posx;
  float dy = y - posy;
  float len = sqrt(dx * dx + dy * dy);

  if (len <= TPS) {
    moveto(x, y);
    return;
  }

  long pieces = floor(len / TPS);
  float x0 = posx;
  float y0 = posy;

  for (long j = 0; j <= pieces; ++j) {
    float amount = (float)j / (float)pieces;
    moveto((x - x0) * amount + x0,
           (y - y0) * amount + y0);
  }

  moveto(x, y);
}


void raisePen() {
  pen.write(PEN_UP_ANGLE);
  penIsDown = false;
  delay(PEN_SETTLE_MS);
}


void lowerPen() {
  pen.write(PEN_DOWN_ANGLE);
  penIsDown = true;
  delay(PEN_SETTLE_MS);
}


// Move both motors directly while positioning the carriage manually.
// Raw jogging clears the logical zero because its XY position is unknown.
void rawJogPair(int direction1, int direction2) {
  if (penIsDown) {
    raisePen();
  }

  for (long i = 0; i < RAW_JOG_STEPS; ++i) {
    m1.moveRelativeInSteps(direction1);
    m2.moveRelativeInSteps(direction2);
    delayMicroseconds(STEP_DELAY_US);
  }

  zeroIsSet = false;
}


void printPosition() {
  Serial.print(F("Logical XY: "));
  Serial.print(posx, 3);
  Serial.print(F(", "));
  Serial.println(posy, 3);

  Serial.print(F("Logical cord steps: "));
  Serial.print(laststep1);
  Serial.print(F(", "));
  Serial.println(laststep2);

  Serial.print(F("Zero set: "));
  Serial.println(zeroIsSet ? F("YES") : F("NO"));
}


void printHelp() {
  Serial.println();
  Serial.println(F("=== Moebius Polargraph Gallery ==="));
  Serial.println(F("Set Serial Monitor to 115200 baud and Newline."));
  Serial.println();
  Serial.println(F("Before Z - raw physical positioning:"));
  Serial.println(F("  W : move physically UP"));
  Serial.println(F("  S : move physically DOWN"));
  Serial.println(F("  A : move physically LEFT"));
  Serial.println(F("  D : move physically RIGHT"));
  Serial.println();
  Serial.println(F("Gallery commands:"));
  Serial.println(F("  1 : draw a heart at logical zero"));
  Serial.println(F("  2 : draw a butterfly curve at logical zero"));
  Serial.println(F("  3 : draw an ellipse at logical zero"));
  Serial.println(F("  4 : draw a rotated rectangle at logical zero"));
  Serial.println(F("  G : draw all four as a 2 x 2 gallery"));
  Serial.println();
  Serial.println(F("Reference and utility commands:"));
  Serial.println(F("  Z : declare current physical position as logical (0,0)"));
  Serial.println(F("  C : return to logical (0,0)"));
  Serial.println(F("  U : pen up test (90 degrees)"));
  Serial.println(F("  N : pen down test (60 degrees)"));
  Serial.println(F("  P : print logical state"));
  Serial.println(F("  H : print this help"));
  Serial.println();
  Serial.println(F("Any W/S/A/D command clears the logical zero."));
}


bool requireZero() {
  if (zeroIsSet) {
    return true;
  }

  Serial.println(F("Command ignored: position the carriage and press Z first."));
  return false;
}


void finishDrawing() {
  raisePen();
  Serial.println(F("Returning to logical zero."));
  line_safe(0.0, 0.0);
  printPosition();
}


// Parametric heart adapted from the original WallDrawDemo firmware.
void drawHeart(float centerX, float centerY,
               float xScale, float yScale) {
  float startX = centerX;
  float startY = centerY + yScale * 7.0;

  raisePen();
  line_safe(startX, startY);
  lowerPen();

  for (float angle = 0.0; angle <= TWO_PI; angle += CURVE_STEP * 0.5) {
    float sine = sin(angle);
    float x = xScale * 15.0 * sine * sine * sine;
    float y = yScale * (15.0 * cos(angle)
                         - 5.0 * cos(2.0 * angle)
                         - 2.0 * cos(3.0 * angle)
                         - cos(4.0 * angle));

    line_safe(centerX + x, centerY + y);
  }

  line_safe(startX, startY);
  raisePen();
}


// Butterfly curve adapted from the original WallDrawDemo firmware.
void drawButterfly(float centerX, float centerY, int loops,
                   float xScale, float yScale) {
  float startY = centerY + yScale * 0.71828;

  raisePen();
  line_safe(centerX, startY);
  lowerPen();

  for (float angle = 0.0;
       angle < TWO_PI * loops;
       angle += CURVE_STEP) {
    float sineTerm = sin(angle / 12.0);
    float sineToFifth = sineTerm * sineTerm * sineTerm
                        * sineTerm * sineTerm;
    float radial = exp(cos(angle))
                   - 2.0 * cos(4.0 * angle)
                   + sineToFifth;

    float x = xScale * sin(angle) * radial;
    float y = yScale * cos(angle) * radial;
    line_safe(centerX + x, centerY + y);
  }

  raisePen();
}


void drawEllipse(float centerX, float centerY,
                 float radiusX, float radiusY) {
  raisePen();
  line_safe(centerX + radiusX, centerY);
  lowerPen();

  for (float angle = CURVE_STEP; angle < TWO_PI; angle += CURVE_STEP) {
    line_safe(centerX + cos(angle) * radiusX,
              centerY + sin(angle) * radiusY);
  }

  line_safe(centerX + radiusX, centerY);
  raisePen();
}


void drawRotatedRectangle(float centerX, float centerY,
                          float width, float height,
                          float angleDegrees) {
  float halfWidth = width * 0.5;
  float halfHeight = height * 0.5;
  float angle = radians(angleDegrees);
  float cosine = cos(angle);
  float sine = sin(angle);

  float localX[4] = {-halfWidth, halfWidth, halfWidth, -halfWidth};
  float localY[4] = {-halfHeight, -halfHeight, halfHeight, halfHeight};
  float x[4];
  float y[4];

  for (int i = 0; i < 4; ++i) {
    x[i] = centerX + localX[i] * cosine - localY[i] * sine;
    y[i] = centerY + localX[i] * sine + localY[i] * cosine;
  }

  raisePen();
  line_safe(x[0], y[0]);
  lowerPen();

  for (int i = 1; i < 4; ++i) {
    line_safe(x[i], y[i]);
  }

  line_safe(x[0], y[0]);
  raisePen();
}


void drawSingleHeart() {
  Serial.println(F("Drawing heart."));
  drawHeart(0.0, 0.0, 2.0, 2.0);
  finishDrawing();
}


void drawSingleButterfly() {
  Serial.println(F("Drawing butterfly curve."));
  drawButterfly(0.0, 0.0, 3, 10.0, 10.0);
  finishDrawing();
}


void drawSingleEllipse() {
  Serial.println(F("Drawing ellipse."));
  drawEllipse(0.0, 0.0, 35.0, 20.0);
  finishDrawing();
}


void drawSingleRotatedRectangle() {
  Serial.println(F("Drawing rotated rectangle."));
  drawRotatedRectangle(0.0, 0.0, 60.0, 35.0, 30.0);
  finishDrawing();
}


// Draw four compact examples inside the area validated by the earlier tests.
void drawGallery() {
  Serial.println(F("Gallery 1/4: heart."));
  drawHeart(-50.0, 45.0, 1.0, 1.0);

  Serial.println(F("Gallery 2/4: butterfly curve."));
  drawButterfly(50.0, 45.0, 3, 4.5, 4.5);

  Serial.println(F("Gallery 3/4: ellipse."));
  drawEllipse(-50.0, -45.0, 20.0, 12.0);

  Serial.println(F("Gallery 4/4: rotated rectangle."));
  drawRotatedRectangle(50.0, -45.0, 36.0, 22.0, 30.0);

  finishDrawing();
}


void setup() {
  Serial.begin(BAUD);

  // Preserve the validated motor pin mapping.
  m1.connectToPins(7, 8, 9, 10);
  m2.connectToPins(2, 3, 5, 6);

  // Preserve the values used by the validated drawing firmware.
  m1.setSpeedInStepsPerSecond(10000);
  m1.setAccelerationInStepsPerSecondPerSecond(100000);
  m2.setSpeedInStepsPerSecond(10000);
  m2.setAccelerationInStepsPerSecondPerSecond(100000);

  pen.attach(A0);
  raisePen();

  // The physical reference must be declared manually with Z.
  zeroIsSet = false;
  printHelp();
}


void loop() {
  if (!Serial.available()) {
    return;
  }

  char command = Serial.read();

  if (command == '\n' || command == '\r' || command == ' ') {
    return;
  }

  switch (command) {
    case 'W':
    case 'w':
      rawJogPair(M1_REEL_IN, M2_REEL_IN);
      Serial.println(F("Raw jog: physical UP. Zero cleared."));
      break;

    case 'S':
    case 's':
      rawJogPair(M1_REEL_OUT, M2_REEL_OUT);
      Serial.println(F("Raw jog: physical DOWN. Zero cleared."));
      break;

    case 'A':
    case 'a':
      rawJogPair(M1_REEL_IN, M2_REEL_OUT);
      Serial.println(F("Raw jog: physical LEFT. Zero cleared."));
      break;

    case 'D':
    case 'd':
      rawJogPair(M1_REEL_OUT, M2_REEL_IN);
      Serial.println(F("Raw jog: physical RIGHT. Zero cleared."));
      break;

    case 'Z':
    case 'z':
      teleport(0.0, 0.0);
      zeroIsSet = true;
      Serial.println(F("Current physical position declared as logical (0,0)."));
      printPosition();
      break;

    case 'C':
    case 'c':
      if (requireZero()) {
        finishDrawing();
      }
      break;

    case '1':
      if (requireZero()) {
        drawSingleHeart();
      }
      break;

    case '2':
      if (requireZero()) {
        drawSingleButterfly();
      }
      break;

    case '3':
      if (requireZero()) {
        drawSingleEllipse();
      }
      break;

    case '4':
      if (requireZero()) {
        drawSingleRotatedRectangle();
      }
      break;

    case 'G':
    case 'g':
      if (requireZero()) {
        drawGallery();
      }
      break;

    case 'U':
    case 'u':
      raisePen();
      Serial.println(F("Pen raised to 90 degrees."));
      break;

    case 'N':
    case 'n':
      lowerPen();
      Serial.println(F("Pen lowered to 60 degrees."));
      break;

    case 'P':
    case 'p':
      printPosition();
      break;

    case 'H':
    case 'h':
      printHelp();
      break;

    default:
      Serial.print(F("Unknown command: "));
      Serial.println(command);
      Serial.println(F("Press H for help."));
      break;
  }
}
