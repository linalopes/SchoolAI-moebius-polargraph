# Moebius Serial Protocol v1

**Status:** approved specification. Firmware is not implemented by this documentation task.

**Protocol name:** Moebius Serial Protocol v1  
**Protocol version:** `1`  
**Initial experimental firmware version:** `0.1.0`  
**Document date:** 2026-08-25

This is original Moebius Polargraph documentation by Lina Lopes and is licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Full license text: [`LICENSES/CC-BY-4.0.txt`](../LICENSES/CC-BY-4.0.txt).

This document defines the first serial protocol between:

- the future Moebius Polargraph Arduino firmware in this repository;
- the separate SchoolAI Plotter Workspace web application.

It is **not** GRBL. It is **not** the current single-key Serial Monitor UI. It does not relicense Walldraw, TinyStepper_28BYJ_48, AccelStepper, the Arduino SD library, Servo, or any other third-party material. Existing copyright, license, and attribution notices in this repository must remain intact.

Startup and identification responses expose both version identifiers:

```text
protocol=1
firmware=0.1.0
```

---

## Table of contents

1. [Scope and non-goals](#1-scope-and-non-goals)
2. [Architecture](#2-architecture)
3. [Physical link](#3-physical-link)
4. [Coordinate system](#4-coordinate-system)
5. [Units and numeric formatting](#5-units-and-numeric-formatting)
6. [Line termination and framing](#6-line-termination-and-framing)
7. [Command grammar](#7-command-grammar)
8. [Response grammar](#8-response-grammar)
9. [Machine states](#9-machine-states)
10. [State transitions](#10-state-transitions)
11. [Configuration keys](#11-configuration-keys)
12. [Complete command reference](#12-complete-command-reference)
13. [Validation and safety rules](#13-validation-and-safety-rules)
14. [Reset and reconnection behavior](#14-reset-and-reconnection-behavior)
15. [Example sessions](#15-example-sessions)
16. [Compatibility and versioning](#16-compatibility-and-versioning)
17. [MVP limitations](#17-mvp-limitations)
18. [Future extensions outside v1](#18-future-extensions-outside-v1)
19. [Firmware implementation notes](#19-firmware-implementation-notes)
20. [Web-client implementation notes](#20-web-client-implementation-notes)
21. [Acceptance-test checklist](#21-acceptance-test-checklist)
22. [Repository facts used](#22-repository-facts-used)
23. [Open decisions](#23-open-decisions)

---

## 1. Scope and non-goals

### 1.1 Scope

v1 defines a small, explicit, line-based ASCII protocol so a browser can:

- identify the protocol version (`1`) and firmware version (`0.1.0`);
- read and temporarily change runtime kinematic, workspace, and pen configuration (RAM only);
- position the carriage with raw `NUDGE` commands before zero is known;
- declare or clear logical `(0,0)` by hand;
- raise and lower the pen with explicit commands;
- jog and plot in absolute Cartesian millimetres **after** zero is set;
- stream drawing segments one at a time;
- abort between completed commands by raising the pen;
- receive deterministic completion and error lines.

v1 is intended for a future serial-controlled drawing sketch derived from the validated calibration/gallery kinematics. The diagnostic two-stepper test is out of scope.

### 1.2 Non-goals

v1 explicitly does **not**:

- emulate the complete GRBL protocol, GRBL streaming (`ok` pipelining, status `?`, `$` settings, or real-time bytes);
- add SD-card support or file playback;
- write configuration to EEPROM, flash, or any other persistent store;
- implement automatic homing, limit switches, encoders, or other position feedback;
- implement a firmware command queue or multi-command planner;
- store a complete drawing on the Arduino;
- recover a plot after power loss or serial disconnect;
- provide calibrated Cartesian feedrate control;
- pause or abort a movement that is already executing;
- accept independent per-motor step vectors or IK-bypass plotting (the only step-count command is `NUDGE`, which names a physical direction);
- support inches, relative G-code (`G91`), arcs (`G2`/`G3`), probing, or spindle/laser semantics beyond pen up/down aliases;
- change current firmware in this documentation task;
- change licenses or rename existing files.

Client-side pause (not sending the next command) is in scope for the web application. Firmware `PAUSE` / `RESUME` commands are **not** in the v1 command set.

---

## 2. Architecture

```text
SchoolAI Plotter Workspace          Arduino firmware
(browser, Web Serial)               (Moebius Polargraph)

  SVG / paths
       │
       ▼
  flatten, scale,                   inverse kinematics
  center, clip                      cord-step conversion
  to millimetre XY                  Bresenham coordination
       │                            servo pen lift
       ▼
  ASCII lines  ──────────────────►  one blocking command
  NUDGE (raw steps)                 `ok` after physical done
  or Cartesian mm XY
       ▲
       └──────── responses ─────────┘
```

### 2.1 Division of labor

| Responsibility | Owner |
| --- | --- |
| Curve flattening, scaling, centering, path optimization | Browser |
| Streaming one segment at a time; pausing by not sending | Browser |
| Raw physical positioning (`NUDGE` direction + step count) | Browser → firmware |
| Cartesian millimetres, absolute XY (`JOG`, `G0`, `G1`) | Browser → firmware, only after zero |
| Inverse kinematics (XY → two cord lengths) | Firmware |
| Cord length → motor steps | Firmware |
| Mapping `NUDGE` directions onto validated motor pairings | Firmware |
| Coordinated two-motor motion | Firmware |
| Pen servo angles and settle time | Firmware |
| Declaring or clearing logical zero | Operator / client, via `ZERO` and `ZERO CLEAR` |
| Storing the complete drawing | Neither; not stored on the Arduino |

The browser never sends pin numbers, independent left/right step vectors, or reel-in/reel-out signs. Plotting uses Cartesian millimetres only. `NUDGE` is the exception that names a physical direction and a step magnitude; it is not a Cartesian millimetre command.

### 2.2 Zero is manual and volatile

The machine has no limit switches, encoders, or automatic homing.

Logical zero is undefined:

- after firmware boot;
- after `ZERO CLEAR`;
- after a successful kinematic `CONFIG` write;
- after `NUDGE` (which always invalidates zero).

The operator must place the carriage at the geometric origin implied by the active kinematics (`X_SEPARATION` and `ZERO_DEPTH`), then send `ZERO`. That command does not move the motors. It declares the current physical pose as logical `(0,0)` and initializes the firmware cord-step counters from inverse kinematics, matching the current calibration/gallery `Z` + `teleport(0,0)` workflow.

Opening a serial port **must not** be assumed to reset every compatible Arduino board. The SchoolAI Plotter Workspace Moebius controller therefore sends `ZERO CLEAR` on every new connection before enabling Cartesian movement or plotting. That makes the safety behaviour explicit instead of depending on USB/DTR reset.

### 2.3 One command at a time (blocking)

The firmware processes at most one protocol command at a time. There is no drawing queue on the Arduino.

`ok` is emitted **only after** the physical operation has completed (motors at rest, and for pen commands after `PEN_SETTLE_MS`).

The client **must** wait for that terminal `ok` or `error:…` before sending the next command.

Long Cartesian movements are internally subdivided by the existing `line_safe()` geometry (segments of about one cord-step in XY). That subdivision is an implementation detail of one blocking command. It does **not** create protocol-visible pause or abort points.

### 2.4 Pause is a client behaviour; abort is between commands

Protocol v1 has no firmware `PAUSE` or `RESUME`. The web client pauses a job by not sending the next command after the current `ok`.

`ABORT` is processed only when the firmware is idle between completed blocking commands. It raises the pen and does not move the carriage. It cannot interrupt a movement already inside `moveto()` / TinyStepper.

The client cancels a job by:

1. stopping transmission;
2. waiting for the in-flight command’s `ok` or `error:…` if one was already sent;
3. sending `ABORT`.

Immediate mid-motion emergency stopping is outside protocol v1.

### 2.5 Runtime configuration is RAM-only

`CONFIG` changes live in RAM. They return to compiled defaults after reboot. The firmware must not write EEPROM as a side effect of `CONFIG`, `ZERO`, `ZERO CLEAR`, or plotting.

---

## 3. Physical link

| Parameter | v1 value | Source |
| --- | --- | --- |
| Baud | `115200` | Calibration and gallery sketches (`BAUD`); this protocol |
| Data bits | 8 | This protocol (`8N1`) |
| Parity | none | This protocol |
| Stop bits | 1 | This protocol |
| Flow control | none | Not used by current sketches |
| Encoding | US-ASCII, line-based | This protocol |

Commands are terminated by `LF` (`0x0A`). The firmware may accept `CRLF` by ignoring `CR` (`0x0D`).

The two-stepper diagnostic sketch uses `9600` baud and is **not** a v1 protocol endpoint.

Some Arduino-class boards pulse DTR on serial open and reboot; others do not. See [§14](#14-reset-and-reconnection-behavior).

---

## 4. Coordinate system

Logical plotting coordinates are a right-handed Cartesian plane on the drawing surface.

| Axis | Direction | Firmware fact |
| --- | --- | --- |
| `+X` | physically right | Calibration IK comment and README |
| `+Y` | physically up, toward the motor anchors | Same |
| Origin | declared by `ZERO` at the current carriage pose | Calibration/gallery `Z` |

The inverse-kinematics anchors used by the drawing sketches, expressed in logical millimetres after a correct `ZERO`, are:

| Anchor | Logical coordinates (mm) |
| --- | --- |
| Left cord exit | `(-X_SEPARATION/2, ZERO_DEPTH)` = `(-410, 520)` at defaults |
| Right cord exit | `(+X_SEPARATION/2, ZERO_DEPTH)` = `(+410, 520)` at defaults |

Protocol key `ZERO_DEPTH` is the drawing-sketch constant `LIMYMIN`: the vertical offset of logical origin **below** the anchors, not a minimum drawing Y. At logical `(0,0)` the carriage is centered between the cord exits and `ZERO_DEPTH` millimetres below them.

`ZERO` is valid only if the operator has placed the carriage at that geometric origin. If `ZERO` is declared elsewhere, every later Cartesian command will be systematically wrong. The protocol cannot detect that error.

Plotting commands (`G0`, `G1`) are **absolute** (`G90`). Cartesian `JOG` is **relative** millimetres after zero. `NUDGE` is **not** a Cartesian command.

### 4.1 Software workspace

Protocol v1 uses this conservative default rectangle around logical zero:

| Key | Default (mm) |
| --- | --- |
| `X_MIN` | `-100.0` |
| `X_MAX` | `+100.0` |
| `Y_MIN` | `-100.0` |
| `Y_MAX` | `+100.0` |

This is an **initially validated software envelope**, not the complete physical reach of the machine. The calibration four-shape test and gallery drawings in this repository stay near logical zero (tens of millimetres). Future testing may enlarge the rectangle through `CONFIG`.

Cartesian `G0`, `G1`, and post-zero `JOG` targets outside this rectangle return `error:outside-workspace`.

The firmware must also reject geometry that reaches or crosses the anchor line. The configured workspace must satisfy:

```text
Y_MAX < ZERO_DEPTH
```

At compiled defaults, `100.0 < 520` holds.

---

## 5. Units and numeric formatting

| Quantity | Unit | Notes |
| --- | --- | --- |
| `X`, `Y`, Cartesian `JOG` | millimetres | Only Cartesian unit in v1 (`G21`) |
| `NUDGE` magnitude | motor steps | Positive integer; not millimetres |
| Pen angles | integer degrees | Servo `write()` units |
| Times | integer milliseconds | Pen settle |
| Kinematic CONFIG | millimetres or steps | See [§11](#11-configuration-keys) |

Protocol v1 does **not** accept a feedrate `F` parameter. See [§12.4](#124-plotting).

Rules:

- Cartesian commands may use signed integers or decimal fractions (`10`, `-12.5`, `0.025`).
- Scientific notation is **rejected** (`1e2` → `error:invalid-parameter`).
- Firmware **reports** Cartesian coordinates with **exactly 3 decimal places** (`0.000`), matching the calibration/gallery `Serial.print(posx, 3)` behaviour.
- The parser may accept more than 3 input digits, then use the internal float consumed by IK.
- Leading `+` is allowed. Leading zeros are allowed. Empty numbers are not.
- Integer configuration values and `NUDGE` step counts must parse as integers. A non-integer decimal is `error:invalid-parameter`.

Cord millimetres per step in the current drawing sketches (firmware internal; not a Cartesian feedrate):

```text
TPS = (SPOOL_DIAMETER * 3.1416) / STEPS_PER_TURN
    = (35 * 3.1416) / 2048
    ≈ 0.053689 mm / step
```

The sketches use `3.1416`, not a library `PI` macro, for spool circumference. v1 firmware must keep that constant.

---

## 6. Line termination and framing

- One command per line.
- Command terminator: `LF` (`0x0A`).
- `CRLF` is accepted by ignoring `CR` (`0x0D`).
- Responses are one or more ASCII lines, each ending in `LF`. The web client may ignore extra `CR`.
- Empty lines (optional spaces plus terminator) are ignored: no `ok`, no error.
- There is no binary framing, no checksum, and no line comment syntax in v1.
- Characters outside printable ASCII (`0x20`–`0x7E`) plus `CR`/`LF` are `error:invalid-command`.
- Maximum command line length: **80 bytes** including an optional `CR` but excluding the terminating `LF`.
- If a line exceeds 80 bytes, the firmware discards through the next `LF` and returns `error:invalid-command`.

The implementation must use a **fixed-size character buffer** (80 bytes plus a terminating `NUL`). It must **not** use Arduino `String` allocation for parsing or replies.

The Arduino hardware serial buffer is typically 64 bytes. The firmware reads a complete line only while idle. Clients must not pipeline. If a client violates the wait rule, bytes may accumulate in the UART ring and run as the next command **after** the current command’s terminal line; that is not a supported queue.

---

## 7. Command grammar

Matching is **case-insensitive** for command names, directions, and configuration keys. Responses are **lowercase** and exact.

Recommended client style: uppercase commands, uppercase keys, no trailing spaces.

```text
line       = command [ 1*SP argument ] *SP
command    = token
token      = 1*( DIGIT / ALPHA / "?" )
argument   = 1*( %x21-7E )          ; printable ASCII except space
```

Space (`0x20`) separates tokens. Tabs are `error:invalid-command`.

Required v1 command list:

```text
HELLO
INFO
STATUS

CONFIG?
CONFIG <KEY>=<VALUE>

NUDGE <DIRECTION> <STEPS>
JOG X <float>
JOG Y <float>
ZERO
ZERO CLEAR
PEN UP
PEN DOWN

G21
G90
G0 [X<float>] [Y<float>]
G1 [X<float>] [Y<float>]
M3
M5

ABORT
```

`DIRECTION` is one of `UP`, `DOWN`, `LEFT`, `RIGHT`.  
`STEPS` is a positive integer.

Reserved future commands (not implemented; see [§12.5](#125-control)):

```text
PAUSE
RESUME
```

Notes:

- `G0` / `G1` axis letters may appear in any order. Do not put a space between the letter and the number (`G1 X10.0 Y-3.5` is valid; `G1 X 10` is `error:invalid-command`).
- At least one of `X` or `Y` is required on `G0` and `G1`. A missing axis keeps the current logical value.
- Any `F` token on `G0` or `G1` is `error:not-supported`.
- Duplicate axis letters on one line are `error:invalid-parameter`.
- Unknown words, extra tokens, or missing required tokens are `error:invalid-command` or `error:invalid-parameter` as specified per command.
- `CONFIG <KEY>=<VALUE>` allows optional spaces around `=`. `KEY` is `A–Z`, `0–9`, and `_` only.

---

## 8. Response grammar

Every accepted non-empty command line produces **exactly one terminal line**. Informational lines, if any, are printed **before** that terminal line.

### 8.1 Terminal lines

| Terminal line | Meaning |
| --- | --- |
| `ok` | Command finished; physical side effects are complete. |
| `error:zero-required` | A Cartesian command was sent while zero is unset. |
| `error:invalid-command` | Unknown command, bad framing, overflow, illegal characters. |
| `error:invalid-parameter` | Known command, bad or missing numeric/key/direction value. |
| `error:outside-workspace` | Cartesian target or workspace `CONFIG` fails validation. |
| `error:not-supported` | Recognized token outside v1 (`G91`, `G20`, `F`, `PAUSE`, `RESUME`, read-only `CONFIG` writes). |

No other terminal prefixes exist in v1. Do not emit GRBL-style `error:N` integers.

**`busy` is not an emitted terminal response in protocol v1.** It names the internal condition “a blocking command is running.” During that condition the parser does not accept a new line. The client observes this only as: it has sent a command and has not yet received `ok` or `error:…`.

**`idle` is not a terminal success line.** It is the `state=` value in `STATUS`. Whenever `STATUS` can reply, the parser is idle, so `STATUS` always reports `state=idle` in v1.

Clients wait for exactly one of `ok` or `error:…` per command. They must not wait for `busy`.

### 8.2 Informational lines

Informational lines use `key=value` tokens, separated by a single space, with no spaces around `=`.

```text
protocol=1 firmware=0.1.0
state=idle zero=0 position=unknown pen=up
X_SEPARATION=820
```

Rules:

- Keys are lowercase in **runtime** replies (`boot`, `HELLO`, `INFO`, `STATUS`).
- Configuration dump keys match the canonical key names in [§11](#11-configuration-keys) (uppercase with underscores).
- Values contain no spaces.
- Booleans and flags are `0` or `1`.
- Cartesian positions use the `position=` field in [§8.4](#84-position-reporting).

### 8.3 Unsolicited startup banner

After boot, the firmware **must** emit exactly one unsolicited banner, with no following `ok`:

```text
boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0
```

The web client must accept and parse this banner. After the banner, the firmware stays silent until a command arrives.

The current calibration/gallery sketches print a human help banner unsolicited. v1 firmware **must not** print that help text.

If the client opens a port on a board that did **not** reset, there may be no new banner. The client must still run the connection sequence in [§14](#14-reset-and-reconnection-behavior), including `ZERO CLEAR`.

### 8.4 Position reporting

When zero is not set, Cartesian position is unknown. The firmware must **not** report a fictional `(0,0)`.

| Zero | Field |
| --- | --- |
| Unset | `position=unknown` |
| Set | `position=<x>,<y>` with exactly 3 decimal places, comma, no spaces |

Examples:

```text
position=unknown
position=0.000,0.000
position=-15.000,20.500
```

`HELLO`, `INFO`, and `STATUS` never return `error:zero-required`.

---

## 9. Machine states

v1 has two **observable** control states via `STATUS`, plus one internal condition:

| Condition | Observable? | Meaning |
| --- | --- | --- |
| Idle, zero unset | `STATUS`: `state=idle zero=0 position=unknown` | Accepts identification, config, `NUDGE`, pen, `ZERO`, `ZERO CLEAR`, `ABORT`. Rejects Cartesian `JOG`/`G0`/`G1`. |
| Idle, zero set | `STATUS`: `state=idle zero=1 position=…` | Accepts Cartesian plotting and jog, plus the commands above. `NUDGE` is still allowed and clears zero. |
| Busy (blocking) | Not reported by `STATUS` | Executing one command. No line is parsed until that command emits `ok` or `error:…`. |

There is no paused firmware state in v1.

`STATUS` fields when a reply is possible:

| Field | Values | Meaning |
| --- | --- | --- |
| `state` | `idle` | Parser is idle (the only `STATUS` state in v1) |
| `zero` | `0` / `1` | Whether `ZERO` is in effect |
| `position` | `unknown` or `x,y` | Logical Cartesian pose |
| `pen` | `up` / `down` | Last completed pen command |

`HELLO` and `INFO` expose `protocol` and `firmware`. `INFO` also exposes capabilities and geometry. `CONFIG?` exposes the active configuration keys.

---

## 10. State transitions

```text
                    firmware boot
                         │
                         │  banner: zero=0
                         ▼
              ┌─────────────────────┐
              │ idle, zero=0        │◄── ZERO CLEAR
              │ position=unknown    │◄── NUDGE
              │ pen=up after boot   │◄── kinematic CONFIG
              └─────────┬───────────┘
         NUDGE / PEN    │
         CONFIG / ABORT │ ZERO
                 ▲      ▼
                 │    ┌─────────────────────┐
                 │    │ idle, zero=1        │
                 │    │ position known      │
                 │    └─────────┬───────────┘
                 │         G0/G1/JOG/PEN/NUDGE/…
                 │              ▼
                 │    ┌─────────────────────┐
                 │    │ busy (internal)     │
                 │    │ blocking; no parse  │
                 │    └─────────┬───────────┘
                 │              │ ok or error:…
                 └──────────────┘  back to idle
                                   (NUDGE returns zero=0)
```

Additional rules:

- Firmware boot: `idle`, `zero=0`, `position=unknown`, pen driven to `PEN_UP_ANGLE`, compiled `CONFIG` defaults.
- `ZERO CLEAR`: `zero=0`, `position=unknown`, motors do not move, pen unchanged.
- `NUDGE`: always `zero=0`, `position=unknown` after completion.
- Successful kinematic `CONFIG`: `zero=0`, `position=unknown`.
- Cartesian `JOG` after zero: `zero` remains `1`, `position` updates.
- `ABORT`: pen up; carriage unmoved; `zero` and `position` unchanged.
- `G21` and `G90` do not change motion or zero.
- `PAUSE` / `RESUME` do not change state; they return `error:not-supported`.

A new web-client connection always sends `ZERO CLEAR`, so the session starts Cartesian-disabled even if the MCU did not reboot.

---

## 11. Configuration keys

`CONFIG?` prints every key, one per line, then `ok`.  
`CONFIG <KEY>=<VALUE>` sets one writable key. The firmware validates the **complete** resulting configuration before committing. On success it echoes `KEY=VALUE` then `ok`. On failure it returns an error and leaves **every** previous value unchanged.

Values after reboot match the compiled defaults below.

### 11.1 Identity (read-only)

| Key | Reset value | Writable |
| --- | --- | --- |
| `PROTOCOL_VERSION` | `1` | no |
| `FIRMWARE_VERSION` | `0.1.0` | no |
| `MACHINE_NAME` | `moebius-polargraph` | no |

Writing a read-only key returns `error:not-supported`.

### 11.2 Kinematic configuration (writable; successful change clears zero)

| Key | Reset value | Unit | Firmware source |
| --- | --- | --- | --- |
| `X_SEPARATION` | `820` | mm | `#define X_SEPARATION 820` |
| `ZERO_DEPTH` | `520` | mm | `#define LIMYMIN 520` |
| `SPOOL_DIAMETER` | `35` | mm | `#define SPOOL_DIAMETER 35` |
| `STEPS_PER_TURN` | `2048` | steps | `#define STEPS_PER_TURN 2048` |

Any **successful change** to a kinematic value immediately sets `zero=0` and `position=unknown`. A write of the already-stored value is `ok`, echoes the value, and does **not** clear zero.

After a kinematic change, derived read-only values are recomputed:

| Key | Formula | Default |
| --- | --- | --- |
| `LIMXMIN` | `-X_SEPARATION * 0.5` | `-410` |
| `LIMXMAX` | `+X_SEPARATION * 0.5` | `410` |
| `SPOOL_CIRC` | `SPOOL_DIAMETER * 3.1416` | `109.956` |
| `TPS` | `SPOOL_CIRC / STEPS_PER_TURN` | ≈ `0.053689` |

`LIMXMIN` / `LIMXMAX` are IK anchor X coordinates, not the software workspace.

### 11.3 Workspace configuration (writable; does not move the machine)

| Key | Reset value | Unit |
| --- | --- | --- |
| `X_MIN` | `-100.0` | mm |
| `X_MAX` | `+100.0` | mm |
| `Y_MIN` | `-100.0` | mm |
| `Y_MAX` | `+100.0` | mm |

Workspace writes do not clear zero by themselves. The **entire** resulting rectangle is validated before any key is stored. If validation fails, the previous configuration is unchanged.

If `zero=1` and the current `position` would lie outside the new rectangle, the write is `error:outside-workspace` and nothing changes. That avoids a pose that can no longer legally `JOG` or `G0`/`G1`.

### 11.4 Pen configuration (writable; does not clear zero)

| Key | Reset value | Unit | Firmware source |
| --- | --- | --- | --- |
| `PEN_UP_ANGLE` | `90` | degrees | `#define PEN_UP_ANGLE 90` |
| `PEN_DOWN_ANGLE` | `60` | degrees | `#define PEN_DOWN_ANGLE 60` |
| `PEN_SETTLE_MS` | `500` | ms | `#define PEN_SETTLE_MS 500` |

Changing an angle does not move the servo until the next pen command. Changing `PEN_SETTLE_MS` applies to subsequent pen commands only.

### 11.5 Representational validation ranges

These ranges reject implausible ASCII values. They are **not** a guarantee that every accepted value is mechanically safe.

| Key | Accepted range |
| --- | --- |
| `X_SEPARATION` | `100` to `3000` mm |
| `ZERO_DEPTH` | `100` to `3000` mm |
| `SPOOL_DIAMETER` | `5` to `100` mm |
| `STEPS_PER_TURN` | `1` to `100000` |
| `X_MIN`, `X_MAX`, `Y_MIN`, `Y_MAX` | `-2500` to `+2500` mm |
| `PEN_UP_ANGLE`, `PEN_DOWN_ANGLE` | `0` to `180` degrees |
| `PEN_SETTLE_MS` | `0` to `5000` ms |

Workspace validation must additionally require all of:

```text
X_MIN < X_MAX
Y_MIN < Y_MAX
Y_MAX < ZERO_DEPTH
X_MIN < 0 < X_MAX     ; logical (0,0) inside in X
Y_MIN < 0 < Y_MAX     ; logical (0,0) inside in Y
```

A kinematic write that would break `Y_MAX < ZERO_DEPTH` is rejected; previous configuration unchanged.

Unknown key → `error:invalid-parameter`.  
No `CONFIG` command writes EEPROM.  
Reboot restores compiled defaults.

Motion timing (`STEP_DELAY_US`, TinyStepper speed/acceleration) is **not** a v1 `CONFIG` surface. Firmware uses the validated compiled values from the drawing sketches.

---

## 12. Complete command reference

Unless noted, success is optional informational lines plus terminal `ok`.

### 12.1 Identification

#### `HELLO`

Handshake. Reply:

```text
machine=moebius-polargraph protocol=1 firmware=0.1.0
ok
```

Does not move motors or the pen. Does not change zero.

#### `INFO`

Identity, versions, capabilities, and active geometry. Field order is fixed:

```text
machine=moebius-polargraph
protocol=1
firmware=0.1.0
geometry=polargraph
queue=0
eeprom=0
homing=0
feedrate=0
x_separation=820
zero_depth=520
spool_diameter=35
steps_per_turn=2048
x_min=-100.000
x_max=100.000
y_min=-100.000
y_max=100.000
pen_up_angle=90
pen_down_angle=60
pen_settle_ms=500
nudge_max_steps=200
ok
```

`feedrate=0` means Cartesian feedrate control is **unavailable** in protocol v1. The SchoolAI Plotter Workspace Moebius controller must omit `F` when generating commands.

`INFO` reports active RAM configuration, not only compile-time defaults.

#### `STATUS`

Live state. Because `STATUS` is only parsed while idle:

When zero is unset:

```text
state=idle
zero=0
position=unknown
pen=up
ok
```

When zero is set:

```text
state=idle
zero=1
position=0.000,0.000
pen=up
ok
```

### 12.2 Configuration

#### `CONFIG?`

Dump all keys from [§11](#11-configuration-keys), one `KEY=VALUE` line each, then `ok`.

#### `CONFIG <KEY>=<VALUE>`

Validate the complete resulting configuration. Commit all-or-nothing. Echo the canonical key and stored value, then `ok`.

Example:

```text
> CONFIG PEN_UP_ANGLE=90
PEN_UP_ANGLE=90
ok
```

### 12.3 Manual operation

#### `NUDGE <DIRECTION> <STEPS>`

Raw physical positioning. **Not** Cartesian millimetres. **Not** inverse kinematics.

| Token | Meaning |
| --- | --- |
| `UP` | physically up |
| `DOWN` | physically down |
| `LEFT` | physically left |
| `RIGHT` | physically right |

Motor pairings are the validated calibration/gallery `rawJogPair` directions:

| Direction | `m1` (pins 7–10, left IK cord) | `m2` (pins 2,3,5,6, right IK cord) | Same as key |
| --- | --- | --- | --- |
| `UP` | reel in | reel in | `W` |
| `DOWN` | reel out | reel out | `S` |
| `LEFT` | reel in | reel out | `A` |
| `RIGHT` | reel out | reel in | `D` |

Rules:

- Allowed before zero is set.
- Intended only for manually positioning the carriage before `ZERO`.
- Does not claim geometrically exact Cartesian movement.
- Must **never** update Cartesian XY; after `ok`, `position=unknown`.
- Must **clear** logical zero if zero was set.
- If the pen is down, raise it first and wait `PEN_SETTLE_MS` (same safety as current `rawJogPair`).
- `STEPS` is a positive integer motor-step magnitude applied with the pairing above (both motors step `STEPS` times in the tabulated directions).
- Maximum `STEPS` per command: **`200`** (`nudge_max_steps` in `INFO`). That is ten times the calibration `RAW_JOG_STEPS` of `20`, as a conservative per-command cap, not a measured mechanical limit.
- `STEPS` of `0`, negative, non-integer, missing, or greater than `200` → `error:invalid-parameter`.
- Unknown direction → `error:invalid-parameter`.
- Extra tokens → `error:invalid-command`.

`NUDGE` has no software XY workspace check. The operator is responsible for slack, spool flanges, and the frame.

#### `JOG X <millimetres>` / `JOG Y <millimetres>`

Relative Cartesian move along one logical axis, using inverse kinematics and `line_safe()` / `moveto()`.

Rules:

- Allowed **only after** zero is set. Otherwise `error:zero-required` and no motion.
- Updates the current logical XY.
- **Preserves** logical zero.
- Obeys the software workspace; out-of-rectangle targets → `error:outside-workspace`.
- If the pen is down, raise it first and wait `PEN_SETTLE_MS` (manual positioning, not inking).
- `JOG X 0` / `JOG Y 0` raise the pen if needed and return `ok` without stepping.
- Non-numeric distance → `error:invalid-parameter`.

#### `ZERO`

Declare the current physical pose as logical `(0,0)`. Does not move motors. Does not change pen state.

Firmware action, matching `teleport(0.0, 0.0)`:

1. Set logical pose to `(0,0)`.
2. Compute cord steps with IK at `(0,0)`.
3. Store those steps as the current cord-step counters.
4. Set `zero=1`.

Reply:

```text
zero=1 position=0.000,0.000
ok
```

The operator must already have placed the carriage at the geometric origin (X centered, `ZERO_DEPTH` below the anchors). The protocol cannot verify that.

#### `ZERO CLEAR`

Invalidate logical zero without moving the motors and without changing pen state.

Reply:

```text
zero=0 position=unknown
ok
```

Cord-step counters used for Cartesian IK are undefined after this command until the next `ZERO`. Cartesian `JOG`/`G0`/`G1` must then return `error:zero-required`.

The SchoolAI Plotter Workspace Moebius controller sends `ZERO CLEAR` on every new connection before enabling Cartesian movement or plotting.

#### `PEN UP`

Alias of `M5`. See [§12.4](#124-plotting).

#### `PEN DOWN`

Alias of `M3`. See [§12.4](#124-plotting).

### 12.4 Plotting

Modal state after boot: millimetres (`G21`) and absolute coordinates (`G90`) are already active. Clients should still send both before a plot for explicitness.

#### `G21`

Select millimetres. No motion. `ok`.

#### `G90`

Select absolute Cartesian mode. No motion. `ok`.

`G20` (inches) and `G91` (relative) → `error:not-supported`.

#### `G0 X... Y...`

Rapid traverse to an **absolute** logical XY using the firmware’s **fixed** validated motion speed and acceleration. Requires `zero=1`, else `error:zero-required`.

Does **not** change pen state. The client must send `M5` / `PEN UP` before travel if the pen must be up.

Uses `line_safe()` so the path is subdivided in Cartesian space before IK.

At least one of `X`,`Y` required. Workspace validation applies.

If an `F` parameter is present → `error:not-supported` (no motion).

#### `G1 X... Y...`

Coordinated draw/travel move to an **absolute** logical XY, same fixed validated motion timing as `G0`. Requires `zero=1`.

Does **not** change pen state. The client must send `M3` / `PEN DOWN` to ink.

In protocol v1, `G0` and `G1` use the same compiled motion speed. The distinction is semantic for the client (travel vs. drawn segment), not a feedrate difference.

If an `F` parameter is present → `error:not-supported` (no motion).

Feedrate support is reserved for a future compatible extension or later protocol version.

#### `M3`

Pen **down**: servo to `PEN_DOWN_ANGLE` (default `60`). Allowed before zero. `ok` **only after** `PEN_SETTLE_MS` (default `500`). No `S` parameter.

#### `M5`

Pen **up**: servo to `PEN_UP_ANGLE` (default `90`). Same settle rule as `M3`.

`PEN DOWN` is an alias for `M3`. `PEN UP` is an alias for `M5`.

The client is always responsible for explicit pen commands. `G0` does not automatically raise the pen. `G1` does not automatically lower the pen.

Any other G/M code (`G2`, `G4`, `G28`, `M30`, `M114`, …) → `error:not-supported` if the token looks like G-code, else `error:invalid-command`.

### 12.5 Control

#### `ABORT`

Processed only between completed blocking commands (when the parser is idle).

1. Raise the pen (`M5` semantics) and wait `PEN_SETTLE_MS`.
2. Do not move the carriage.
3. Do not return to `(0,0)`.
4. Preserve logical zero and the last known Cartesian `position` (or `unknown` if zero was already unset).
5. Reply `ok` after the pen-up settle delay.

`ABORT` does not interrupt a movement currently executing. If a client sends `ABORT` while a command is in flight, compliant clients must not do that; they wait first. Bytes that arrive early sit in the UART buffer and run only after the current command’s terminal line.

There is no protocol-level command queue to clear beyond “the next unparsed line.” `ABORT` still raises the pen so a cancelled job does not leave the pen down.

Allowed before zero (pen up only).

#### Reserved: `PAUSE` / `RESUME`

Not in the v1 command set. If received:

```text
error:not-supported
```

Firmware v1 cannot pause a movement already in progress. The web client pauses by not sending the next command.

---

## 13. Validation and safety rules

### 13.1 Command admission

| Condition | Response |
| --- | --- |
| Unknown command | `error:invalid-command` |
| Line overflow / illegal characters / tab | `error:invalid-command` |
| Bad number, bad key, bad `NUDGE` direction/steps, missing required axis | `error:invalid-parameter` |
| Feature outside v1 (`F`, `PAUSE`, `RESUME`, `G20`, `G91`, …) | `error:not-supported` |
| `JOG` / `G0` / `G1` while `zero=0` | `error:zero-required` |
| Cartesian target outside workspace or at/above the anchor line | `error:outside-workspace` |
| `CONFIG` that fails complete validation | `error:invalid-parameter` or `error:outside-workspace` as specified; previous config unchanged |

A rejected command has **no** motor motion and **no** pen motion.

### 13.2 Cartesian target checks (`G0`, `G1`, `JOG`)

Apply in order to the **target** logical pose:

1. Coordinates must be finite after parsing.
2. `X_MIN ≤ X ≤ X_MAX` and `Y_MIN ≤ Y ≤ Y_MAX`; else `error:outside-workspace`.
3. `Y < ZERO_DEPTH`; else `error:outside-workspace` (anchor line). With a valid workspace, `Y_MAX < ZERO_DEPTH` already implies this for in-rectangle targets.
4. IK cord lengths must be strictly greater than zero after rounding to steps; otherwise `error:outside-workspace`.

The firmware does not simulate cord slack, spool overrun, or paper size. Cords are approximately `2.30` m each per the README; that length is not a v1 `CONFIG` key.

### 13.3 `NUDGE`

No XY workspace check. Step-count limits as in [§12.3](#123-manual-operation).

### 13.4 Streaming and memory

The Arduino must not allocate a path buffer for the whole drawing. After the terminal line, the firmware may forget the command text. Persisted in RAM: last completed Cartesian pose (only if `zero=1`), last cord-step counters (only if `zero=1`), pen flag, zero flag, and RAM config.

---

## 14. Reset and reconnection behavior

### 14.1 Firmware boot (power cycle, reset button, watchdog, or a board that actually reboots on port open)

| Item | Value |
| --- | --- |
| Unsolicited banner | `boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0` |
| `zero` | `0` |
| `position` | `unknown` |
| Control | idle |
| Pen | `PEN_UP_ANGLE` in `setup()`, as current sketches do |
| RAM `CONFIG` | compiled defaults |
| EEPROM | not read, not written |
| Modal G-code | `G21` and `G90` active |

### 14.2 New web-client connection (reset is not assumed)

Do **not** assume that opening a serial port resets every compatible Arduino. A previous session’s zero, pose, and RAM `CONFIG` may still be live.

Required client sequence on **every** new Moebius connection:

1. Open `115200 8N1`. Drain incoming bytes. Parse a `boot …` banner if one arrives; proceed if none arrives.
2. `HELLO` — require `protocol=1`. Read `firmware=`.
3. `ZERO CLEAR` — invalidate zero before any Cartesian enablement.
4. `INFO` and/or `CONFIG?` — read kinematics, workspace, and `feedrate=0`.
5. `STATUS` — require `zero=0` and `position=unknown` before enabling plot.
6. Operator uses `NUDGE` (and pen tests), then `ZERO`.
7. `STATUS` until `zero=1` and `position=0.000,0.000`.
8. Only then stream `G0`/`G1` / Cartesian `JOG`.

If `HELLO` returns another protocol version, the client must not send v1 plotting commands.

After disconnect or power loss, the client must not resume a drawing. There is no recovery protocol. A new connection repeats the sequence above, including a new manual `ZERO`.

---

## 15. Example sessions

Lines starting with `>` are client → firmware. Other lines are firmware → client. Comments in parentheses are not on the wire.

### 15.1 Connect, raw nudge, declare zero

```text
boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0

> HELLO
machine=moebius-polargraph protocol=1 firmware=0.1.0
ok

> ZERO CLEAR
zero=0 position=unknown
ok

> INFO
machine=moebius-polargraph
protocol=1
firmware=0.1.0
geometry=polargraph
queue=0
eeprom=0
homing=0
feedrate=0
x_separation=820
zero_depth=520
spool_diameter=35
steps_per_turn=2048
x_min=-100.000
x_max=100.000
y_min=-100.000
y_max=100.000
pen_up_angle=90
pen_down_angle=60
pen_settle_ms=500
nudge_max_steps=200
ok

> STATUS
state=idle
zero=0
position=unknown
pen=up
ok

> NUDGE UP 20
ok

> NUDGE LEFT 20
ok

> ZERO
zero=1 position=0.000,0.000
ok

> STATUS
state=idle
zero=1
position=0.000,0.000
pen=up
ok
```

### 15.2 Stream a 30 mm square (no `F`)

The browser holds the path. Pen commands are explicit. `G0`/`G1` use fixed firmware motion timing.

```text
> G21
ok
> G90
ok
> M5
ok
> G0 X-15.000 Y-15.000
ok
> M3
ok
> G1 X15.000 Y-15.000
ok
> G1 X15.000 Y15.000
ok
> G1 X-15.000 Y15.000
ok
> G1 X-15.000 Y-15.000
ok
> M5
ok
> G0 X0.000 Y0.000
ok
> STATUS
state=idle
zero=1
position=0.000,0.000
pen=up
ok
```

### 15.3 Errors, client-side pause, abort, reconnect without assuming reset

```text
(zero not yet set)

> G1 X10.000 Y0.000
error:zero-required

> JOG X 5
error:zero-required

> W
error:invalid-command

> G91
error:not-supported

> PAUSE
error:not-supported

> G1 X10.000 Y0.000 F200
error:not-supported

> NUDGE UP 0
error:invalid-parameter

> NUDGE UP 201
error:invalid-parameter

> CONFIG NO_SUCH_KEY=1
error:invalid-parameter

> ZERO
zero=1 position=0.000,0.000
ok

> JOG X 10
ok
> STATUS
state=idle
zero=1
position=10.000,0.000
pen=up
ok

> G0 X0.000 Y150.000
error:outside-workspace

(client pauses the job by not sending the next G1)

> G1 X20.000 Y0.000
ok

(client cancels: waits for the previous ok, then)

> ABORT
ok
> STATUS
state=idle
zero=1
position=20.000,0.000
pen=up
ok

> NUDGE DOWN 20
ok
> STATUS
state=idle
zero=0
position=unknown
pen=up
ok

(new web-client connection; banner may be absent if the board did not reboot)

> HELLO
machine=moebius-polargraph protocol=1 firmware=0.1.0
ok
> ZERO CLEAR
zero=0 position=unknown
ok
> STATUS
state=idle
zero=0
position=unknown
pen=up
ok
```

`Y=150` is rejected because the default software workspace is `Y_MAX=+100.0`.

---

## 16. Compatibility and versioning

- The protocol name is **Moebius Serial Protocol**. The integer `protocol=1` is the major version. Firmware `0.1.0` is the initial experimental build identifier and may change without changing `protocol`.
- v1 clients must refuse to plot if `protocol` is missing or not `1`.
- Additive backward-compatible commands may appear later only with an explicit, approved spec change. This document does not define a minor protocol field on the wire.
- Removing a command, changing a terminal error token, changing the coordinate system, or making `ok` mean “accepted but not finished” requires **major** version 2.
- This protocol is not GRBL-compatible.
- Current calibration/gallery single-key commands (`W`,`Z`,`F`,…) are **not** part of v1 (`error:invalid-command`).
- `PAUSE` and `RESUME` remain reserved names that return `error:not-supported` in v1.

---

## 17. MVP limitations

Document these in the web UI as well as here:

1. No homing; a wrong `ZERO` produces a wrong drawing with no firmware alarm.
2. No limit switches or encoders; `NUDGE` has no XY software limits.
3. Default `±100` mm workspace is a conservative software envelope, not full mechanical reach.
4. No command queue; the browser is the buffer.
5. `ok` is completion, so throughput is one segment per round trip.
6. No firmware pause; the client pauses by not sending.
7. `ABORT` does not interrupt a command already executing.
8. No calibrated Cartesian feedrate; `F` is `error:not-supported`.
9. `G0` and `G1` share the same compiled motion timing.
10. The Arduino does not store the drawing and cannot resume after disconnect or power loss.
11. No EEPROM; configuration returns to compiled defaults after reboot.
12. No SD card.
13. Opening the serial port might not reboot the board; the client must send `ZERO CLEAR`.
14. Pen-up travel is the client’s duty (`M5` then `G0`). Firmware will `G1` with the pen down if asked.
15. Motor pin left/right **names** differ between the two-stepper test and the drawing sketches; the protocol never exposes those names.

---

## 18. Future extensions outside v1

Do not implement in v1:

- complete GRBL emulation;
- SD-card listing, selection, and playback;
- EEPROM / NVM machine profiles;
- automatic homing, encoders, or stall detection;
- firmware look-ahead queue, acceleration across segments, or cornering;
- onboard storage of complete drawings;
- recovery after power loss or disconnect;
- real-time interrupt bytes (GRBL `!`, `~`, `0x18`);
- immediate mid-motion pause or abort (including TinyStepper `setupStop()` as a protocol feature);
- calibrated Cartesian feedrate (`F`);
- firmware `PAUSE` / `RESUME` as working commands;
- `G2`/`G3` arcs, `G4` dwell, `G28`, workspace offsets;
- inches or relative mode;
- independent per-motor step commands;
- SVG parsing on the Arduino;
- Wi-Fi, Bluetooth, or USB-HID alternatives to this UART protocol.

A later protocol version may enlarge `X_MIN`…`Y_MAX` after hardware testing, still via `CONFIG`.

---

## 19. Firmware implementation notes

This specification is approved for a future implementation. This documentation task does not implement firmware.

### 19.1 Provenance and licenses

- Start from the calibration sketch kinematics, not from GRBL and not from the two-stepper AccelStepper test.
- Keep Walldraw MIT attribution and Lina Lopes copyright headers on derived sketch files.
- Do not relicense TinyStepper_28BYJ_48 (MIT, S. Reifel).
- Do not pull AccelStepper into the serial drawing firmware (GPL V2 or Commercial).
- Do not `#include <SD.h>` in v1.
- Do not add EEPROM writes.
- Do not rename existing files in this repository as part of protocol work.

### 19.2 Reuse from current drawing sketches

| Behavior | Current implementation |
| --- | --- |
| IK | `IK()` using `LIMXMIN`/`LIMXMAX`/`LIMYMIN`/`TPS` (`LIMYMIN` = protocol `ZERO_DEPTH`) |
| Teleport on zero | `teleport(0,0)` |
| Clear zero | protocol `ZERO CLEAR`; also `NUDGE` and kinematic `CONFIG` |
| Straight XY | `line_safe()` then `moveto()` as **one blocking command** |
| Two-motor coordination | Bresenham-style loop, `STEP_DELAY_US` `1` |
| Directions | `M1_REEL_OUT=1`, `M1_REEL_IN=-1`, `M2_REEL_OUT=-1`, `M2_REEL_IN=1` |
| Pins | `m1`: 7,8,9,10; `m2`: 2,3,5,6; servo `A0`; pin `4` unused |
| Speed/accel | `10000` / `100000` (compiled, not `CONFIG`) |
| Pen | `90` up, `60` down, `500` ms settle |
| Baud | `115200`, `8N1` |
| Raw positioning | `NUDGE` uses the `W/A/S/D` pairings, not millimetre IK |

Internal mapping: `m1`/`l1` is the cord to the **left** IK anchor (pins 7–10). `m2`/`l2` is the cord to the **right** IK anchor (pins 2,3,5,6). The two-stepper test names those pin groups `right` and `left` respectively. Do not “fix” names without hardware re-validation.

### 19.3 Parser

- `char line[81];` plus an index; never `String`.
- Read a line only in the idle parser. While `moveto()` or a servo delay runs, do not parse protocol commands.
- Compare commands case-insensitively.
- Emit PROGMEM/`F()` string literals for all replies.
- After overflow, resync to `LF`.
- Emit the mandatory `boot` banner once from `setup()` after `Serial.begin`.

### 19.4 Completion semantics

`ok` is printed only after stepper loops have returned and, for pen motion, after `PEN_SETTLE_MS`.

Do not print `ok` when a move is merely accepted. Do not print a terminal `busy` line. v1 has no queue.

Do not poll `PAUSE`/`ABORT` between internal `line_safe()` pieces of a single `G0`/`G1`/`JOG` in v1. Those pieces are not protocol commands.

---

## 20. Web-client implementation notes

SchoolAI Plotter Workspace Moebius controller must:

1. Use Web Serial at `115200`, 8 data bits, no parity, 1 stop bit.
2. After `open()`, drain bytes and parse `boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0` if present. **Do not** assume a reset occurred.
3. Send `HELLO`. Abort the session if `protocol` is not `1`.
4. Send `ZERO CLEAR` before enabling Cartesian movement or plotting.
5. Send `INFO` / `CONFIG?` / `STATUS`. Require `feedrate=0` handling: **never generate `F`**.
6. Present raw positioning as `NUDGE UP|DOWN|LEFT|RIGHT <steps>`, labelled as raw motor positioning, **not** as millimetres.
7. Present Cartesian `JOG` only after `zero=1`.
8. Disable Plot until `STATUS` shows `zero=1` and `position` is not `unknown`.
9. Flatten SVG/paths on the computer. Stream `G1` point-to-point without `F`.
10. Use millimetres, `+X` right, `+Y` up, absolute coordinates. Convert from SVG’s often-downward `Y` before sending. Clip to `X_MIN`…`Y_MAX`.
11. Wait for `ok` or `error:…` on every line. Never pipeline. Never wait for `busy`.
12. Pause by not sending the next command.
13. Cancel by stopping transmission, waiting for the in-flight terminal line, then sending `ABORT`.
14. Before travel: `M5` then `G0`. Before ink: `M3` then `G1`. Do not rely on `G0`/`G1` to change pen state.
15. On `error:zero-required`, stop the job and require a new `ZERO`.
16. On `error:outside-workspace`, stop the job and show the rejected target.
17. On disconnect, do not resume the job. On reconnect, repeat `HELLO` + `ZERO CLEAR` + manual `ZERO`.
18. Do not send GRBL real-time bytes, `$` settings, or `?`.
19. Do not offer SD-card, EEPROM, homing, or feedrate UI against a v1 device.
20. Optionally send `G21` and `G90` at job start.

---

## 21. Acceptance-test checklist

Firmware claiming Moebius Serial Protocol v1 must pass the following. Tests use `115200 8N1`, `LF` termination, and a single command outstanding.

### Identification

- [ ] After boot, unsolicited `boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0`.
- [ ] `HELLO` returns `protocol=1` and `firmware=0.1.0`, then `ok`.
- [ ] `INFO` includes `queue=0`, `eeprom=0`, `homing=0`, `feedrate=0`, defaults `820`, `520`, `35`, `2048`, workspace `±100`, pen `90`/`60`/`500`, `nudge_max_steps=200`.
- [ ] `STATUS` after boot has `state=idle`, `zero=0`, `position=unknown`, `pen=up`.

### Framing

- [ ] `CR LF` and `LF` both parse.
- [ ] Empty lines produce no terminal response.
- [ ] Unknown command `FOO` → `error:invalid-command`.
- [ ] Line longer than 80 bytes → `error:invalid-command`, parser resynchronizes.
- [ ] `G20`, `G91`, `PAUSE`, `RESUME` → `error:not-supported`.

### Zero gate and `ZERO CLEAR`

- [ ] `G0 X0 Y0` before `ZERO` → `error:zero-required` and no motion.
- [ ] `G1 X1 Y0` before `ZERO` → `error:zero-required`.
- [ ] `JOG X 1` before `ZERO` → `error:zero-required` and no motion.
- [ ] `NUDGE UP 20` before `ZERO` → motion, then `ok`; `STATUS` still `zero=0`, `position=unknown`.
- [ ] `M3` / `PEN DOWN` before `ZERO` → servo moves; `ok` only after settle time.
- [ ] `ZERO` → `zero=1 position=0.000,0.000` and `ok`.
- [ ] `ZERO CLEAR` → `zero=0 position=unknown`; motors do not move; subsequent `G1` → `error:zero-required`.
- [ ] `G0`/`G1`/`JOG` after `ZERO` are accepted if inside the workspace.

### `NUDGE` versus `JOG`

- [ ] `NUDGE UP 20` after `ZERO` clears zero and reports `position=unknown`.
- [ ] `JOG X 10` after `ZERO` preserves zero and reports `position=10.000,0.000` (from origin).
- [ ] `NUDGE UP 0`, `NUDGE UP -1`, `NUDGE UP 201` → `error:invalid-parameter` and no motion.
- [ ] `NUDGE FORWARD 20` → `error:invalid-parameter`.

### Configuration

- [ ] `CONFIG?` lists compiled defaults from [§11](#11-configuration-keys).
- [ ] `CONFIG PEN_DOWN_ANGLE=60` echoes and `ok`; reboot restores compiled pen defaults.
- [ ] `CONFIG X_SEPARATION=820` with unchanged value does not clear zero.
- [ ] `CONFIG X_SEPARATION=800` clears zero (`STATUS` `zero=0`, `position=unknown`).
- [ ] `CONFIG Y_MAX=600` with default `ZERO_DEPTH=520` → error; previous config unchanged (`Y_MAX < ZERO_DEPTH`).
- [ ] `CONFIG X_MIN=10` → error (`(0,0)` must lie inside); previous config unchanged.
- [ ] `CONFIG PROTOCOL_VERSION=2` → `error:not-supported`.
- [ ] `CONFIG NOPE=1` → `error:invalid-parameter`.
- [ ] No EEPROM write (code review / reboot test).

### Motion and pen

- [ ] `PEN UP` / `M5` and `PEN DOWN` / `M3` use `90` / `60`; `ok` only after ≥ settle time.
- [ ] After zero, `G0 X10 Y0` then `STATUS` shows `position=10.000,0.000`.
- [ ] After zero, `G1` with omitted `Y` keeps previous `Y`.
- [ ] `G0 X0 Y150` → `error:outside-workspace` (default `Y_MAX=100`).
- [ ] `G1 X10 Y0 F200` → `error:not-supported` and no motion.
- [ ] `G0 X10 Y0 F200` → `error:not-supported` and no motion.
- [ ] `G0` with pen down does not raise the pen.
- [ ] Client wait: second command sent only after `ok` executes in order.

### Abort and pause

- [ ] `PAUSE` / `RESUME` → `error:not-supported`.
- [ ] After a finished `G1`, `ABORT` raises pen; `STATUS` `pen=up`; pose equals last completed segment; `zero` unchanged.
- [ ] Documented limitation: a single long `G1` is not interrupted mid-command.

### Reset and connection

- [ ] Hardware reset: banner, `STATUS` `zero=0`, `position=unknown`, config defaults restored.
- [ ] Client sequence `HELLO` then `ZERO CLEAR` leaves Cartesian commands rejected until a new `ZERO`.

### Negative / safety

- [ ] No Arduino `String` in the parser (code review).
- [ ] No SD and no EEPROM APIs in the v1 sketch (code review).
- [ ] Third-party license headers remain (code review).
- [ ] Cartesian plotting commands never include `F` (client review).
- [ ] Pre-zero UI uses `NUDGE`, not millimetre `JOG` (client review).

---

## 22. Repository facts used

These values were read from the tree, not invented:

| Fact | Location |
| --- | --- |
| `STEPS_PER_TURN 2048` | Calibration and gallery sketches |
| `SPOOL_DIAMETER 35`, circumference factor `3.1416` | Same |
| `X_SEPARATION 820`, `LIMYMIN 520` (protocol `ZERO_DEPTH`), derived `±410` | Same |
| Pen `90` / `60`, settle `500` ms, servo `A0` | Same |
| Baud `115200` on drawing sketches; `9600` on two-stepper test | Sketches / README |
| `RAW_JOG_STEPS 20` | Calibration (basis for `nudge_max_steps=200` = 10×) |
| Direction constants `M1_REEL_OUT 1`, `M1_REEL_IN -1`, `M2_REEL_OUT -1`, `M2_REEL_IN 1` | Drawing sketches (from WallDrawDemo.ino) |
| Pin map `m1=7,8,9,10`, `m2=2,3,5,6`, pin `4` reserved | Sketches / README |
| TinyStepper speed `10000`, accel `100000` | Drawing `setup()` |
| IK, `teleport`, `line_safe`, Bresenham `moveto`, `rawJogPair` | Drawing sketches |
| Zero starts unset; `Z` declares origin; raw jog clears zero in current firmware | Drawing sketches |
| No endstops, no EEPROM, no G-code, no SD in active sketches | README / sketches |
| Equal cords ≈ `2.30` m, `0.24` mm PE line | README |
| Four-shape and gallery tests stay near logical zero (tens of mm) | Calibration / gallery |
| Licenses: firmware MIT (shihaipeng03 + Lina Lopes); docs CC BY 4.0; third-party separate | `LICENSE`, `CREDITS.md`, `LICENSES/` |

The default `±100` mm workspace is an approved protocol envelope around that empirically small drawing area. It is not a measured full-machine workspace.

---

## 23. Open decisions

No remaining protocol decisions block firmware implementation of v1.

Non-blocking notes (do not stall `0.1.0`):

- Sketches do not name a specific Arduino board. The pin map is Uno-style. v1 has no board identity key beyond `machine=moebius-polargraph`.
- `nudge_max_steps=200` is a protocol cap (10 × calibration `RAW_JOG_STEPS`). It may be revised later without changing `protocol=1` if testing requires it; `INFO` already exposes the value.
- v1 firmware omits the calibration `H` help banner. Serial Monitor users of the future sketch are expected to use this document or the web app.

---

## Document history

| Date | Notes |
| --- | --- |
| 2026-08-25 | First written design of Moebius Serial Protocol v1. No firmware implementation. |
| 2026-08-25 | Revision: approved identifiers, `NUDGE` vs Cartesian `JOG`, `ZERO CLEAR`, workspace `±100` mm, no firmware pause, `F` rejected, `position=unknown`, `busy` not emitted. No firmware implementation. |
