// Based on Walldraw, Copyright (c) 2021 shihaipeng03 (MIT License).
// Modifications Copyright (c) 2026 Lina Lopes.
// SPDX-License-Identifier: MIT
//
// Moebius Polargraph serial firmware 0.1.0
// Moebius Serial Protocol v1
//
// Derived from the validated Calibration and Gallery sketches:
// inverse kinematics, line subdivision, motor-direction constants,
// Bresenham-style coordination, TinyStepper pin mapping, and pen angles.
//
// Upstream Walldraw: https://github.com/shihaipeng03/Walldraw
// TinyStepper_28BYJ_48: Copyright (c) S. Reifel & Co.

#include <TinyStepper_28BYJ_48.h>
#include <Servo.h>
#include <math.h>
#include <string.h>

#define PROTOCOL_VERSION_STR  "1"
#define FIRMWARE_VERSION_STR  "0.1.0"
#define MACHINE_NAME_STR      "moebius-polargraph"

#define BAUD                  115200
#define LINE_MAX              80
#define LINE_BUF              (LINE_MAX + 1)
#define NUDGE_MAX_STEPS       200
#define STEP_DELAY_US         1
#define PI_SPOOL              3.1416f

#define M1_REEL_OUT           1
#define M1_REEL_IN           -1
#define M2_REEL_OUT          -1
#define M2_REEL_IN            1

#define PIN_M1_IN1            7
#define PIN_M1_IN2            8
#define PIN_M1_IN3            9
#define PIN_M1_IN4            10
#define PIN_M2_IN1            2
#define PIN_M2_IN2            3
#define PIN_M2_IN3            5
#define PIN_M2_IN4            6
#define PIN_SERVO             A0
// Pin 4 remains reserved for a future SD chip-select.

#define DEF_X_SEPARATION      820.0f
#define DEF_ZERO_DEPTH        520.0f
#define DEF_SPOOL_DIAMETER    35.0f
#define DEF_STEPS_PER_TURN    2048L
#define DEF_X_MIN            -100.0f
#define DEF_X_MAX             100.0f
#define DEF_Y_MIN            -100.0f
#define DEF_Y_MAX             100.0f
#define DEF_PEN_UP_ANGLE      90
#define DEF_PEN_DOWN_ANGLE    60
#define DEF_PEN_SETTLE_MS     500

#define STEPPER_SPEED         10000.0f
#define STEPPER_ACCEL         100000.0f

#define FLOAT_EPS             0.0001f


struct Config {
  float x_separation;
  float zero_depth;
  float spool_diameter;
  long steps_per_turn;
  float x_min;
  float x_max;
  float y_min;
  float y_max;
  int pen_up_angle;
  int pen_down_angle;
  int pen_settle_ms;
};

TinyStepper_28BYJ_48 m1;
TinyStepper_28BYJ_48 m2;
Servo pen;

static Config g_cfg;

static long g_laststep1 = 0;
static long g_laststep2 = 0;
static float g_posx = 0.0f;
static float g_posy = 0.0f;
static bool g_zero = false;
static bool g_pen_down = false;


static void replyOk() {
  Serial.println(F("ok"));
}

static void replyError(const __FlashStringHelper *code) {
  Serial.println(code);
}

static void toUpperInPlace(char *s) {
  while (*s) {
    if (*s >= 'a' && *s <= 'z') {
      *s = (char)(*s - 'a' + 'A');
    }
    ++s;
  }
}

static bool streq(const char *a, const char *b) {
  return strcmp(a, b) == 0;
}

static bool isFiniteNumber(float v) {
  return isfinite(v);
}

static bool nearlyEqual(float a, float b) {
  return fabs(a - b) <= FLOAT_EPS;
}

static float spoolCirc() {
  return g_cfg.spool_diameter * PI_SPOOL;
}

static float tps() {
  return spoolCirc() / (float)g_cfg.steps_per_turn;
}

static float limXMin() {
  return -g_cfg.x_separation * 0.5f;
}

static float limXMax() {
  return g_cfg.x_separation * 0.5f;
}

static bool parseFloatStrict(const char *s, float *out) {
  if (s == 0 || *s == 0) {
    return false;
  }

  const char *p = s;
  if (*p == '+' || *p == '-') {
    ++p;
  }
  if (*p == 0) {
    return false;
  }

  bool seenDigit = false;
  bool seenDot = false;
  while (*p) {
    if (*p >= '0' && *p <= '9') {
      seenDigit = true;
    } else if (*p == '.' && !seenDot) {
      seenDot = true;
    } else {
      return false;
    }
    ++p;
  }

  if (!seenDigit) {
    return false;
  }

  float v = atof(s);
  if (!isFiniteNumber(v)) {
    return false;
  }

  *out = v;
  return true;
}

static bool parseLongStrict(const char *s, long *out) {
  if (s == 0 || *s == 0) {
    return false;
  }

  const char *p = s;
  bool neg = false;
  if (*p == '+') {
    ++p;
  } else if (*p == '-') {
    neg = true;
    ++p;
  }
  if (*p < '0' || *p > '9') {
    return false;
  }

  long v = 0;
  while (*p) {
    if (*p < '0' || *p > '9') {
      return false;
    }
    int d = *p - '0';
    if (v > (2147483647L - d) / 10L) {
      return false;
    }
    v = v * 10L + d;
    ++p;
  }

  *out = neg ? -v : v;
  return true;
}

static bool parseIntStrict(const char *s, int *out) {
  long v;
  if (!parseLongStrict(s, &v)) {
    return false;
  }
  if (v < -32768L || v > 32767L) {
    return false;
  }
  *out = (int)v;
  return true;
}

static void skipSpaces(char **pp) {
  while (**pp == ' ') {
    ++(*pp);
  }
}

static void trimTrailingSpaces(char *s) {
  size_t n = strlen(s);
  while (n > 0 && s[n - 1] == ' ') {
    s[--n] = 0;
  }
}

static void printCompactFloat(float v) {
  if (nearlyEqual(v, (float)lround(v))) {
    Serial.print(lround(v));
  } else {
    Serial.print(v, 3);
  }
}

static void printPositionValue() {
  if (!g_zero) {
    Serial.print(F("unknown"));
  } else {
    Serial.print(g_posx, 3);
    Serial.print(',');
    Serial.print(g_posy, 3);
  }
}

static void printPositionField() {
  Serial.print(F("position="));
  printPositionValue();
}

static bool inWorkspace(float x, float y) {
  return x >= g_cfg.x_min && x <= g_cfg.x_max &&
         y >= g_cfg.y_min && y <= g_cfg.y_max &&
         y < g_cfg.zero_depth;
}

static bool validateConfig(const Config &c) {
  if (c.x_separation < 100.0f || c.x_separation > 3000.0f) {
    return false;
  }
  if (c.zero_depth < 100.0f || c.zero_depth > 3000.0f) {
    return false;
  }
  if (c.spool_diameter < 5.0f || c.spool_diameter > 100.0f) {
    return false;
  }
  if (c.steps_per_turn < 1L || c.steps_per_turn > 100000L) {
    return false;
  }
  if (c.x_min < -2500.0f || c.x_min > 2500.0f) {
    return false;
  }
  if (c.x_max < -2500.0f || c.x_max > 2500.0f) {
    return false;
  }
  if (c.y_min < -2500.0f || c.y_min > 2500.0f) {
    return false;
  }
  if (c.y_max < -2500.0f || c.y_max > 2500.0f) {
    return false;
  }
  if (c.pen_up_angle < 0 || c.pen_up_angle > 180) {
    return false;
  }
  if (c.pen_down_angle < 0 || c.pen_down_angle > 180) {
    return false;
  }
  if (c.pen_settle_ms < 0 || c.pen_settle_ms > 5000) {
    return false;
  }
  if (!(c.x_min < c.x_max)) {
    return false;
  }
  if (!(c.y_min < c.y_max)) {
    return false;
  }
  if (!(c.y_max < c.zero_depth)) {
    return false;
  }
  if (!(c.x_min < 0.0f && 0.0f < c.x_max)) {
    return false;
  }
  if (!(c.y_min < 0.0f && 0.0f < c.y_max)) {
    return false;
  }
  if (!isFiniteNumber(c.x_separation) || !isFiniteNumber(c.zero_depth) ||
      !isFiniteNumber(c.spool_diameter) || !isFiniteNumber(c.x_min) ||
      !isFiniteNumber(c.x_max) || !isFiniteNumber(c.y_min) ||
      !isFiniteNumber(c.y_max)) {
    return false;
  }

  float circ = c.spool_diameter * PI_SPOOL;
  float stepMm = circ / (float)c.steps_per_turn;
  if (!isFiniteNumber(circ) || !isFiniteNumber(stepMm) || !(stepMm > 0.0f)) {
    return false;
  }

  return true;
}

static void loadCompiledDefaults(Config *c) {
  c->x_separation = DEF_X_SEPARATION;
  c->zero_depth = DEF_ZERO_DEPTH;
  c->spool_diameter = DEF_SPOOL_DIAMETER;
  c->steps_per_turn = DEF_STEPS_PER_TURN;
  c->x_min = DEF_X_MIN;
  c->x_max = DEF_X_MAX;
  c->y_min = DEF_Y_MIN;
  c->y_max = DEF_Y_MAX;
  c->pen_up_angle = DEF_PEN_UP_ANGLE;
  c->pen_down_angle = DEF_PEN_DOWN_ANGLE;
  c->pen_settle_ms = DEF_PEN_SETTLE_MS;
}

static void clearZero() {
  g_zero = false;
}

// Convert a logical XY position to left and right cord lengths in steps.
// Anchors are at (-X_SEPARATION/2, ZERO_DEPTH) and (+X_SEPARATION/2, ZERO_DEPTH).
// Logical +X is physically right; logical +Y is physically up toward the anchors.
static bool IK(float x, float y, long &l1, long &l2) {
  float stepMm = tps();
  if (!isFiniteNumber(stepMm) || !(stepMm > 0.0f)) {
    return false;
  }

  float dy = y - g_cfg.zero_depth;
  float dx = x - limXMin();
  float d2 = dx * dx + dy * dy;
  if (!isFiniteNumber(d2) || d2 <= 0.0f) {
    return false;
  }
  float dist = sqrt(d2);
  if (!isFiniteNumber(dist) || !(dist > 0.0f)) {
    return false;
  }
  float s1 = dist / stepMm;
  if (!isFiniteNumber(s1) || !(s1 > 0.0f)) {
    return false;
  }
  l1 = (long)lround(s1);
  if (l1 <= 0L) {
    return false;
  }

  dx = x - limXMax();
  d2 = dx * dx + dy * dy;
  if (!isFiniteNumber(d2) || d2 <= 0.0f) {
    return false;
  }
  dist = sqrt(d2);
  if (!isFiniteNumber(dist) || !(dist > 0.0f)) {
    return false;
  }
  float s2 = dist / stepMm;
  if (!isFiniteNumber(s2) || !(s2 > 0.0f)) {
    return false;
  }
  l2 = (long)lround(s2);
  if (l2 <= 0L) {
    return false;
  }

  return true;
}

static bool cartesianTargetOk(float x, float y) {
  if (!isFiniteNumber(x) || !isFiniteNumber(y)) {
    return false;
  }
  if (!inWorkspace(x, y)) {
    return false;
  }
  long l1;
  long l2;
  return IK(x, y, l1, l2);
}

// Change only the virtual position. No motor movement occurs here.
static bool teleport(float x, float y) {
  long l1;
  long l2;
  if (!IK(x, y, l1, l2)) {
    return false;
  }
  g_posx = x;
  g_posy = y;
  g_laststep1 = l1;
  g_laststep2 = l2;
  return true;
}

// Move to one XY target using the validated cord directions and the original
// Bresenham-style coordination between the two motors.
static void moveto(float x, float y) {
  long l1;
  long l2;
  if (!IK(x, y, l1, l2)) {
    return;
  }

  long d1 = l1 - g_laststep1;
  long d2 = l2 - g_laststep2;
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

  g_laststep1 = l1;
  g_laststep2 = l2;
  g_posx = x;
  g_posy = y;
}

// Split a Cartesian line into short targets so that it remains straight in XY
// space after the Polargraph inverse-kinematics conversion.
static void line_safe(float x, float y) {
  float stepMm = tps();
  float dx = x - g_posx;
  float dy = y - g_posy;
  float len = sqrt(dx * dx + dy * dy);

  if (!isFiniteNumber(len) || !isFiniteNumber(stepMm) || !(stepMm > 0.0f) ||
      len <= stepMm) {
    moveto(x, y);
    return;
  }

  long pieces = (long)floor(len / stepMm);
  if (pieces < 1L) {
    moveto(x, y);
    return;
  }

  float x0 = g_posx;
  float y0 = g_posy;

  for (long j = 0; j <= pieces; ++j) {
    float amount = (float)j / (float)pieces;
    moveto((x - x0) * amount + x0, (y - y0) * amount + y0);
  }

  moveto(x, y);
}

static void raisePen() {
  pen.write(g_cfg.pen_up_angle);
  g_pen_down = false;
  delay((unsigned int)g_cfg.pen_settle_ms);
}

static void lowerPen() {
  pen.write(g_cfg.pen_down_angle);
  g_pen_down = true;
  delay((unsigned int)g_cfg.pen_settle_ms);
}

static void rawNudge(int direction1, int direction2, long steps) {
  if (g_pen_down) {
    raisePen();
  }

  for (long i = 0; i < steps; ++i) {
    m1.moveRelativeInSteps(direction1);
    m2.moveRelativeInSteps(direction2);
    delayMicroseconds(STEP_DELAY_US);
  }

  clearZero();
}

static bool looksLikeGCodeToken(const char *t) {
  if (t[0] != 'G' && t[0] != 'M' && t[0] != 'g' && t[0] != 'm') {
    if ((t[0] >= 'A' && t[0] <= 'Z') || (t[0] >= 'a' && t[0] <= 'z')) {
      if (t[1] != 0) {
        return true;
      }
    }
    return false;
  }

  if (t[1] < '0' || t[1] > '9') {
    return false;
  }

  const char *p = t + 1;
  while (*p) {
    if (*p < '0' || *p > '9') {
      return false;
    }
    ++p;
  }
  return true;
}

static bool parseGNumber(const char *t, long *num) {
  if (t[0] != 'G' && t[0] != 'M') {
    return false;
  }
  return parseLongStrict(t + 1, num);
}

static void cmdHello(char *rest) {
  if (*rest) {
    replyError(F("error:invalid-command"));
    return;
  }
  Serial.println(F("machine=moebius-polargraph protocol=1 firmware=0.1.0"));
  replyOk();
}

static void cmdInfo(char *rest) {
  if (*rest) {
    replyError(F("error:invalid-command"));
    return;
  }

  Serial.println(F("machine=moebius-polargraph"));
  Serial.println(F("protocol=1"));
  Serial.println(F("firmware=0.1.0"));
  Serial.println(F("geometry=polargraph"));
  Serial.println(F("queue=0"));
  Serial.println(F("eeprom=0"));
  Serial.println(F("homing=0"));
  Serial.println(F("feedrate=0"));

  Serial.print(F("x_separation="));
  printCompactFloat(g_cfg.x_separation);
  Serial.println();

  Serial.print(F("zero_depth="));
  printCompactFloat(g_cfg.zero_depth);
  Serial.println();

  Serial.print(F("spool_diameter="));
  printCompactFloat(g_cfg.spool_diameter);
  Serial.println();

  Serial.print(F("steps_per_turn="));
  Serial.println(g_cfg.steps_per_turn);

  Serial.print(F("x_min="));
  Serial.println(g_cfg.x_min, 3);
  Serial.print(F("x_max="));
  Serial.println(g_cfg.x_max, 3);
  Serial.print(F("y_min="));
  Serial.println(g_cfg.y_min, 3);
  Serial.print(F("y_max="));
  Serial.println(g_cfg.y_max, 3);

  Serial.print(F("pen_up_angle="));
  Serial.println(g_cfg.pen_up_angle);
  Serial.print(F("pen_down_angle="));
  Serial.println(g_cfg.pen_down_angle);
  Serial.print(F("pen_settle_ms="));
  Serial.println(g_cfg.pen_settle_ms);

  Serial.println(F("nudge_max_steps=200"));
  replyOk();
}

static void cmdStatus(char *rest) {
  if (*rest) {
    replyError(F("error:invalid-command"));
    return;
  }

  Serial.println(F("state=idle"));
  Serial.print(F("zero="));
  Serial.println(g_zero ? 1 : 0);
  printPositionField();
  Serial.println();
  Serial.print(F("pen="));
  Serial.println(g_pen_down ? F("down") : F("up"));
  replyOk();
}

static void dumpConfig() {
  Serial.print(F("PROTOCOL_VERSION="));
  Serial.println(F(PROTOCOL_VERSION_STR));
  Serial.print(F("FIRMWARE_VERSION="));
  Serial.println(F(FIRMWARE_VERSION_STR));
  Serial.print(F("MACHINE_NAME="));
  Serial.println(F(MACHINE_NAME_STR));

  Serial.print(F("X_SEPARATION="));
  printCompactFloat(g_cfg.x_separation);
  Serial.println();
  Serial.print(F("ZERO_DEPTH="));
  printCompactFloat(g_cfg.zero_depth);
  Serial.println();
  Serial.print(F("SPOOL_DIAMETER="));
  printCompactFloat(g_cfg.spool_diameter);
  Serial.println();
  Serial.print(F("STEPS_PER_TURN="));
  Serial.println(g_cfg.steps_per_turn);

  Serial.print(F("LIMXMIN="));
  printCompactFloat(limXMin());
  Serial.println();
  Serial.print(F("LIMXMAX="));
  printCompactFloat(limXMax());
  Serial.println();
  Serial.print(F("SPOOL_CIRC="));
  Serial.println(spoolCirc(), 3);
  Serial.print(F("TPS="));
  Serial.println(tps(), 6);

  Serial.print(F("X_MIN="));
  Serial.println(g_cfg.x_min, 3);
  Serial.print(F("X_MAX="));
  Serial.println(g_cfg.x_max, 3);
  Serial.print(F("Y_MIN="));
  Serial.println(g_cfg.y_min, 3);
  Serial.print(F("Y_MAX="));
  Serial.println(g_cfg.y_max, 3);

  Serial.print(F("PEN_UP_ANGLE="));
  Serial.println(g_cfg.pen_up_angle);
  Serial.print(F("PEN_DOWN_ANGLE="));
  Serial.println(g_cfg.pen_down_angle);
  Serial.print(F("PEN_SETTLE_MS="));
  Serial.println(g_cfg.pen_settle_ms);
}

static void echoConfigKey(const char *key, const Config &c) {
  Serial.print(key);
  Serial.print('=');
  if (streq(key, "X_SEPARATION")) {
    printCompactFloat(c.x_separation);
  } else if (streq(key, "ZERO_DEPTH")) {
    printCompactFloat(c.zero_depth);
  } else if (streq(key, "SPOOL_DIAMETER")) {
    printCompactFloat(c.spool_diameter);
  } else if (streq(key, "STEPS_PER_TURN")) {
    Serial.print(c.steps_per_turn);
  } else if (streq(key, "X_MIN")) {
    Serial.print(c.x_min, 3);
  } else if (streq(key, "X_MAX")) {
    Serial.print(c.x_max, 3);
  } else if (streq(key, "Y_MIN")) {
    Serial.print(c.y_min, 3);
  } else if (streq(key, "Y_MAX")) {
    Serial.print(c.y_max, 3);
  } else if (streq(key, "PEN_UP_ANGLE")) {
    Serial.print(c.pen_up_angle);
  } else if (streq(key, "PEN_DOWN_ANGLE")) {
    Serial.print(c.pen_down_angle);
  } else if (streq(key, "PEN_SETTLE_MS")) {
    Serial.print(c.pen_settle_ms);
  }
  Serial.println();
}

static bool isReadOnlyKey(const char *key) {
  return streq(key, "PROTOCOL_VERSION") ||
         streq(key, "FIRMWARE_VERSION") ||
         streq(key, "MACHINE_NAME") ||
         streq(key, "LIMXMIN") ||
         streq(key, "LIMXMAX") ||
         streq(key, "SPOOL_CIRC") ||
         streq(key, "TPS");
}

static bool isKinematicKey(const char *key) {
  return streq(key, "X_SEPARATION") ||
         streq(key, "ZERO_DEPTH") ||
         streq(key, "SPOOL_DIAMETER") ||
         streq(key, "STEPS_PER_TURN");
}

static bool applyConfigKey(Config *c, const char *key, const char *value) {
  if (streq(key, "X_SEPARATION")) {
    return parseFloatStrict(value, &c->x_separation);
  }
  if (streq(key, "ZERO_DEPTH")) {
    return parseFloatStrict(value, &c->zero_depth);
  }
  if (streq(key, "SPOOL_DIAMETER")) {
    return parseFloatStrict(value, &c->spool_diameter);
  }
  if (streq(key, "STEPS_PER_TURN")) {
    return parseLongStrict(value, &c->steps_per_turn);
  }
  if (streq(key, "X_MIN")) {
    return parseFloatStrict(value, &c->x_min);
  }
  if (streq(key, "X_MAX")) {
    return parseFloatStrict(value, &c->x_max);
  }
  if (streq(key, "Y_MIN")) {
    return parseFloatStrict(value, &c->y_min);
  }
  if (streq(key, "Y_MAX")) {
    return parseFloatStrict(value, &c->y_max);
  }
  if (streq(key, "PEN_UP_ANGLE")) {
    return parseIntStrict(value, &c->pen_up_angle);
  }
  if (streq(key, "PEN_DOWN_ANGLE")) {
    return parseIntStrict(value, &c->pen_down_angle);
  }
  if (streq(key, "PEN_SETTLE_MS")) {
    return parseIntStrict(value, &c->pen_settle_ms);
  }
  return false;
}

static bool configChangedKinematic(const Config &a, const Config &b) {
  return !nearlyEqual(a.x_separation, b.x_separation) ||
         !nearlyEqual(a.zero_depth, b.zero_depth) ||
         !nearlyEqual(a.spool_diameter, b.spool_diameter) ||
         a.steps_per_turn != b.steps_per_turn;
}

static void cmdConfigQuery() {
  dumpConfig();
  replyOk();
}

static void cmdConfigSet(char *rest) {
  if (*rest == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *p = rest;
  char *key = p;
  while (*p && *p != '=' && *p != ' ') {
    ++p;
  }
  if (key == p) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *keyEnd = p;
  skipSpaces(&p);
  if (*p != '=') {
    replyError(F("error:invalid-command"));
    return;
  }
  *keyEnd = 0;
  ++p;
  skipSpaces(&p);
  if (*p == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *value = p;
  trimTrailingSpaces(value);
  toUpperInPlace(key);

  for (char *k = key; *k; ++k) {
    if (!((*k >= 'A' && *k <= 'Z') || (*k >= '0' && *k <= '9') || *k == '_')) {
      replyError(F("error:invalid-parameter"));
      return;
    }
  }

  if (isReadOnlyKey(key)) {
    replyError(F("error:not-supported"));
    return;
  }

  Config next = g_cfg;
  if (!applyConfigKey(&next, key, value)) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  if (!validateConfig(next)) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  if (g_zero) {
    if (g_posx < next.x_min || g_posx > next.x_max ||
        g_posy < next.y_min || g_posy > next.y_max ||
        g_posy >= next.zero_depth) {
      replyError(F("error:outside-workspace"));
      return;
    }
  }

  bool kinematicChanged = isKinematicKey(key) &&
                          configChangedKinematic(g_cfg, next);
  g_cfg = next;
  if (kinematicChanged) {
    clearZero();
  }

  echoConfigKey(key, g_cfg);
  replyOk();
}

static void cmdNudge(char *rest) {
  if (*rest == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *dir = rest;
  char *p = rest;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }
  *p++ = 0;
  skipSpaces(&p);
  if (*p == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *stepsTok = p;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p) {
    *p++ = 0;
    skipSpaces(&p);
    if (*p) {
      replyError(F("error:invalid-command"));
      return;
    }
  }

  toUpperInPlace(dir);

  int d1;
  int d2;
  if (streq(dir, "UP")) {
    d1 = M1_REEL_IN;
    d2 = M2_REEL_IN;
  } else if (streq(dir, "DOWN")) {
    d1 = M1_REEL_OUT;
    d2 = M2_REEL_OUT;
  } else if (streq(dir, "LEFT")) {
    d1 = M1_REEL_IN;
    d2 = M2_REEL_OUT;
  } else if (streq(dir, "RIGHT")) {
    d1 = M1_REEL_OUT;
    d2 = M2_REEL_IN;
  } else {
    replyError(F("error:invalid-parameter"));
    return;
  }

  long steps;
  if (!parseLongStrict(stepsTok, &steps) || steps <= 0L ||
      steps > (long)NUDGE_MAX_STEPS) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  rawNudge(d1, d2, steps);
  replyOk();
}

static void cmdJog(char *rest) {
  if (*rest == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *axisTok = rest;
  char *p = rest;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }
  *p++ = 0;
  skipSpaces(&p);
  if (*p == 0) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  char *numTok = p;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p) {
    *p++ = 0;
    skipSpaces(&p);
    if (*p) {
      replyError(F("error:invalid-command"));
      return;
    }
  }

  toUpperInPlace(axisTok);
  if (!(streq(axisTok, "X") || streq(axisTok, "Y"))) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  float delta;
  if (!parseFloatStrict(numTok, &delta)) {
    replyError(F("error:invalid-parameter"));
    return;
  }

  if (!g_zero) {
    replyError(F("error:zero-required"));
    return;
  }

  float tx = g_posx;
  float ty = g_posy;
  if (axisTok[0] == 'X') {
    tx += delta;
  } else {
    ty += delta;
  }

  if (!cartesianTargetOk(tx, ty)) {
    replyError(F("error:outside-workspace"));
    return;
  }

  if (g_pen_down) {
    raisePen();
  }

  if (nearlyEqual(tx, g_posx) && nearlyEqual(ty, g_posy)) {
    replyOk();
    return;
  }

  line_safe(tx, ty);
  replyOk();
}

static void cmdZero(char *rest) {
  if (*rest == 0) {
    if (!teleport(0.0f, 0.0f)) {
      replyError(F("error:outside-workspace"));
      return;
    }
    g_zero = true;
    Serial.print(F("zero=1 "));
    printPositionField();
    Serial.println();
    replyOk();
    return;
  }

  char *p = rest;
  char *word = p;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p) {
    *p++ = 0;
    skipSpaces(&p);
    if (*p) {
      replyError(F("error:invalid-command"));
      return;
    }
  }

  toUpperInPlace(word);
  if (!streq(word, "CLEAR")) {
    replyError(F("error:invalid-command"));
    return;
  }

  clearZero();
  Serial.println(F("zero=0 position=unknown"));
  replyOk();
}

static void cmdPenAlias(bool down, char *rest) {
  if (*rest) {
    if (looksLikeGCodeToken(rest)) {
      replyError(F("error:not-supported"));
    } else {
      replyError(F("error:invalid-command"));
    }
    return;
  }
  if (down) {
    lowerPen();
  } else {
    raisePen();
  }
  replyOk();
}

static void cmdPenWords(char *rest) {
  if (*rest == 0) {
    replyError(F("error:invalid-command"));
    return;
  }

  char *p = rest;
  char *word = p;
  while (*p && *p != ' ') {
    ++p;
  }
  if (*p) {
    *p++ = 0;
    skipSpaces(&p);
    if (*p) {
      replyError(F("error:invalid-command"));
      return;
    }
  }

  toUpperInPlace(word);
  if (streq(word, "UP")) {
    raisePen();
    replyOk();
  } else if (streq(word, "DOWN")) {
    lowerPen();
    replyOk();
  } else {
    replyError(F("error:invalid-command"));
  }
}

static void cmdSimpleOk(char *rest) {
  if (*rest) {
    replyError(F("error:invalid-command"));
    return;
  }
  replyOk();
}

static void cmdAbort(char *rest) {
  if (*rest) {
    replyError(F("error:invalid-command"));
    return;
  }
  raisePen();
  replyOk();
}

static void cmdMoveG(char *rest) {
  bool gotX = false;
  bool gotY = false;
  bool gotF = false;
  bool dup = false;
  bool badNum = false;
  bool extraWord = false;
  bool garbage = false;
  float x = 0.0f;
  float y = 0.0f;

  char *p = rest;
  while (*p) {
    skipSpaces(&p);
    if (*p == 0) {
      break;
    }

    char *tok = p;
    while (*p && *p != ' ') {
      ++p;
    }
    char saved = *p;
    *p = 0;

    char letter = tok[0];
    if (letter >= 'a' && letter <= 'z') {
      letter = (char)(letter - 'a' + 'A');
    }

    if (letter == 'F') {
      gotF = true;
    } else if (letter == 'X' || letter == 'Y') {
      float v;
      if (tok[1] == 0 || !parseFloatStrict(tok + 1, &v)) {
        badNum = true;
      } else if (letter == 'X') {
        if (gotX) {
          dup = true;
        }
        gotX = true;
        x = v;
      } else {
        if (gotY) {
          dup = true;
        }
        gotY = true;
        y = v;
      }
    } else if (looksLikeGCodeToken(tok)) {
      extraWord = true;
    } else {
      garbage = true;
    }

    *p = saved;
    if (saved) {
      ++p;
    }
  }

  if (gotF || extraWord) {
    replyError(F("error:not-supported"));
    return;
  }
  if (garbage) {
    replyError(F("error:invalid-command"));
    return;
  }
  if (dup || badNum || (!gotX && !gotY)) {
    replyError(F("error:invalid-parameter"));
    return;
  }
  if (!g_zero) {
    replyError(F("error:zero-required"));
    return;
  }

  float tx = gotX ? x : g_posx;
  float ty = gotY ? y : g_posy;
  if (!cartesianTargetOk(tx, ty)) {
    replyError(F("error:outside-workspace"));
    return;
  }

  line_safe(tx, ty);
  replyOk();
}

static void dispatch(char *line) {
  skipSpaces(&line);
  if (*line == 0) {
    return;
  }

  char *cmd = line;
  char *rest = line;
  while (*rest && *rest != ' ') {
    ++rest;
  }
  if (*rest) {
    *rest++ = 0;
    skipSpaces(&rest);
  }

  toUpperInPlace(cmd);

  if (streq(cmd, "HELLO")) {
    cmdHello(rest);
  } else if (streq(cmd, "INFO")) {
    cmdInfo(rest);
  } else if (streq(cmd, "STATUS")) {
    cmdStatus(rest);
  } else if (streq(cmd, "CONFIG?")) {
    if (*rest) {
      replyError(F("error:invalid-command"));
    } else {
      cmdConfigQuery();
    }
  } else if (streq(cmd, "CONFIG")) {
    cmdConfigSet(rest);
  } else if (streq(cmd, "NUDGE")) {
    cmdNudge(rest);
  } else if (streq(cmd, "JOG")) {
    cmdJog(rest);
  } else if (streq(cmd, "ZERO")) {
    cmdZero(rest);
  } else if (streq(cmd, "PEN")) {
    cmdPenWords(rest);
  } else if (streq(cmd, "ABORT")) {
    cmdAbort(rest);
  } else if (streq(cmd, "PAUSE") || streq(cmd, "RESUME")) {
    replyError(F("error:not-supported"));
  } else if (cmd[0] == 'G' || cmd[0] == 'M') {
    long num;
    if (!parseGNumber(cmd, &num)) {
      if (looksLikeGCodeToken(cmd)) {
        replyError(F("error:not-supported"));
      } else {
        replyError(F("error:invalid-command"));
      }
      return;
    }
    if (cmd[0] == 'G') {
      if (num == 21 || num == 90) {
        cmdSimpleOk(rest);
      } else if (num == 0 || num == 1) {
        cmdMoveG(rest);
      } else {
        replyError(F("error:not-supported"));
      }
    } else {
      if (num == 3) {
        cmdPenAlias(true, rest);
      } else if (num == 5) {
        cmdPenAlias(false, rest);
      } else {
        replyError(F("error:not-supported"));
      }
    }
  } else {
    replyError(F("error:invalid-command"));
  }
}

static void processOneLine() {
  char buf[LINE_BUF];
  uint8_t n = 0;
  bool overflow = false;
  bool badchar = false;

  for (;;) {
    int c;
    do {
      c = Serial.read();
    } while (c < 0);

    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      break;
    }
    if (c < 0x20 || c > 0x7E) {
      badchar = true;
    }
    if (n >= LINE_MAX) {
      overflow = true;
    } else if (!overflow) {
      buf[n++] = (char)c;
    }
  }

  if (overflow || badchar) {
    replyError(F("error:invalid-command"));
    return;
  }

  buf[n] = 0;
  trimTrailingSpaces(buf);

  char *s = buf;
  skipSpaces(&s);
  if (*s == 0) {
    return;
  }

  dispatch(s);
}

void setup() {
  Serial.begin(BAUD);

  loadCompiledDefaults(&g_cfg);
  g_zero = false;
  g_pen_down = false;
  g_posx = 0.0f;
  g_posy = 0.0f;
  g_laststep1 = 0;
  g_laststep2 = 0;

  m1.connectToPins(PIN_M1_IN1, PIN_M1_IN2, PIN_M1_IN3, PIN_M1_IN4);
  m2.connectToPins(PIN_M2_IN1, PIN_M2_IN2, PIN_M2_IN3, PIN_M2_IN4);
  m1.setSpeedInStepsPerSecond(STEPPER_SPEED);
  m1.setAccelerationInStepsPerSecondPerSecond(STEPPER_ACCEL);
  m2.setSpeedInStepsPerSecond(STEPPER_SPEED);
  m2.setAccelerationInStepsPerSecondPerSecond(STEPPER_ACCEL);

  pen.attach(PIN_SERVO);
  raisePen();

  Serial.println(F("boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0"));
}

void loop() {
  if (!Serial.available()) {
    return;
  }
  processOneLine();
}
