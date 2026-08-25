# Credits

This document records authorship found in this repository and in the upstream sources it cites. Usernames are left as usernames unless a fuller legal name appears in the files.

This document is original Moebius Polargraph documentation by Lina Lopes and is licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Third-party names and license identifiers below are factual credits, not a relicensing of those works.

## Moebius Polargraph

Created and maintained by **Lina Lopes**.

Project adaptation, calibration, testing, and documentation: **Lina Lopes**.

Moebius Polargraph is part of the Machines to Draw collection by Lina Lopes.

The sketches under [`firmware/`](firmware/) are the Moebius Polargraph programs in this tree.

Lina Lopes did not create the original Walldraw hardware or the full upstream firmware set.

## Upstream firmware and hardware sources

### Walldraw

- Project: Walldraw (also referred to in sketch comments as WallDraw / Wall Drawing Machine)
- GitHub user: **shihaipeng03**
- Repository: <https://github.com/shihaipeng03/Walldraw>
- Upstream software license: MIT License, Copyright (c) 2021 shihaipeng03

This repository cites Walldraw as the original wall-drawing project. The calibration and gallery sketches copy motor-direction constants from `WallDrawDemo.ino` and preserve that sketch’s Cartesian inverse kinematics, line subdivision, and Bresenham-style two-motor coordination.

The gallery heart and butterfly parametric equations are adapted from the upstream Walldraw demo firmware. Lina Lopes did not author those original equations.

No fuller personal name for `shihaipeng03` is present in this repository.

## Third-party Arduino libraries

These libraries are bundled under [`libraries/`](libraries/) unless noted. They are **not** licensed by Lina Lopes.

### AccelStepper

- Name: AccelStepper
- Version in this tree: 1.58
- Author: Mike McCauley `<mikem@airspayce.com>`
- Maintainer listed in `library.properties`: Patrick Wasp `<patrickwasp@gmail.com>`
- Homepage: <http://www.airspayce.com/mikem/arduino/AccelStepper/>
- License: GPL V2 or Commercial, as stated in [`libraries/AccelStepper/LICENSE`](libraries/AccelStepper/LICENSE) and the library headers. Copyright (C) 2008–2010 Mike McCauley (years as printed in those files).
- Used by: [`Moebius_Polargraph_TwoStepperTest.ino`](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)

The bundled README describes this copy as a fork reorganized for Arduino library conventions, following the upstream AccelStepper version.

Distributing a combined program that links AccelStepper is subject to AccelStepper’s GPL V2 or commercial terms.

### TinyStepper_28BYJ_48

- Name: TinyStepper_28BYJ_48
- Version in this tree: 1.0.0
- Author: S. Reifel
- Copyright: Copyright (c) 2017 S. Reifel & Co. (source headers); Copyright (c) 2018 Stanley Reifel & Co. / S. Reifel & Co. (license files)
- Homepage: <https://github.com/Stan-Reifel/TinyStepper_28BYJ_48>
- License: MIT License — [`libraries/TinyStepper_28BYJ_48/LICENSE.txt`](libraries/TinyStepper_28BYJ_48/LICENSE.txt)
- Used by: [`Moebius_Polargraph_Calibration.ino`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino), [`Moebius_Polargraph_Gallery.ino`](firmware/Moebius_Polargraph_Gallery/Moebius_Polargraph_Gallery.ino)

### SD

- Name: SD (Arduino SD library wrapping sdfatlib)
- Version in this tree: 1.2.3
- Authors listed in `library.properties`: Arduino, SparkFun
- Additional copyright in source and README: Copyright (C) 2008–2010 William Greiman (sdfatlib / SdFat sources); Copyright (c) 2010 SparkFun Electronics (SD wrapper)
- Homepage: <http://www.arduino.cc/en/Reference/SD>
- License: GNU General Public License v3 or later, because sdfatlib is licensed that way. See [`libraries/SD/README.adoc`](libraries/SD/README.adoc) and [`libraries/SD/src/README.txt`](libraries/SD/src/README.txt).
- Used by current Moebius Polargraph sketches: **no**

### Servo (not bundled)

- Name: Servo
- Include: `Servo.h`
- Used by: [`Moebius_Polargraph_Calibration.ino`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino), [`Moebius_Polargraph_Gallery.ino`](firmware/Moebius_Polargraph_Gallery/Moebius_Polargraph_Gallery.ino)
- Source in this repository: none. The library is supplied with the Arduino IDE or Library Manager.
- Upstream Arduino Servo library credits (not present as files here): Copyright (c) 2009 Michael Margolis; Copyright (c) 2013 Arduino LLC; GNU Lesser General Public License v2.1 or later.

## Manuals and other third-party documents

### Wall Drawing Machine 2020 installation manual

- File: [`parts/Wall Drawing Machine 2020 Installation and debugging instructions.pdf`](parts/Wall%20Drawing%20Machine%202020%20Installation%20and%20debugging%20instructions.pdf)
- PDF metadata author: **Duke**
- PDF metadata creation date: 5 November 2020
- Title in the document: Wall Drawing Machine 2020 Installation and debugging instructions
- Upstream source: <https://github.com/shihaipeng03/Walldraw> (`Wall Drawing Machine 2020安装调试说明.pdf`)

This PDF is third-party reference material. No license statement was found in the file. It is **excluded** from the Moebius Polargraph MIT, CC BY 4.0, and future CERN-OHL-S-2.0 licenses. It is not original documentation by Lina Lopes.

### Bundled library documentation

TinyStepper includes `Documentation.md`, `Documentation.pdf`, and `HookupGuide.pdf`. AccelStepper includes HTML extras under `libraries/AccelStepper/extras/doc/`. Those files belong to their libraries and keep those libraries’ licenses.

[`libraries/What's it.md`](libraries/What's%20it.md) is a short note on copying bundled libraries into the Arduino libraries folder. It does not name an author and is treated as third-party kit documentation.

## Contributors

None besides Lina Lopes are verifiable from this repository as project maintainers.
