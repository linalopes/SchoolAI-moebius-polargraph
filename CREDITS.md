# Credits

This document records authorship found in this repository and in the upstream sources it cites. Usernames are left as usernames unless a fuller legal name appears in the files.

## Moebius Polargraph

Created and maintained by **Lina Lopes**.

Project adaptation, calibration, testing, and documentation: **Lina Lopes**.

Moebius Polargraph is part of the Machines to Draw collection by Lina Lopes.

The two sketches under [`firmware/`](firmware/) are the Moebius Polargraph programs in this tree.

Lina Lopes did not create the original Walldraw hardware or the full upstream firmware set.

No other contributors are recorded in this repository. There is no git history here to inspect.

## Upstream firmware and hardware sources

### Walldraw

- Project: Walldraw (also referred to in sketch comments as WallDraw / Wall Drawing Machine)
- GitHub user: **shihaipeng03**
- Repository: <https://github.com/shihaipeng03/Walldraw>
- Upstream software license: MIT License, Copyright (c) 2021 shihaipeng03 (upstream `LICENSE` file; not copied into this tree)

This repository cites Walldraw as the original wall-drawing project. The calibration sketch states that it copies motor-direction constants unchanged from `WallDrawDemo.ino` and that it preserves that sketch’s Cartesian inverse kinematics, line subdivision, and Bresenham-style two-motor coordination.

No fuller personal name for `shihaipeng03` is present in this repository.

## Third-party Arduino libraries

These libraries are bundled under [`libraries/`](libraries/) unless noted.

### AccelStepper

- Name: AccelStepper
- Version in this tree: 1.58
- Author: Mike McCauley `<mikem@airspayce.com>`
- Maintainer listed in `library.properties`: Patrick Wasp `<patrickwasp@gmail.com>`
- Homepage: <http://www.airspayce.com/mikem/arduino/AccelStepper/>
- License: GPL V2 or Commercial, as stated in [`libraries/AccelStepper/LICENSE`](libraries/AccelStepper/LICENSE) and the library headers. Copyright (C) 2008–2010 Mike McCauley (years as printed in those files).
- Used by: [`Moebius_Polargraph_TwoStepperTest.ino`](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)

The bundled README describes this copy as a fork reorganized for Arduino library conventions, following the upstream AccelStepper version.

### TinyStepper_28BYJ_48

- Name: TinyStepper_28BYJ_48
- Version in this tree: 1.0.0
- Author: S. Reifel
- Copyright: Copyright (c) 2017 S. Reifel & Co. (source headers); Copyright (c) 2018 Stanley Reifel & Co. / S. Reifel & Co. (license files)
- Homepage: <https://github.com/Stan-Reifel/TinyStepper_28BYJ_48>
- License: MIT License — [`libraries/TinyStepper_28BYJ_48/LICENSE.txt`](libraries/TinyStepper_28BYJ_48/LICENSE.txt)
- Used by: [`Moebius_Polargraph_Calibration.ino`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)

### SD

- Name: SD (Arduino SD library wrapping sdfatlib)
- Version in this tree: 1.2.3
- Authors listed in `library.properties`: Arduino, SparkFun
- Additional copyright in source and README: Copyright (C) 2008–2010 William Greiman (sdfatlib / SdFat sources); Copyright (c) 2010 SparkFun Electronics (SD wrapper)
- Homepage: <http://www.arduino.cc/en/Reference/SD>
- License: GNU General Public License v3 or later, because sdfatlib is licensed that way. See [`libraries/SD/README.adoc`](libraries/SD/README.adoc) and [`libraries/SD/src/README.txt`](libraries/SD/src/README.txt).
- Used by current Moebius Polargraph sketches: **no**. The library is bundled; pin `4` remains free for the original Walldraw SD interface.

### Servo (not bundled)

- Name: Servo
- Include: `Servo.h`
- Used by: [`Moebius_Polargraph_Calibration.ino`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)
- Source in this repository: none. The library is supplied with the Arduino IDE or Library Manager.
- Upstream Arduino Servo library credits (not present as files here): Copyright (c) 2009 Michael Margolis; Copyright (c) 2013 Arduino LLC; GNU Lesser General Public License v2.1 or later.

## Manuals and other third-party documents

### Wall Drawing Machine 2020 installation manual

- File: [`parts/Wall Drawing Machine 2020 Installation and debugging instructions.pdf`](parts/Wall%20Drawing%20Machine%202020%20Installation%20and%20debugging%20instructions.pdf)
- PDF metadata author: **Duke**
- PDF metadata creation date: 5 November 2020
- Title in the document: Wall Drawing Machine 2020 Installation and debugging instructions
- A file with a matching role exists in the upstream Walldraw repository (`Wall Drawing Machine 2020安装调试说明.pdf`).

No license or copyright statement was found inside this PDF. It is treated as third-party documentation. It is not claimed as original Moebius Polargraph writing.

### Bundled library documentation

TinyStepper includes `Documentation.md`, `Documentation.pdf`, and `HookupGuide.pdf`. AccelStepper includes HTML extras under `libraries/AccelStepper/extras/doc/`. Those files belong to their libraries and keep those libraries’ licenses.
