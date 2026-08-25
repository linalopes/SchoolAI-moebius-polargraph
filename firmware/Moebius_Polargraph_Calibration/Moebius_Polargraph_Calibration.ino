// WallDraw_Xuxu.ino
// Manual controller and four-shape drawing test for the WallDraw machine.
//
// Purpose:
// 1. Position the carriage manually with raw motor jogging.
// 2. Declare the known physical reference as logical (0, 0) with Z.
// 3. Draw four shapes around the logical origin with F.
//
// The firmware preserves the validated Cartesian IK, line subdivision,
// motor direction constants, and Bresenham-style coordination.

#include <TinyStepper_28BYJ_48.h>
#include <Servo.h>

#define STEPS_PER_TURN  2048
#define SPOOL_DIAMETER  35
#define SPOOL_CIRC      (SPOOL_DIAMETER * 3.1416)
#define TPS             (SPOOL_CIRC / STEPS_PER_TURN)

#define STEP_DELAY_US   1

// These direction constants are copied unchanged from WallDrawDemo.ino.
#define M1_REEL_OUT      1
#define M1_REEL_IN      -1
#define M2_REEL_OUT     -1
#define M2_REEL_IN       1

#define X_SEPARATION    820
#define LIMXMAX         ( X_SEPARATION * 0.5)
#define LIMXMIN         (-X_SEPARATION * 0.5)
#define LIMYMIN         520

#define PEN_UP_ANGLE    90
#define PEN_DOWN_ANGLE  60

#define BAUD            115200

// Each raw jog changes each selected motor by this many steps.
// With the configured spool, 20 steps correspond to about 1.07 mm of cord.
#define RAW_JOG_STEPS   20

#define PEN_SETTLE_MS   500

// Four-shape test geometry in millimeters.
#define SHAPE_OFFSET    60.0
#define CIRCLE_RADIUS   15.0
#define SHAPE_SIZE      30.0
#define CIRCLE_STEP     (PI / 90.0)

TinyStepper_28BYJ_48 m1;
TinyStepper_28BYJ_48 m2;
Servo pen;

static long laststep1 = 0;
static long laststep2 = 0;
static float posx = 0.0;
static float posy = 0.0;
static bool zeroIsSet = false;
static bool penIsDown = false;


// Convert a logical XY position directly to left and right cord lengths.
// The anchors are at (-410, +520) and (+410, +520), so logical +X is
// physically right and logical +Y is physically up toward the anchors.
void IK(float x, float y, long &l1, long &l2) {
  float dy = y - LIMYMIN;
  float dx = x - LIMXMIN;
  l1 = round(sqrt(dx * dx + dy * dy) / TPS);

  dx = x - LIMXMAX;
  l2 = round(sqrt(dx * dx + dy * dy) / TPS);
}


// Change only the virtual position. No motor movement occurs here.
static void teleport(float x, float y) {
  posx = x;
  posy = y;

  long l1;
  long l2;
  IK(posx, posy, l1, l2);
  laststep1 = l1;
  laststep2 = l2;
}


// Move to one XY target using direct cord-length direction selection and
// the original Bresenham-style coordination between the two motors.
void moveto(float x, float y) {
  long l1;
  long l2;
  IK(x, y, l1, l2);

  long d1 = l1 - laststep1;
  long d2 = l2 - laststep2;

  long ad1 = abs(d1);
  long ad2 = abs(d2);

  // A longer geometric cord length requires reeling OUT.
  // A shorter geometric cord length requires reeling IN.
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


// Split a Cartesian line into very small targets before calling moveto().
// This function preserves the subdivision behavior of WallDrawDemo.ino.
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
    float a = (float)j / (float)pieces;
    moveto((x - x0) * a + x0, (y - y0) * a + y0);
  }

  moveto(x, y);
}


// Raise the pen and wait for the servo mechanism to settle.
void raisePen() {
  pen.write(PEN_UP_ANGLE);
  penIsDown = false;
  delay(PEN_SETTLE_MS);
}


// Lower the pen and wait for the servo mechanism to settle.
void lowerPen() {
  pen.write(PEN_DOWN_ANGLE);
  penIsDown = true;
  delay(PEN_SETTLE_MS);
}


// Move both motors directly without using XY coordinates.
// Raw jogging invalidates any previously declared logical zero.
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
  Serial.println(F("=== WallDraw_Xuxu manual controller ==="));
  Serial.println(F("Set Serial Monitor to 115200 baud and Newline."));
  Serial.println();
  Serial.println(F("Before Z - raw physical positioning:"));
  Serial.println(F("  W : move physically UP"));
  Serial.println(F("  S : move physically DOWN"));
  Serial.println(F("  A : move physically LEFT"));
  Serial.println(F("  D : move physically RIGHT"));
  Serial.println();
  Serial.println(F("Reference and drawing commands:"));
  Serial.println(F("  Z : declare current physical position as logical (0,0)"));
  Serial.println(F("  F : draw circle, triangle, square, and diamond"));
  Serial.println(F("  C : return to logical (0, 0)"));
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


// Draw a circle using the same 2-degree angular sampling as WallDrawDemo.ino.
void drawCircle(float centerX, float centerY, float radius) {
  float startX = centerX + radius;
  float startY = centerY;

  raisePen();
  line_safe(startX, startY);
  lowerPen();

  for (float angle = CIRCLE_STEP; angle < TWO_PI; angle += CIRCLE_STEP) {
    line_safe(centerX + cos(angle) * radius,
              centerY + sin(angle) * radius);
  }

  line_safe(startX, startY);
  raisePen();
}


// Draw an equilateral triangle centered on its centroid.
void drawTriangle(float centerX, float centerY, float side) {
  float topY = centerY + side / sqrt(3.0);
  float bottomY = centerY - side / (2.0 * sqrt(3.0));
  float halfSide = side * 0.5;

  raisePen();
  line_safe(centerX, topY);
  lowerPen();
  line_safe(centerX + halfSide, bottomY);
  line_safe(centerX - halfSide, bottomY);
  line_safe(centerX, topY);
  raisePen();
}


// Draw an axis-aligned square centered on the requested position.
void drawSquare(float centerX, float centerY, float side) {
  float halfSide = side * 0.5;

  raisePen();
  line_safe(centerX - halfSide, centerY - halfSide);
  lowerPen();
  line_safe(centerX + halfSide, centerY - halfSide);
  line_safe(centerX + halfSide, centerY + halfSide);
  line_safe(centerX - halfSide, centerY + halfSide);
  line_safe(centerX - halfSide, centerY - halfSide);
  raisePen();
}


// Draw a diamond with equal horizontal and vertical diagonals.
void drawDiamond(float centerX, float centerY, float diagonal) {
  float halfDiagonal = diagonal * 0.5;

  raisePen();
  line_safe(centerX, centerY + halfDiagonal);
  lowerPen();
  line_safe(centerX + halfDiagonal, centerY);
  line_safe(centerX, centerY - halfDiagonal);
  line_safe(centerX - halfDiagonal, centerY);
  line_safe(centerX, centerY + halfDiagonal);
  raisePen();
}


// Draw the four validated test shapes around logical zero.
void drawFourShapes() {
  Serial.println(F("Drawing circle at (-60,+60)."));
  drawCircle(-SHAPE_OFFSET, SHAPE_OFFSET, CIRCLE_RADIUS);

  Serial.println(F("Drawing triangle at (+60,+60)."));
  drawTriangle(SHAPE_OFFSET, SHAPE_OFFSET, SHAPE_SIZE);

  Serial.println(F("Drawing square at (-60,-60)."));
  drawSquare(-SHAPE_OFFSET, -SHAPE_OFFSET, SHAPE_SIZE);

  Serial.println(F("Drawing diamond at (+60,-60)."));
  drawDiamond(SHAPE_OFFSET, -SHAPE_OFFSET, SHAPE_SIZE);

  raisePen();
  Serial.println(F("Returning to logical zero."));
  line_safe(0.0, 0.0);
  printPosition();
}


void setup() {
  Serial.begin(BAUD);

  // Preserve the original motor pin mapping.
  m1.connectToPins(7, 8, 9, 10);
  m2.connectToPins(2, 3, 5, 6);

  // Preserve the original speed and acceleration values.
  m1.setSpeedInStepsPerSecond(10000);
  m1.setAccelerationInStepsPerSecondPerSecond(100000);
  m2.setSpeedInStepsPerSecond(10000);
  m2.setAccelerationInStepsPerSecondPerSecond(100000);

  pen.attach(A0);
  raisePen();

  // Do not call teleport() here. The physical reference must be declared
  // explicitly by the user with Z after manual positioning.
  zeroIsSet = false;

  printHelp();
}


void loop() {
  if (!Serial.available()) {
    return;
  }

  char command = Serial.read();

  // Ignore line endings sent by the Serial Monitor.
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
        raisePen();
        Serial.println(F("Returning to logical (0,0)."));
        line_safe(0.0, 0.0);
        printPosition();
      }
      break;

    case 'F':
    case 'f':
      if (requireZero()) {
        drawFourShapes();
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