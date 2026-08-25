# Licenses

This file is a **license inventory**, not a grant of rights for original Moebius Polargraph material. It does not relicense any third-party file.

This repository has **no project-wide license**. A single MIT (or other) license must not be applied to the whole tree.

## Original Moebius Polargraph material — license not declared

The following have **no license text** in this repository:

- [`firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino`](firmware/Moebius_Polargraph_Calibration/Moebius_Polargraph_Calibration.ino)
- [`firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino`](firmware/Moebius_Polargraph_TwoStepperTest/Moebius_Polargraph_TwoStepperTest.ino)
- [`README.md`](README.md)
- [`CREDITS.md`](CREDITS.md)
- this file
- [`libraries/What's it.md`](libraries/What's%20it.md)

**Consequence:** source that is only publicly visible is not permission to copy, modify, redistribute, or relicense it. Until Lina Lopes publishes a license for original contributions, those files should be treated as all rights reserved by default.

The calibration sketch is derived from Walldraw (`WallDrawDemo.ino`). Walldraw’s MIT License allows reuse of that upstream software if the MIT notice is preserved. That does **not** by itself license Lina Lopes’s original documentation or later original fabrication files, and it does not relicense bundled third-party libraries.

## Upstream Walldraw

- Project: <https://github.com/shihaipeng03/Walldraw>
- Upstream `LICENSE`: MIT License, Copyright (c) 2021 shihaipeng03
- That `LICENSE` file is **not** copied into this repository.

Walldraw being public on GitHub is not a substitute for keeping its MIT notice with any substantial Walldraw-derived code that is redistributed.

## Bundled third-party libraries

These licenses already exist in-tree. They remain in force. They are not replaced by a future Moebius Polargraph license.

| Path | License found in this tree |
| --- | --- |
| [`libraries/TinyStepper_28BYJ_48/`](libraries/TinyStepper_28BYJ_48/) | MIT License. Copyright (c) 2018 Stanley Reifel & Co. [`LICENSE.txt`](libraries/TinyStepper_28BYJ_48/LICENSE.txt) |
| [`libraries/AccelStepper/`](libraries/AccelStepper/) | GPL V2 **or** Commercial. Copyright (C) 2008 Mike McCauley. [`LICENSE`](libraries/AccelStepper/LICENSE) |
| [`libraries/SD/`](libraries/SD/) | GNU GPL v3 or later (wrapper and sdfatlib). Copyright (C) 2009 William Greiman; Copyright (c) 2010 SparkFun Electronics. [`README.adoc`](libraries/SD/README.adoc) |

Notes:

- AccelStepper’s GPL-2.0 option is copyleft. Distributing a combined program that links AccelStepper (including the two-stepper test as built with that library) is subject to those GPL terms unless a commercial AccelStepper license is obtained. AccelStepper cannot be relicensed as MIT.
- The SD library is GPL-3.0-or-later. It is bundled even though current sketches do not `#include` it. Redistributing that library remains subject to GPL-3.0-or-later.
- TinyStepper_28BYJ_48 is MIT and is compatible with MIT-licensed firmware that uses it.

## Servo library (not bundled)

The calibration sketch includes `Servo.h`. No Servo license file is in this repository. The Arduino Servo library is published separately under LGPL-2.1-or-later.

## Manuals and fabrication files

[`parts/Wall Drawing Machine 2020 Installation and debugging instructions.pdf`](parts/Wall%20Drawing%20Machine%202020%20Installation%20and%20debugging%20instructions.pdf) contains no license statement in this repository. PDF metadata names the author as Duke (2020). It must not be treated as MIT-licensed original Moebius Polargraph documentation.

There are no original fabrication files (CAD, STL, cut drawings) in this repository yet. When they are added, they should carry their own license, separate from firmware and from third-party manuals.

## Why there is no root `LICENSE` file

A root `LICENSE` would be read as covering the entire repository. That would be incorrect because:

1. original Moebius Polargraph files have no chosen license yet;
2. AccelStepper is GPL V2 or Commercial, not MIT;
3. the SD library is GPL-3.0-or-later;
4. the 2020 PDF has no license text here;
5. third-party code must not be relicensed.

## Decision needed from Lina Lopes

Please choose licenses **by scope**, rather than one license for every file:

1. **Original firmware contributions** (the two `.ino` files, after keeping Walldraw MIT notices on derived portions). MIT would be compatible with Walldraw and TinyStepper. It would **not** relicense AccelStepper or SD.
2. **Original documentation** written for Moebius Polargraph (`README.md`, `CREDITS.md`, later manuals). A documentation license such as CC BY 4.0, or the same MIT text, can be used; this needs an explicit choice.
3. **Future original fabrication files**, if any. Use a separate hardware license when those files exist.
4. **The 2020 PDF.** Either keep it as third-party unlicensed material, replace it with a link to upstream, or obtain permission from the rights holder. Do not relicense it.
5. **Whether to keep bundling AccelStepper and SD.** Keeping them is fine if their license files stay with them. Removing unused SD would reduce GPL-3.0 material in the tree; that is optional.

Until those choices are written down in license files that name the original work, this repository should not be described as a uniformly licensed or “open source” product.
