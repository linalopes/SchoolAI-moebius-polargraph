# Changelog

All notable changes to Moebius Polargraph are recorded here.

Original documentation by Lina Lopes, licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

## 0.1.0 — 2026-08-25

First experimental serial firmware release. Physically validated on Lina Lopes’s machine. Not a claim of universal Polargraph compatibility.

### Added

- Moebius Serial Protocol v1 (`docs/SERIAL_PROTOCOL_V1.md`).
- Arduino Uno serial firmware `0.1.0` (`firmware/Moebius_Polargraph/`).
- Fixed-size ASCII line parser (no Arduino `String`, no heap allocation, no command queue).
- Manual zero workflow (`ZERO`, `ZERO CLEAR`).
- Raw physical positioning (`NUDGE`) that invalidates Cartesian zero.
- Cartesian relative jog after zero (`JOG`), with workspace checks.
- Inverse-kinematics `G0` / `G1` in millimetres, streamed one blocking command at a time.
- Explicit pen control (`M3` / `M5`, `PEN DOWN` / `PEN UP`).
- Volatile RAM configuration (`CONFIG?` / `CONFIG`) with compiled defaults restored on reboot.
- Conservative software workspace `-100…+100` mm on both axes, with `Y_MAX < ZERO_DEPTH`.
- Successful physical validation on Lina Lopes’s machine (25 August 2026): banner, identification, pre-zero rejection, four `NUDGE` directions, `ZERO`, `JOG`, pen lift, `G21`/`G90`/`G0`/`G1`/`M3`/`M5`, a `30 × 30` mm square, and return to logical and physical `(0,0)`.

### Known limitations

- No homing, limit switches, or encoders.
- No EEPROM persistence.
- No SD-card drawing.
- No calibrated Cartesian feedrate (`F` is `error:not-supported`).
- No planner queue.
- No immediate mid-motion pause or abort.
- No recovery after power loss or serial disconnect.
- Default workspace remains the conservative `±100` mm envelope.
