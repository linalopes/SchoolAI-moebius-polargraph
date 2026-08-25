# Third-party material

This file lists third-party copyright and licenses found in this
repository. None of this material is licensed by Lina Lopes. None of it
is relicensed by the MIT or CC BY 4.0 grants in the root [LICENSE](../LICENSE) file.

## Walldraw

- Project: Walldraw (also named WallDraw / Wall Drawing Machine in source comments)
- Author: shihaipeng03
- URL: https://github.com/shihaipeng03/Walldraw
- License: MIT License, Copyright (c) 2021 shihaipeng03
- Role in this repository: upstream source for Polargraph inverse kinematics, motor-direction constants, line subdivision, Bresenham-style two-motor coordination, the two-stepper diagnostic pattern, and the heart and butterfly parametric equations used in the gallery sketch
- No fuller personal name for shihaipeng03 is present in this repository

Lina Lopes did not create the original Walldraw firmware or hardware.

## TinyStepper_28BYJ_48

- Path: [libraries/TinyStepper_28BYJ_48/](../libraries/TinyStepper_28BYJ_48/)
- Version in this tree: 1.0.0
- Author: S. Reifel
- Copyright: Copyright (c) 2017 S. Reifel & Co. (source headers); Copyright (c) 2018 Stanley Reifel & Co. / S. Reifel & Co. (license files)
- URL: https://github.com/Stan-Reifel/TinyStepper_28BYJ_48
- License: MIT License — [libraries/TinyStepper_28BYJ_48/LICENSE.txt](../libraries/TinyStepper_28BYJ_48/LICENSE.txt)
- Used by: Moebius_Polargraph_Calibration.ino, Moebius_Polargraph_Gallery.ino

## AccelStepper

- Path: [libraries/AccelStepper/](../libraries/AccelStepper/)
- Version in this tree: 1.58
- Author: Mike McCauley `<mikem@airspayce.com>`
- Maintainer listed in `library.properties`: Patrick Wasp `<patrickwasp@gmail.com>`
- URL: http://www.airspayce.com/mikem/arduino/AccelStepper/
- License: GPL V2 **or** Commercial, as stated in [libraries/AccelStepper/LICENSE](../libraries/AccelStepper/LICENSE) and the library headers. Copyright (C) 2008–2010 Mike McCauley (years as printed in those files).
- Used by: Moebius_Polargraph_TwoStepperTest.ino

AccelStepper is not MIT-licensed. Distributing a combined program that links AccelStepper, including a built copy of the two-stepper test, is subject to those GPL V2 or commercial terms unless a commercial AccelStepper license is obtained.

## Arduino SD library

- Path: [libraries/SD/](../libraries/SD/)
- Version in this tree: 1.2.3
- Authors listed in `library.properties`: Arduino, SparkFun
- Additional copyright in source and README: Copyright (C) 2008–2010 William Greiman (sdfatlib / SdFat sources); Copyright (c) 2010 SparkFun Electronics (SD wrapper)
- URL: http://www.arduino.cc/en/Reference/SD
- License: GNU GPL v3 or later, because sdfatlib is licensed that way. See [libraries/SD/README.adoc](../libraries/SD/README.adoc) and [libraries/SD/src/README.txt](../libraries/SD/src/README.txt).
- Used by current Moebius Polargraph sketches: **no**

This library is bundled but not `#include`d by the active sketches. Redistributing it remains subject to GPL-3.0-or-later.

## Servo (not bundled)

- Include: `Servo.h`
- Used by: Moebius_Polargraph_Calibration.ino, Moebius_Polargraph_Gallery.ino
- Source in this repository: none
- Upstream Arduino Servo library: Copyright (c) 2009 Michael Margolis; Copyright (c) 2013 Arduino LLC; GNU Lesser General Public License v2.1 or later

## Wall Drawing Machine 2020 installation manual

- Path: [parts/Wall Drawing Machine 2020 Installation and debugging instructions.pdf](../parts/Wall%20Drawing%20Machine%202020%20Installation%20and%20debugging%20instructions.pdf)
- PDF metadata author: Duke
- PDF metadata creation date: 5 November 2020
- Upstream source: https://github.com/shihaipeng03/Walldraw (`Wall Drawing Machine 2020安装调试说明.pdf`)
- License in this repository: none found

This PDF is third-party reference material. It is excluded from the Moebius Polargraph MIT, CC BY 4.0, and future CERN-OHL-S-2.0 licenses. It must not be treated as original documentation by Lina Lopes.

## Other third-party files

- [libraries/What's it.md](../libraries/What's%20it.md) — short Arduino library-install note with a generic Windows path. No author or license named in the file. Treat it as third-party kit documentation, not as original Moebius Polargraph writing.
- TinyStepper `Documentation.md`, `Documentation.pdf`, and `HookupGuide.pdf` belong to TinyStepper_28BYJ_48 and remain MIT with that library.
- AccelStepper HTML extras under `libraries/AccelStepper/extras/doc/` belong to AccelStepper and remain under that library’s GPL V2 or commercial terms.
