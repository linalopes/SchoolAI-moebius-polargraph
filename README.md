# Moebius Polargraph

Moebius Polargraph is a suspended wall-drawing machine in the **Machines to Draw** collection by Lina Lopes. Two upper stepper motors change the lengths of two cords attached to a pen holder; a servo raises and lowers the pen. The firmware in this repository is adapted from the upstream Walldraw project and is used to test the motors, set a drawing origin, and run geometric drawing tests.

Created and maintained by Lina Lopes.

This is an open-source project. It uses **multiple licenses**. The complete repository is not uniformly MIT-licensed. See [License](#license).

## Project Status

The machine is a working drawing rig and a **work in progress** for documentation and future SD-card support. It is not a finished consumer product.

The mechanical cord problem has been resolved. After installing two equal cords, the machine successfully drew a circle, a triangle, a square, and a diamond. Earlier lower-area geometric distortion was caused by insufficient usable cord length, not by the inverse-kinematics model.

Current firmware supports:

- a two-stepper diagnostic test with no pen-lift servo and no Polargraph geometry;
- manual cord jogging and declaring a logical origin;
- pen-up and pen-down tests;
- a four-shape drawing test (circle, triangle, square, diamond);
- a standalone gallery of heart, butterfly, ellipse, and rotated-rectangle drawings.

G-code playback and SD-card drawing are **not implemented**. They are planned in [Future SD Card Support](#future-sd-card-support).

## How It Works

The two motors sit above the drawing surface. Each motor winds or unwinds a cord connected to the pen holder. Changing both cord lengths together moves the holder up or down. Changing them in opposite directions moves it left or right.

The drawing sketches convert a requested XY position into two cord lengths (inverse kinematics), then step the motors together with Bresenham-style coordination so both cords reach the target together. A servo on analog pin `A0` lifts the pen between shapes.

Logical `+X` is physically right. Logical `+Y` is physically up toward the motor anchors.

## Hardware

The current machine uses:

- two **28BYJ-48** stepper motors;
- two **ULN2003** drivers;
- an Arduino-compatible controller;
- a servo to lift and lower the pen;
- two **35 mm** spools;
- two equal cords, approximately **2.30 meters** each, of **0.24 mm** round braided PE line.

The thicker 0.24 mm line did not create a significant scale problem.

The sketches do not name a specific Arduino board. Pin numbers match a typical Arduino Uno-style digital layout. Pin `4` is left unused so a future SD-card interface can use the original Walldraw chip-select wiring.

Firmware pin map:

| Function | Pins |
| --- | --- |
| Motor on Arduino pins 2, 3, 5, 6 | IN1–IN4 (calibration and gallery: `m2`; two-stepper test: left motor) |
| Motor on Arduino pins 7, 8, 9, 10 | IN1–IN4 (calibration and gallery: `m1`; two-stepper test: right motor) |
| Pen-lift servo | `A0` (calibration and gallery) |
| Reserved (future SD chip select) | `4` |

The 28BYJ-48 half-step wiring order used by the two-stepper test is IN1, IN3, IN2, IN4. The calibration and gallery sketches pass IN1, IN2, IN3, IN4 to `TinyStepper_28BYJ_48`.

These geometry values are **firmware constants**:

| Parameter | Current value in the drawing sketches |
| --- | --- |
| Steps per motor turn | `2048` |
| Spool diameter | `35` mm |
| Motor-anchor separation (`X_SEPARATION`) | `820` mm |
| Vertical offset of logical origin below the anchors (`LIMYMIN`) | `520` mm |
| Pen-up / pen-down servo angles | `90°` / `60°` |

## Repository Structure

```text
.
├── LICENSE
├── LICENSES/
│   ├── MIT.txt
│   ├── CC-BY-4.0.txt
│   └── THIRD-PARTY.md
├── CREDITS.md
├── README.md
├── firmware/
│   ├── Moebius_Polargraph_Calibration/
│   │   └── Moebius_Polargraph_Calibration.ino
│   ├── Moebius_Polargraph_Gallery/
│   │   └── Moebius_Polargraph_Gallery.ino
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

There are three Arduino sketches. They are independent programs. Upload one at a time.

### `Moebius_Polargraph_TwoStepperTest`

[firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)

Diagnostic sketch. It drives both 28BYJ-48 motors through ULN2003 boards with AccelStepper in half-step mode. It does **not** use the pen-lift servo and does **not** calculate Polargraph geometry.

After reset, each motor travels back and forth between two opposite targets (`279` steps left, `673` steps right). The unequal distances produce a repeating motion pattern. The sketch prints a short banner at `9600` baud and then runs until the board is powered off or reset.

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

This sketch does not read an SD card, parse G-code, or home to endstops.

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

| Library | Version in this tree | Used by current sketches? |
| --- | --- | --- |
| [AccelStepper](libraries/AccelStepper/) | 1.58 | Yes — two-stepper test only |
| [TinyStepper_28BYJ_48](libraries/TinyStepper_28BYJ_48/) | 1.0.0 | Yes — calibration and gallery |
| [SD](libraries/SD/) | 1.2.3 | No |

See [CREDITS.md](CREDITS.md) and [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md). These libraries are **not** licensed by Lina Lopes.

### Arduino Library Manager / IDE libraries

- **AccelStepper** and **TinyStepper_28BYJ_48** can also be installed from Arduino IDE **Tools > Manage Libraries**, as noted in [`libraries/What's it.md`](libraries/What's%20it.md).
- **Servo** (`Servo.h`) is required by the calibration and gallery sketches and is **not** bundled here.

### Code written for Moebius Polargraph

The sketches under [`firmware/`](firmware/) are the Moebius Polargraph programs in this repository. The calibration and gallery sketches are derived from Walldraw (`WallDrawDemo.ino` kinematics, direction constants, and line subdivision). The gallery heart and butterfly curves come from that upstream demo. The two-stepper test follows the original Walldraw stepper diagnostic pattern and cites the same project.

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

Confirm that the physical left/right motors match the sketch you uploaded. The sketches name the same pin groups differently (`left`/`right` versus `m1`/`m2`).

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

Declare `Z` at the geometric origin implied by `X_SEPARATION` and `LIMYMIN`: centered between the cord exits, `LIMYMIN` millimeters below them.

Any `W` `S` `A` `D` jog clears the origin. You must press `Z` again before `F` or `C` will run.

Keep enough equal cord on both spools for the full drawing area. Short usable cord, not the IK model, was the cause of earlier distortion in the lower drawing area.

## Power and Safety

This is not a complete electrical safety manual. Treat the machine as experimental hardware.

- Disconnect power before changing wiring.
- Check supply voltage and polarity before connecting motors, drivers, the servo, or a shield.
- Use a regulated supply that can power the motors, ULN2003 boards, servo, and controller together. Do not assume an Arduino USB port can supply the motors.
- Keep cords on the spools. Stop immediately if a cord reaches its physical limit, climbs off a spool, or becomes slack.
- Stay clear of the moving holder and cords while the motors are running.
- The two-stepper test runs until you cut power or reset the board.

## Known Limitations

- Documentation of this specific mechanical build is still incomplete.
- There is no homing, no endstops, and no stored machine profile.
- Current sketches do not draw from SD cards or G-code.
- The two-stepper test cannot validate pen lift or drawing geometry.
- Raw jogging has no software travel limits.
- The 2020 assembly PDF describes the upstream Wall Drawing Machine kit and is third-party material.

## Future SD Card Support

**Future work.** Current firmware cannot draw from an SD card. Do not add the SD library to an active sketch until the stages below are reached.

Planned stages:

1. Finish and freeze the calibrated drawing firmware.
2. Create an isolated SD-card diagnostic sketch that only initializes the card, lists files, and prints file contents without moving the motors.
3. Define a documented, restricted Moebius G-code subset.
4. Add a dry-run parser that validates coordinates and drawing bounds without motor movement.
5. Integrate SD playback with the existing manual-zero workflow.
6. Require `Z` to be set before any file can run.
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

The calibration and gallery sketches preserve inverse kinematics, line subdivision, and coordinated stepping from `WallDrawDemo.ino`. The gallery heart and butterfly curves are adapted from that demo. The two-stepper test cites the same upstream project. Walldraw is a separate project. This repository does not claim to be the original Walldraw hardware or the complete upstream firmware set.

## Credits

Created and maintained by **Lina Lopes**.

Project adaptation, calibration, testing, and documentation: **Lina Lopes**.

Moebius Polargraph is part of the Machines to Draw collection by Lina Lopes.

Upstream Walldraw: **shihaipeng03**.

Third-party library and manual credits are listed in [CREDITS.md](CREDITS.md) and [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md).

## License

Moebius Polargraph is an open-source project with a **scoped multi-license** structure. The complete repository is **not** uniformly MIT-licensed.

| Scope | License |
| --- | --- |
| Original firmware contributions and Walldraw-derived sketch code under [`firmware/`](firmware/) | [MIT License](LICENSES/MIT.txt). Copyright (c) 2021 shihaipeng03; Copyright (c) 2026 Lina Lopes |
| Original documentation by Lina Lopes (`README.md`, `CREDITS.md`, this license overview, future original manuals, original diagrams and photographs unless marked otherwise) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) — full text in [LICENSES/CC-BY-4.0.txt](LICENSES/CC-BY-4.0.txt) |
| Future original fabrication files by Lina Lopes | **Not applied yet.** Intended license: CERN-OHL-S-2.0. No such files are in this repository |
| Bundled libraries | Their own licenses; see [LICENSES/THIRD-PARTY.md](LICENSES/THIRD-PARTY.md) |
| 2020 assembly PDF | Third-party reference material. **Excluded** from the MIT, CC BY 4.0, and future CERN-OHL-S-2.0 grants |

The root [LICENSE](LICENSE) file explains these scopes.

`Moebius_Polargraph_TwoStepperTest.ino` depends on AccelStepper. AccelStepper is licensed GPL V2 or Commercial and is not relicensed by the MIT firmware grant. Distributing a combined program built with AccelStepper is subject to AccelStepper’s terms.

This README is licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
