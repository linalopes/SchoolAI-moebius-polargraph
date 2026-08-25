# Moebius Polargraph

Moebius Polargraph is a suspended wall-drawing machine in the **Machines to Draw** collection by Lina Lopes. Two upper stepper motors change the lengths of two cords attached to a pen holder; a servo raises and lowers the pen. The firmware in this repository is adapted from the upstream Walldraw project and is used to test the motors, set a drawing origin, and run geometric drawing tests while the machine is calibrated.

Created and maintained by Lina Lopes.

## Project Status

This project is a **work in progress**. The machine is being calibrated and documented. It is not a finished consumer product.

Current firmware supports:

- a two-stepper diagnostic test with no pen-lift servo and no Polargraph geometry;
- manual cord jogging;
- declaring a logical origin;
- pen-up and pen-down tests;
- a four-shape drawing test around that origin.

G-code playback, SD-card drawing, and a complete measured calibration procedure for this specific machine are **not implemented** in the sketches collected here.

## How It Works

The two motors sit above the drawing surface. Each motor winds or unwinds a cord connected to the pen holder. Changing both cord lengths together moves the holder up or down. Changing them in opposite directions moves it left or right.

The calibration sketch converts a requested XY position into two cord lengths (inverse kinematics), then steps the motors together with Bresenham-style coordination so both cords reach the target together. A servo on analog pin `A0` lifts the pen between shapes.

Logical `+X` is physically right. Logical `+Y` is physically up toward the motor anchors.

## Hardware

The current machine uses:

- two **28BYJ-48** stepper motors;
- two **ULN2003** drivers;
- an Arduino-compatible controller;
- a servo to lift and lower the pen;
- two cords from the upper motors to a suspended pen holder.

The sketches do not name a specific Arduino board. Pin numbers match a typical Arduino Uno-style digital layout. Pin `4` is left unused so the original Walldraw SD-card wiring can remain in place.

Firmware pin map:


| Function                           | Pins                                                              |
| ---------------------------------- | ----------------------------------------------------------------- |
| Motor on Arduino pins 2, 3, 5, 6   | IN1–IN4 (calibration sketch: `m2`; two-stepper test: left motor)  |
| Motor on Arduino pins 7, 8, 9, 10  | IN1–IN4 (calibration sketch: `m1`; two-stepper test: right motor) |
| Pen-lift servo                     | `A0` (calibration sketch only)                                    |
| Reserved (original SD chip select) | `4`                                                               |


The 28BYJ-48 half-step wiring order used by the two-stepper test is IN1, IN3, IN2, IN4. The calibration sketch passes IN1, IN2, IN3, IN4 to `TinyStepper_28BYJ_48`.

These geometry values are **firmware constants**, not independently verified measurements of this physical machine:


| Parameter                                                       | Current value in the calibration sketch |
| --------------------------------------------------------------- | --------------------------------------- |
| Steps per motor turn                                            | `2048`                                  |
| Spool diameter                                                  | `35` mm                                 |
| Motor-anchor separation (`X_SEPARATION`)                        | `820` mm                                |
| Vertical offset of logical origin below the anchors (`LIMYMIN`) | `520` mm                                |
| Pen-up / pen-down servo angles                                  | `90°` / `60°`                           |


Updating those constants to match this machine is **work in progress**.

## Repository Structure

```text
.
├── CREDITS.md
├── LICENSES.md
├── README.md
├── firmware/
│   ├── Moebius_Polargraph_Calibration/
│   │   └── Moebius_Polargraph_Calibration.ino
│   └── Moebius_Polargraph_TwoStepperTest/
│       └── Moebius_Polargraph_TwoStepperTest.ino
├── libraries/
│   ├── AccelStepper/
│   ├── SD/
│   ├── TinyStepper_28BYJ_48/
│   └── What's it.md
└── parts/
    └── Wall Drawing Machine 2020 Installation and debugging instructions.pdf
```



## Firmware

There are two Arduino sketches. They are independent programs. Upload one at a time.

### `Moebius_Polargraph_TwoStepperTest`

[firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)

Diagnostic sketch. It drives both 28BYJ-48 motors through ULN2003 boards with AccelStepper in half-step mode. It does **not** use the pen-lift servo and does **not** calculate Polargraph geometry.

After reset, each motor travels back and forth between two opposite targets (`279` steps left, `673` steps right). The unequal distances produce a repeating motion pattern. The sketch prints a short banner at `9600` baud and then runs until the board is powered off or reset.

### `Moebius_Polargraph_Calibration`

[firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)

Serial manual controller and four-shape drawing test. It keeps Walldraw inverse kinematics, line subdivision, motor-direction constants, and Bresenham-style two-motor coordination.

On start it attaches the servo on `A0`, raises the pen to `90°`, and prints a command list at `115200` baud. It does **not** assume a drawing origin until you declare one.

Serial commands (Serial Monitor: `115200` baud, newline):


| Key                   | Action                                                                                                                     |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| `W` / `S` / `A` / `D` | Raw jog physically up / down / left / right (`20` steps per key). Raises the pen if it is down. Clears the logical origin. |
| `Z`                   | Treat the current physical position as logical `(0, 0)`.                                                                   |
| `F`                   | Draw a circle, triangle, square, and diamond around that origin, then return to `(0, 0)`. Requires `Z` first.              |
| `C`                   | Return to logical `(0, 0)` with the pen up. Requires `Z` first.                                                            |
| `U` / `N`             | Pen-up (`90°`) / pen-down (`60°`) test.                                                                                    |
| `P`                   | Print logical XY, cord step counts, and whether origin is set.                                                             |
| `H`                   | Print the command list.                                                                                                    |


The four-shape test draws:

- a circle of radius `15` mm at `(-60, +60)`;
- an equilateral triangle of side `30` mm at `(+60, +60)`;
- a square of side `30` mm at `(-60, -60)`;
- a diamond of diagonal `30` mm at `(+60, -60)`.

This sketch does not read an SD card, parse G-code, or home to endstops.

## Dependencies



### Bundled third-party libraries

Copied under `[libraries/](libraries/)`:


| Library                                                 | Version in this tree | Used by current sketches?                                                |
| ------------------------------------------------------- | -------------------- | ------------------------------------------------------------------------ |
| [AccelStepper](libraries/AccelStepper/)                 | 1.58                 | Yes — two-stepper test                                                   |
| [TinyStepper_28BYJ_48](libraries/TinyStepper_28BYJ_48/) | 1.0.0                | Yes — calibration sketch                                                 |
| [SD](libraries/SD/)                                     | 1.2.3                | No — bundled for the original Walldraw SD interface; pin `4` is reserved |


See [CREDITS.md](CREDITS.md) and [LICENSES.md](LICENSES.md) for authors and licenses. Do not relicense these libraries.

### Arduino Library Manager / IDE libraries

- **AccelStepper** and **TinyStepper_28BYJ_48** can also be installed from Arduino IDE **Tools > Manage Libraries**, as noted in the two-stepper test header and in `[libraries/What's it.md](libraries/What's%20it.md)`.
- **Servo** (`Servo.h`) is required by the calibration sketch and is **not** bundled here. Install it with the Arduino IDE or Library Manager.



### Code written for Moebius Polargraph

The two sketches under `[firmware/](firmware/)` are the Moebius Polargraph programs in this repository. The calibration sketch is derived from Walldraw (`WallDrawDemo.ino` kinematics, direction constants, and line subdivision). The two-stepper test is a diagnostic program for this machine; it cites the Walldraw repository as the original wall-drawing project.

## Setup

Mechanical assembly of this specific machine is **to be documented**.

The PDF in `[parts/](parts/)` is the 2020 Wall Drawing Machine installation and debugging manual from the upstream project. Use it as historical kit documentation, not as a verified description of the current Moebius Polargraph build.

Controller setup that is supported by files in this repository:

1. Install the Arduino IDE.
2. Install the libraries:
  - copy the bundled folders from `[libraries/](libraries/)` into your Arduino `libraries` directory, as described in `[libraries/What's it.md](libraries/What's%20it.md)`, **or**
  - install AccelStepper and TinyStepper_28BYJ_48 from **Tools > Manage Libraries**, and ensure Servo is available.
3. Wire the two ULN2003 boards and the servo to the pins in [Hardware](#hardware). Disconnect power before changing wiring.
4. Open one sketch from `[firmware/](firmware/)` and upload it to the board.

Confirm that the physical left/right motors match the sketch you uploaded. The two sketches name the same pin groups differently (`left`/`right` versus `m1`/`m2`).

## Motor Test

Use `[Moebius_Polargraph_TwoStepperTest](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)` to check that both motors and both ULN2003 drivers move.

1. Disconnect power, then wire only the two stepper drivers (no servo is required).
2. Upload the sketch.
3. Open Serial Monitor at `9600` baud if you want the startup message.
4. Power the motors from a supply suitable for both drivers. Both motors should move continuously.
5. Stop the board if a cord reaches a physical limit, wraps the wrong way, or goes slack.

This test does not set an origin and does not produce a calibrated drawing path.

## Calibration

Use `[Moebius_Polargraph_Calibration](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)`.

Supported by the current sketch:

1. Upload the sketch.
2. Open Serial Monitor at **115200** baud, with line ending set to **Newline**.
3. Use `U` and `N` to confirm pen-up and pen-down.
4. With the pen up, use `W` `S` `A` `D` to jog the holder to the physical point you want as the drawing origin.
5. Press `Z`. The firmware then treats that point as logical `(0, 0)` and computes cord lengths from its IK model.
6. Press `P` to print the logical state.
7. Press `F` to draw the four test shapes, or `C` to return to the declared origin.

**Work in progress:** matching the IK model to the real machine. For the shapes to be undistorted, the point you declare with `Z` should be the geometric origin implied by `X_SEPARATION` and `LIMYMIN` (centered between the cord exits, `LIMYMIN` millimeters below them), and those two constants should be the measured distances on this machine. Measuring, entering, and verifying those values is not finished in this repository.

Any `W` `S` `A` `D` jog clears the origin. You must press `Z` again before `F` or `C` will run.

## Power and Safety

This is not a complete electrical safety manual. Treat the machine as experimental hardware.

- Disconnect power before changing wiring.
- Check supply voltage and polarity before connecting motors, drivers, the servo, or a shield.
- Use a regulated supply that can power the motors, ULN2003 boards, servo, and controller together. Do not assume an Arduino USB port can supply the motors.
- Keep cords on the spools. Stop immediately if a cord reaches its physical limit, climbs off a spool, or becomes slack.
- Stay clear of the moving holder and cords while the motors are running.
- The two-stepper test runs until you cut power or reset the board.



## Known Limitations

- The project is unfinished and still being calibrated.
- There is no homing, no endstops, and no stored machine profile.
- Current sketches do not draw from SD cards or G-code, even though an SD library is bundled.
- Inverse-kinematics constants in the calibration sketch may not match this physical machine.
- The two-stepper test cannot validate pen lift or drawing geometry.
- Raw jogging has no software travel limits.
- The original 2020 assembly PDF describes the upstream Wall Drawing Machine kit. Differences from this machine are **to be documented**.



## Upstream Project

Firmware in this repository was developed from or informed by Walldraw:

- [https://github.com/shihaipeng03/Walldraw](https://github.com/shihaipeng03/Walldraw)

The calibration sketch copies motor-direction constants and preserves inverse kinematics, line subdivision, and coordinated stepping from `WallDrawDemo.ino`. The two-stepper test cites the same upstream project. Walldraw is a separate project; this repository does not claim to be the original Walldraw hardware or the complete upstream firmware set.

## Credits

Created and maintained by **Lina Lopes**.

Project adaptation, calibration, testing, and documentation: **Lina Lopes**.

Moebius Polargraph is part of the Machines to Draw collection by Lina Lopes.

Upstream Walldraw: **shihaipeng03**.

Third-party library and manual credits are listed in [CREDITS.md](CREDITS.md).

## License

This repository **does not** have a single project-wide license. Do not treat the tree as MIT-licensed as a whole.

- Upstream Walldraw software is published under the MIT License. See [LICENSES.md](LICENSES.md).
- Bundled libraries keep their own licenses (MIT, GPL-2.0-or-commercial, and GPL-3.0-or-later). Those licenses are not replaced by anything in this README.
- Original Moebius Polargraph documentation and any original firmware changes have **no license declared yet**. Publicly visible files without a license are not permission to copy, modify, or relicense them.
- The 2020 installation PDF has no license text in this repository.
