# Moebius Polargraph firmware

This document describes the sketches in this repository and the first physically validated serial firmware.

Command grammar, responses, and configuration keys for the serial firmware are defined in [SERIAL_PROTOCOL_V1.md](SERIAL_PROTOCOL_V1.md). This page does not repeat that specification.

This is original Moebius Polargraph documentation by Lina Lopes, licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

## Sketches

| Sketch | Role | Baud | Protocol |
| --- | --- | --- | --- |
| [`firmware/Moebius_Polargraph/`](../firmware/Moebius_Polargraph/) | Serial control from a computer | `115200` | Moebius Serial Protocol v1, firmware `0.1.0` |
| [`firmware/Moebius_Polargraph_Calibration/`](../firmware/Moebius_Polargraph_Calibration/) | Manual `W`/`A`/`S`/`D` positioning and four-shape test | `115200` | Single-key Serial Monitor UI |
| [`firmware/Moebius_Polargraph_Gallery/`](../firmware/Moebius_Polargraph_Gallery/) | Onboard heart, butterfly, ellipse, rectangle | `115200` | Single-key Serial Monitor UI |
| [`firmware/Moebius_Polargraph_TwoStepperTest/`](../firmware/Moebius_Polargraph_TwoStepperTest/) | Motor/driver smoke test | `9600` | Banner only; continuous motion |

Upload one sketch at a time. The serial firmware is the sketch intended for external interfaces.

## Validated hardware configuration

Lina Lopes validated firmware `0.1.0` on **25 August 2026** on her machine. This is not a claim of universal Polargraph compatibility.

| Item | Value |
| --- | --- |
| Controller | Arduino Uno-compatible board |
| Motors | two 28BYJ-48 with ULN2003 drivers |
| Pen servo | `A0`, up `90°`, down `60°`, settle `500` ms |
| Left IK cord motor (`m1`) | pins `7, 8, 9, 10` |
| Right IK cord motor (`m2`) | pins `2, 3, 5, 6` |
| Pin `4` | reserved (unused) |
| `X_SEPARATION` | `820` mm |
| `ZERO_DEPTH` | `520` mm below the anchor line |
| Spool diameter | `35` mm |
| Steps per turn | `2048` |
| Cords | two equal braided PE lines, about `2.30` m each, about `0.24` mm diameter |
| Motor supply | regulated external `5 V` |
| Serial | `115200 8N1`, newline |

## Upload requirements

1. Arduino IDE or `arduino-cli` targeting `arduino:avr:uno`.
2. Bundled [`TinyStepper_28BYJ_48`](../libraries/TinyStepper_28BYJ_48/) `1.0.0` from this repository (serial, calibration, gallery).
3. `Servo.h` from the Arduino IDE (not bundled).
4. Do not compile the serial firmware against AccelStepper. AccelStepper is only for the two-stepper test.
5. Do not enable the bundled SD library in these sketches.

## First-start workflow (serial firmware)

1. Upload `Moebius_Polargraph.ino`.
2. Open Serial Monitor at `115200`, newline.
3. Read `boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0`.
4. Send `HELLO`, then `STATUS`. Expect `zero=0` and `position=unknown`.
5. With the pen up, use `NUDGE … 20` (not `200`) to place the carriage at the geometric origin: centered between the cord exits, `520` mm below the anchors.
6. Send `ZERO`.
7. Confirm `STATUS` shows `zero=1` and `position=0.000,0.000`.
8. Only then send Cartesian `JOG`, `G0`, or `G1`.

A computer or web client that opens the port must send `ZERO CLEAR` before enabling plotting, because opening USB serial does not always reset the board.

## Safe manual test sequence

Keep the pen up (`M5`) until you intend to mark paper.

```text
HELLO
STATUS
NUDGE UP 20
NUDGE DOWN 20
NUDGE LEFT 20
NUDGE RIGHT 20
ZERO
M5
JOG X 10
JOG X -10
STATUS
```

If a nudge direction is wrong, stop, power down, and check motor wiring against Calibration/`m1`/`m2`, not the two-stepper test names.

## Physically validated command sequence

On 25 August 2026, Lina Lopes successfully ran the following class of tests on her machine (not performed by Cursor):

- startup banner, `HELLO`, `INFO`, `STATUS`;
- Cartesian rejection before zero;
- all four `NUDGE` directions;
- manual `ZERO`;
- Cartesian `JOG` and physical return to marked zero;
- pen up/down;
- `G21`, `G90`, `G0`, `G1`;
- `M3` / `M5`;
- a physical `30 × 30` mm square;
- return from that square to logical and physical `(0,0)`.

Example square after `ZERO` (client waits for `ok` after every line):

```text
G21
G90
M5
G0 X-15.000 Y-15.000
M3
G1 X15.000 Y-15.000
G1 X15.000 Y15.000
G1 X-15.000 Y15.000
G1 X-15.000 Y-15.000
M5
G0 X0.000 Y0.000
```

## Pen behavior

| Command | Effect |
| --- | --- |
| `M3` / `PEN DOWN` | Servo to pen-down angle; `ok` after settle |
| `M5` / `PEN UP` | Servo to pen-up angle; `ok` after settle |
| `G0` / `G1` | Do **not** change pen state |
| `JOG` | Raises the pen first if it is down |
| `NUDGE` | Raises the pen first if it is down |

## Zero lifecycle

| Event | Zero | Cartesian position |
| --- | --- | --- |
| Boot | unset | `unknown` |
| `ZERO CLEAR` | unset | `unknown` |
| Successful kinematic `CONFIG` change | unset | `unknown` |
| `NUDGE` | unset | `unknown` |
| `ZERO` | set at current pose as `(0,0)` | `0.000,0.000` |
| `JOG` / `G0` / `G1` | remains set | updated after the move completes |

`ZERO` does not move the motors. A wrong physical pose at `ZERO` produces a wrong drawing with no firmware alarm.

## Configuration lifecycle

Serial firmware configuration is RAM-only. Reboot restores compiled defaults. There is no EEPROM write.

Kinematic keys (`X_SEPARATION`, `ZERO_DEPTH`, `SPOOL_DIAMETER`, `STEPS_PER_TURN`) clear zero when the stored value actually changes. Workspace and pen keys do not clear zero by themselves. A rejected `CONFIG` leaves the previous complete configuration unchanged.

Accepted numeric ranges are protocol validation bounds, not proof that a value is safe on the physical machine. Do not enlarge `X_MIN`…`Y_MAX` before measuring the installation.

## Known limitations

Firmware `0.1.0` / protocol v1:

- no homing, limit switches, or encoders;
- no EEPROM or SD drawing;
- no calibrated feedrate (`F` → `error:not-supported`);
- no command queue;
- no mid-motion pause or abort;
- no job recovery after disconnect or power loss;
- default workspace `-100…+100` mm.

`PAUSE` and `RESUME` return `error:not-supported`. The client pauses by not sending the next line. `ABORT` raises the pen only between completed commands.

## Troubleshooting

| Symptom | Likely cause | What to do |
| --- | --- | --- |
| No startup banner | Wrong baud, wrong sketch, or the board did not reset when the port opened | Use `115200` newline for serial firmware; send `HELLO`. Do not assume USB open resets every board. |
| `error:zero-required` | `JOG` / `G0` / `G1` before `ZERO`, or after `NUDGE` / `ZERO CLEAR` / reboot | Place the carriage, send `ZERO`, then retry. |
| Wrong `NUDGE` direction | Left/right motors swapped vs Calibration `m1`/`m2` | Stop. Compare wiring to serial/calibration pin map, not TwoStepperTest `left`/`right` names. |
| Motor vibrates but does not move | Coil order, supply, or driver | Check 5 V motor supply, ULN2003 wiring, and TinyStepper pin order IN1–IN4. |
| Cord becomes slack | Unwound the wrong way, or a spool ran out | Cut power. Rewind with reserve on both spools. Reposition and `ZERO` again. |
| Carriage does not return physically to zero | Steps lost, wrong `ZERO`, or slack | Do not keep plotting. Raise the pen, nudge to the marked origin, `ZERO` again. |
| Pen angles are incorrect | Mechanism vs `90`/`60` | Test `M5` / `M3`. Change `PEN_UP_ANGLE` / `PEN_DOWN_ANGLE` only after a small test; a `CONFIG` write does not move the servo until the next pen command. |
| `error:not-supported` after sending `F` | Protocol v1 has no feedrate | Omit `F` from `G0`/`G1`. |
| Arduino resets when the serial port opens | Common Uno DTR reset | Expected on many Uno-class boards. Treat boot as `zero=0`. A client should still send `ZERO CLEAR`. |
| Firmware compiles with the wrong TinyStepper library | Library Manager copy instead of this tree | Point the IDE at [`libraries/TinyStepper_28BYJ_48`](../libraries/TinyStepper_28BYJ_48/). |
