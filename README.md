# Moebius Polargraph

Moebius Polargraph is a suspended two-cord wall-drawing machine in the **Machines to Draw** collection by Lina Lopes. Two upper stepper motors change the lengths of two cords attached to a pen holder; a servo raises and lowers the pen.

Created and maintained by **Lina Lopes**.

This is an open-source project. It uses **multiple licenses**. The complete repository is not uniformly MIT-licensed. See [License](#license).

## Current status

The machine is **experimental** and **physically validated** on Lina Lopes’s specific build. It is not a finished consumer product.

| Item | Value |
| --- | --- |
| Current serial firmware | `0.1.0` |
| Serial protocol | Moebius Serial Protocol v1 |
| Git tag | `firmware-v0.1.0` |
| Validated | 25 August 2026, on Lina Lopes’s machine |

Firmware `0.1.0` implements line-based serial control so a computer can stream Cartesian millimetre coordinates. Inverse kinematics and cord-step conversion stay on the Arduino.

That validation applies to the configuration listed below. It does **not** claim universal compatibility with every Polargraph, spool, motor, cord, power supply, or installation.

Standalone calibration and gallery sketches remain available for setup and onboard drawing tests. SD-card drawing is **not** implemented. Planned stages are in [Future SD Card Support](#future-sd-card-support).

## How It Works

The two motors sit above the drawing surface. Each motor winds or unwinds a cord connected to the pen holder. Changing both cord lengths together moves the holder up or down. Changing them in opposite directions moves it left or right.

The drawing firmware converts a requested XY position into two cord lengths (inverse kinematics), then steps the motors together with Bresenham-style coordination so both cords reach the target together. A servo on analog pin `A0` lifts the pen.

Logical `+X` is physically right. Logical `+Y` is physically up toward the motor anchors.

There is no automatic homing. Logical `(0,0)` is declared by hand after the carriage is placed at the geometric origin.

## Hardware

The current machine uses:

- two **28BYJ-48** stepper motors;
- two **ULN2003** drivers;
- an Arduino Uno-compatible controller;
- a servo to lift and lower the pen;
- two **35 mm** spools;
- two equal cords, approximately **2.30 meters** each, of **0.24 mm** round braided PE line;
- a regulated external **5 V** motor supply.

The thicker 0.24 mm line did not create a significant scale problem.

Pin numbers match a typical Arduino Uno-style digital layout. Pin `4` is left unused so a future SD-card interface can use the original Walldraw chip-select wiring.

Firmware pin map:

| Function | Pins |
| --- | --- |
| Motor on Arduino pins 7, 8, 9, 10 | IN1–IN4 (serial, calibration, and gallery: `m1`, left IK cord; two-stepper test: right motor) |
| Motor on Arduino pins 2, 3, 5, 6 | IN1–IN4 (serial, calibration, and gallery: `m2`, right IK cord; two-stepper test: left motor) |
| Pen-lift servo | `A0` (serial, calibration, and gallery) |
| Reserved (future SD chip select) | `4` |

The 28BYJ-48 half-step wiring order used by the two-stepper test is IN1, IN3, IN2, IN4. The serial, calibration, and gallery sketches pass IN1, IN2, IN3, IN4 to `TinyStepper_28BYJ_48`.

These geometry values are the **validated firmware defaults**:

| Parameter | Value |
| --- | --- |
| Steps per motor turn | `2048` |
| Spool diameter | `35` mm |
| Motor-anchor separation (`X_SEPARATION`) | `820` mm |
| Vertical offset of logical origin below the anchors (`ZERO_DEPTH` / `LIMYMIN`) | `520` mm |
| Pen-up / pen-down servo angles | `90°` / `60°` |
| Default software workspace (serial firmware) | `X,Y` in `-100` … `+100` mm |

Confirm that the physical left/right motors match the sketch you uploaded. The sketches name the same pin groups differently (`left`/`right` versus `m1`/`m2`).

## Repository Structure

```text
.
├── LICENSE
├── LICENSES/
├── CREDITS.md
├── CHANGELOG.md
├── README.md
├── docs/
│   ├── SERIAL_PROTOCOL_V1.md
│   └── FIRMWARE.md
├── firmware/
│   ├── Moebius_Polargraph/
│   ├── Moebius_Polargraph_Calibration/
│   ├── Moebius_Polargraph_Gallery/
│   └── Moebius_Polargraph_TwoStepperTest/
├── libraries/
└── parts/
```

| Path | Purpose |
| --- | --- |
| [`firmware/Moebius_Polargraph/`](firmware/Moebius_Polargraph/) | Serial-controlled firmware `0.1.0` for computer and future web-app control |
| [`firmware/Moebius_Polargraph_Calibration/`](firmware/Moebius_Polargraph_Calibration/) | Manual positioning, origin declaration, and four-shape calibration test |
| [`firmware/Moebius_Polargraph_Gallery/`](firmware/Moebius_Polargraph_Gallery/) | Onboard drawing demonstrations |
| [`firmware/Moebius_Polargraph_TwoStepperTest/`](firmware/Moebius_Polargraph_TwoStepperTest/) | Diagnostic test that both stepper motors and drivers move |
| [`libraries/`](libraries/) | Bundled Arduino dependencies with **their own licenses** |
| [`parts/`](parts/) | Third-party assembly reference PDF and a place for future fabrication files |
| [`docs/`](docs/) | Serial protocol and firmware documentation |

## Firmware selection

Upload **one** sketch at a time.

| Goal | Sketch |
| --- | --- |
| Test that both motors and ULN2003 drivers move | `Moebius_Polargraph_TwoStepperTest` |
| Calibrate, jog with `W` `A` `S` `D`, and run the four-shape test | `Moebius_Polargraph_Calibration` |
| Run onboard heart, butterfly, ellipse, and rectangle drawings | `Moebius_Polargraph_Gallery` |
| Serial control from a computer (protocol v1) | `Moebius_Polargraph` |

See [docs/FIRMWARE.md](docs/FIRMWARE.md) for upload details, first-start workflow, and troubleshooting.

## Quick start for serial firmware `0.1.0`

1. Open [`firmware/Moebius_Polargraph/Moebius_Polargraph.ino`](firmware/Moebius_Polargraph/Moebius_Polargraph.ino).
2. Compile for **Arduino Uno** (`arduino:avr:uno`).
3. Use the **bundled** [`libraries/TinyStepper_28BYJ_48`](libraries/TinyStepper_28BYJ_48/) copy (version `1.0.0` in this tree), not an untested Library Manager substitute.
4. Upload.
5. Open Serial Monitor at **`115200` baud**, 8 data bits, no parity, 1 stop bit, line ending **Newline**.
6. Confirm the banner: `boot machine=moebius-polargraph protocol=1 firmware=0.1.0 zero=0`.
7. Position the carriage with **small** nudges, starting at `20` steps:

```text
NUDGE UP 20
NUDGE DOWN 20
NUDGE LEFT 20
NUDGE RIGHT 20
```

Do **not** start with the maximum `200` steps.

8. Place the carriage at the geometric origin (centered between the cord exits, `520` mm below the anchors) and declare it:

```text
ZERO
```

9. Test Cartesian movement with the pen up:

```text
M5
JOG X 10
JOG X -10
```

10. Draw only with explicit pen commands. Send **one line at a time** and wait for `ok` or `error:*` before the next command.

Safe 30 mm square (after `ZERO`, workspace default `±100` mm):

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

Full command grammar: [docs/SERIAL_PROTOCOL_V1.md](docs/SERIAL_PROTOCOL_V1.md).

## Important behavior

- **Zero is manual.** The machine has no homing, limit switches, or encoders.
- **Zero is cleared after boot.** Cartesian `JOG`, `G0`, and `G1` are rejected until `ZERO`.
- A **new web client must send `ZERO CLEAR`** when connecting. Opening a serial port does not always reset the board.
- Raw **`NUDGE` invalidates Cartesian zero** and does not report millimetre XY.
- Cartesian **`JOG` requires zero**. It is relative millimetres, preserves zero, and **raises the pen** if it is down.
- **`G0` and `G1` do not move the pen.** The client must send pen commands.
- **`M3`** = pen down. **`M5`** = pen up. `PEN DOWN` / `PEN UP` are aliases.
- **`F` feedrate is not supported** in protocol v1 (`error:not-supported`).
- Commands are **blocking**. `ok` is sent only after motors and (for pen commands) the settle delay finish.
- Clients **must wait** for the terminal `ok` or `error:*` before sending another line.
- **Pause and abort cannot interrupt** a movement already in progress. The client pauses by not sending the next command. `ABORT` raises the pen between completed commands only.
- After lost steps, reset, slack, a crash, or any uncertain carriage movement, **reposition the machine and declare `ZERO` again**.

## Safety

Treat the machine as experimental hardware.

- Test directions with the **pen raised**.
- Keep cords tensioned and correctly wound on both spools.
- Keep reserve cord on both spools. Never allow a cord to reach its physical end, climb off a spool, or go slack.
- Begin with small movements (`NUDGE … 20`, few millimetres of `JOG` / `G0`).
- Stop power if a motor vibrates, moves the wrong way, or creates slack.
- Do not assume an accepted `CONFIG` value is mechanically safe. Protocol ranges are representational, not a mechanical warranty.
- Use a regulated supply that can power the motors, ULN2003 boards, servo, and controller together. Do not assume an Arduino USB port can supply the motors.
- Do **not** expand the default `±100` mm workspace before validating the physical machine.
- Disconnect power before changing wiring. Stay clear of the moving holder and cords.

## Limitations (protocol v1 / firmware `0.1.0`)

- No automatic homing.
- No limit switches or encoders.
- No EEPROM persistence (configuration returns to compiled defaults after reboot).
- No SD-card drawing.
- No calibrated Cartesian feedrate.
- No planner queue; one command at a time.
- No immediate mid-motion pause or abort.
- No recovery after power loss or serial disconnect.
- Conservative default workspace of `-100…+100` mm on both axes.
- Documentation of this specific mechanical build is still incomplete.
- The 2020 assembly PDF describes the upstream Wall Drawing Machine kit, not this validated build.

## Protocol and integration

The serial contract is defined in:

- [docs/SERIAL_PROTOCOL_V1.md](docs/SERIAL_PROTOCOL_V1.md)

It is intended to support later integration with the separate SchoolAI Plotter Workspace:

- <https://github.com/linalopes/SchoolAI-plotter-workspace>

That web-app integration is **planned work**. This repository does **not** claim that the Plotter Workspace already talks to this firmware.

## Other sketches

### `Moebius_Polargraph_TwoStepperTest`

[firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)

Diagnostic sketch. It drives both 28BYJ-48 motors through ULN2003 boards with AccelStepper in half-step mode. It does **not** use the pen-lift servo and does **not** calculate Polargraph geometry.

After reset, each motor travels back and forth between two opposite targets (`279` steps left, `673` steps right). The sketch prints a short banner at `9600` baud and then runs until the board is powered off or reset.

This sketch depends on AccelStepper (GPL V2 or Commercial). See [License](#license).

### `Moebius_Polargraph_Calibration`

[firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)

Serial manual controller and four-shape drawing test. It keeps Walldraw inverse kinematics, line subdivision, motor-direction constants, and Bresenham-style two-motor coordination.

On start it attaches the servo on `A0`, raises the pen to `90°`, and prints a command list at `115200` baud. It does **not** assume a drawing origin until you declare one.

Serial commands (Serial Monitor: `115200` baud, newline):

| Key | Action |
| --- | --- |
| `W` / `S` / `A` / `D` | Raw jog physically up / down / left / right (`20` steps per key). Raises the pen if it is down. Clears the logical origin. |
| `Z` | Treat the current physical position as logical `(0, 0)`. |
| `F` | Draw a circle, triangle, square, and diamond around that origin, then return to `(0, 0)`. Requires `Z` first. |
| `C` | Return to logical `(0, 0)` with the pen up. Requires `Z` first. |
| `U` / `N` | Pen-up (`90°`) / pen-down (`60°`) test. |
| `P` | Print logical XY, cord step counts, and whether origin is set. |
| `H` | Print the command list. |

The four-shape test draws:

- a circle of radius `15` mm at `(-60, +60)`;
- an equilateral triangle of side `30` mm at `(+60, +60)`;
- a square of side `30` mm at `(-60, -60)`;
- a diamond of diagonal `30` mm at `(+60, -60)`.

After the equal 2.30 m cords were installed, this test completed successfully on the machine.

This sketch does not read an SD card or home to endstops. Its single-key Serial Monitor dialect is **not** Moebius Serial Protocol v1.

### `Moebius_Polargraph_Gallery`

[firmware/Moebius_Polargraph_Gallery/Moebius_Polargraph_Gallery.ino](firmware/Moebius_Polargraph_Gallery/Moebius_Polargraph_Gallery.ino)

Standalone gallery firmware. It uses the same geometry, motor mapping, coordination, manual positioning, logical-zero workflow, and pen angles as the calibration sketch.

Normal workflow:

1. Position the carriage with `W`, `A`, `S`, and `D`.
2. Press `Z` to declare the physical position as logical `(0,0)`.
3. Select a drawing command.

Serial commands (Serial Monitor: `115200` baud, newline):

| Key | Action |
| --- | --- |
| `W` / `S` / `A` / `D` | Raw jog. Clears the logical origin. |
| `Z` | Declare the current physical position as logical `(0, 0)`. |
| `1` | Draw a heart at logical zero. |
| `2` | Draw a butterfly curve at logical zero. |
| `3` | Draw an ellipse at logical zero. |
| `4` | Draw a rotated rectangle at logical zero. |
| `G` | Draw all four as a compact 2 × 2 gallery. |
| `C` | Return to logical `(0, 0)`. |
| `U` / `N` | Pen-up / pen-down test. |
| `P` | Print logical state. |
| `H` | Print the command list. |

The heart and butterfly equations are adapted from the upstream Walldraw demo firmware. Lina Lopes did not author those original equations.

## Dependencies

### Bundled third-party libraries

Copied under [`libraries/`](libraries/):

| Library | Version in this tree | Used by |
| --- | --- | --- |
| [TinyStepper_28BYJ_48](libraries/TinyStepper_28BYJ_48/) | 1.0.0 | Serial firmware, calibration, gallery |
| [AccelStepper](libraries/AccelStepper/) | 1.58 | Two-stepper test only |
| [SD](libraries/SD/) | 1.2.3 | No current sketch |

See [CREDITS.md](CREDITS.md) and [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md). These libraries are **not** licensed by Lina Lopes.

### Arduino Library Manager / IDE libraries

- **AccelStepper** and **TinyStepper_28BYJ_48** can also be installed from Arduino IDE **Tools > Manage Libraries**, as noted in [`libraries/What's it.md`](libraries/What's%20it.md). Prefer the bundled TinyStepper copy for firmware `0.1.0`.
- **Servo** (`Servo.h`) is required by the serial, calibration, and gallery sketches and is **not** bundled here.

### Code written for Moebius Polargraph

The sketches under [`firmware/`](firmware/) are the Moebius Polargraph programs in this repository. The serial, calibration, and gallery sketches are derived from Walldraw (`WallDrawDemo.ino` kinematics, direction constants, and line subdivision). The gallery heart and butterfly curves come from that upstream demo. The two-stepper test follows the original Walldraw stepper diagnostic pattern and cites the same project.

## Setup

Mechanical assembly of this specific machine is **to be documented**.

The PDF in [`parts/`](parts/) is third-party Wall Drawing Machine 2020 installation material from the upstream project. It is **not** covered by the Moebius Polargraph licenses. Use it as historical kit documentation, not as a verified description of the current build.

Controller setup that is supported by files in this repository:

1. Install the Arduino IDE.
2. Install the libraries:
   - copy the bundled folders from [`libraries/`](libraries/) into your Arduino `libraries` directory, as described in [`libraries/What's it.md`](libraries/What's%20it.md), **or**
   - install AccelStepper and TinyStepper_28BYJ_48 from **Tools > Manage Libraries**, and ensure Servo is available.
3. Wire the two ULN2003 boards and the servo to the pins in [Hardware](#hardware). Disconnect power before changing wiring.
4. Open one sketch from [`firmware/`](firmware/) and upload it to the board.

## Motor Test

Use [`Moebius_Polargraph_TwoStepperTest`](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino) to check that both motors and both ULN2003 drivers move.

1. Disconnect power, then wire only the two stepper drivers (no servo is required).
2. Upload the sketch.
3. Open Serial Monitor at `9600` baud if you want the startup message.
4. Power the motors from a supply suitable for both drivers. Both motors should move continuously.
5. Stop the board if a cord reaches a physical limit, wraps the wrong way, or goes slack.

This test does not set an origin and does not produce a calibrated drawing path.

## Calibration

Use [`Moebius_Polargraph_Calibration`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino).

1. Upload the sketch.
2. Open Serial Monitor at **115200** baud, with line ending set to **Newline**.
3. Use `U` and `N` to confirm pen-up and pen-down.
4. With the pen up, use `W` `S` `A` `D` to jog the holder to the physical point you want as the drawing origin.
5. Press `Z`. The firmware then treats that point as logical `(0, 0)` and computes cord lengths from its IK model.
6. Press `P` to print the logical state.
7. Press `F` to draw the four test shapes, or `C` to return to the declared origin.

Declare `Z` at the geometric origin implied by `X_SEPARATION` and `LIMYMIN` / `ZERO_DEPTH`: centered between the cord exits, `520` millimeters below them.

Any `W` `S` `A` `D` jog clears the origin. You must press `Z` again before `F` or `C` will run.

Keep enough equal cord on both spools for the full drawing area. Short usable cord, not the IK model, was the cause of earlier distortion in the lower drawing area.

## Future SD Card Support

**Future work.** Current firmware cannot draw from an SD card. Do not add the SD library to an active sketch until the stages below are reached.

Planned stages:

1. Finish and freeze the calibrated drawing firmware.
2. Create an isolated SD-card diagnostic sketch that only initializes the card, lists files, and prints file contents without moving the motors.
3. Define a documented, restricted Moebius G-code subset.
4. Add a dry-run parser that validates coordinates and drawing bounds without motor movement.
5. Integrate SD playback with the existing manual-zero workflow.
6. Require `Z` / `ZERO` to be set before any file can run.
7. Add file listing, explicit file selection, pause, abort, progress, and error reporting.
8. Develop a computer-side SVG-to-Moebius-G-code converter.
9. Keep curve flattening, scaling, centering, and path optimization on the computer rather than on the Arduino Uno.

Planned hardware mapping:

| SD / servo signal | Arduino pin |
| --- | --- |
| SD `CS` | `4` |
| SD `MOSI` | `11` |
| SD `MISO` | `12` |
| SD `SCK` | `13` |
| Pen-lift servo | `A0` |

## Upstream Project

Firmware in this repository was developed from or informed by Walldraw:

- <https://github.com/shihaipeng03/Walldraw>

The serial, calibration, and gallery sketches preserve inverse kinematics, line subdivision, and coordinated stepping from `WallDrawDemo.ino`. The gallery heart and butterfly curves are adapted from that demo. The two-stepper test cites the same upstream project. Walldraw is a separate project. This repository does not claim to be the original Walldraw hardware or the complete upstream firmware set.

## Credits

Created and maintained by **Lina Lopes**.

Project adaptation, calibration, testing, physical validation, and documentation: **Lina Lopes**.

Moebius Polargraph is part of the Machines to Draw collection by Lina Lopes.

Upstream Walldraw: **shihaipeng03**.

Third-party library and manual credits are listed in [CREDITS.md](CREDITS.md) and [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md).

## License

Moebius Polargraph is an open-source project with a **scoped multi-license** structure. The complete repository is **not** uniformly MIT-licensed.

| Scope | License |
| --- | --- |
| Original firmware contributions and Walldraw-derived sketch code under [`firmware/`](firmware/) | [MIT License](LICENSES/MIT.txt). Copyright (c) 2021 shihaipeng03; Copyright (c) 2026 Lina Lopes |
| Original documentation by Lina Lopes (`README.md`, `CREDITS.md`, `CHANGELOG.md`, `docs/`, this license overview, future original manuals, original diagrams and photographs unless marked otherwise) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) — full text in [LICENSES/CC-BY-4.0.txt](LICENSES/CC-BY-4.0.txt) |
| Future original fabrication files by Lina Lopes | **Not applied yet.** Intended license: CERN-OHL-S-2.0. No such files are in this repository |
| Bundled libraries | Their own licenses; see [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md) |
| 2020 assembly PDF | Third-party reference material. **Excluded** from the MIT, CC BY 4.0, and future CERN-OHL-S-2.0 grants |

The root [LICENSE](LICENSE) file explains these scopes.

`Moebius_Polargraph_TwoStepperTest.ino` depends on AccelStepper. AccelStepper is licensed GPL V2 or Commercial and is not relicensed by the MIT firmware grant. Distributing a combined program built with AccelStepper is subject to AccelStepper’s terms.

This README is licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
