# Spec: Port Mural to the MKS DLC32 MAX V1.2

Status: ready-for-agent

## Problem Statement

I want to build a Mural wall-plotter, but the controller board I own is a **Makerbase MKS DLC32 MAX V1.2**, not the "TMC2209 Pen/Laser FluidNC" board that upstream Mural targets. The upstream firmware hard-codes that board's pins: steppers on GPIO 12/13/14/25/26, TMC2209 drivers configured over UART on GPIO 16/17, servo on GPIO 21, OLED on I2C 15/4, and an SPI SD card on 18/19/23/5. It also assumes a classic ESP32 with 4 MB of flash, while the DLC32 MAX has an ESP32-S3 with 8 MB. The printed enclosure is sized for the original 71.6 × 75.5 mm, low-profile board. The DLC32 MAX is 100 × 70 mm, with mounting holes 92 × 62 mm apart, and its drivers plug in as stepstick modules that stand much taller. I also use longer NEMA17 steppers instead of the pancake motors in the BOM. As it stands, I can neither flash Mural onto my board nor fit the board into the bot.

## Solution

Fork Mural and add a **second, selectable hardware target** for the DLC32 MAX, leaving the upstream target unchanged so upstream improvements keep merging cleanly.

- **Firmware:** a build target for the DLC32 MAX that compiles for the ESP32-S3 with 8 MB of flash and uses the board's pins:
  - steppers on the X and Y driver sockets
  - TMC2209 drivers in standalone mode, with microstepping set by DIP switches and current by Vref
  - servo on the laser TTL output
  - SD card on the onboard 4-bit SDMMC slot
  - OLED on a free header

  Drawing behaviour, kinematics, the web UI and the image pipeline stay identical.
- **Bring-up firmware:** a separate target that exercises each peripheral and reports over USB serial, so the hardware can be verified with minimal human effort before the full firmware is trusted.
- **Enclosure:** a reproducible, script-generated variant of the printed parts that changes only what's needed:
  - the bottom part gets new bosses on the board's 92 × 62 mm hole pattern
  - the cover is widened and made taller to clear the board and the stepstick drivers with heatsinks, with openings for the DC jack, USB, SD slot and motor connectors
  - motor clearance is checked for the longer steppers

  The output is print-ready STL/3MF files.
- **Windows build:** the build works on Windows, the human's machine, as well as in the upstream devcontainer.

## Seams (where it is tested)

1. **Build seam (highest, firmware):** both hardware targets compile from a clean checkout. This guards the "upstream untouched" promise and catches pin and API mistakes for the ESP32-S3.
2. **Board-profile seam (the one new code seam):** every board-specific fact lives in one board profile, selected at compile time: pins, driver configuration mode, storage backend, flash layout and bot mass. The rest of the firmware reads the profile and never names a GPIO.
3. **Storage seam:** the firmware talks to "the card" through one storage interface (filesystem, total bytes, used bytes, mount/format result). Its backend is SPI-SD on the upstream board and SDMMC on the DLC32 MAX.
4. **Hardware seam (human-observed, once):** the bring-up firmware prints a pass/fail report over serial: buzzer, each motor wiggle, enable polarity, servo sweep, SD mount and free space, OLED detect. The human plugs in the board and reports what moved or beeped.
5. **CAD seam:** a script regenerates the enclosure parts from the upstream STEP. An automated check intersects the parts against an envelope model of the DLC32 MAX, the stepstick drivers with heatsinks, the longer steppers and the connectors, and must find zero interference and positive clearance. The check also renders preview images for the human.

## User Stories

1. As a maker with a DLC32 MAX, I want to pick my board as a build target, so that I can flash Mural without editing source code.
2. As a maker, I want the upstream board target to keep building unchanged, so that I can still merge upstream fixes and features into my fork.
3. As a maker on Windows, I want the full build (worker, UI, firmware, filesystem image) to run on Windows, so that I don't need a Linux devcontainer.
4. As a maker, I want the firmware to target the ESP32-S3 with 8 MB of flash and a matching partition layout, so that the app and the web UI fit with room to spare.
5. As a maker, I want upload and serial monitoring to work through the board's CH340 USB port, so that flashing works with standard tooling.
6. As a maker, I want the left belt motor driven by the X socket and the right by the Y socket, so that the wiring maps onto the board's labelled connectors.
7. As a maker, I want the shared stepper-enable pin handled with the correct polarity, so that motors hold torque while drawing and release when idle.
8. As a maker using TMC2209 stepsticks, I want the firmware to skip UART driver configuration on this board, so that it works without soldering a UART bodge.
9. As a maker, I want clear documentation of the DIP-switch setting that gives 1/8 microstepping, so that the firmware's steps-per-rotation matches the hardware.
10. As a maker, I want a documented Vref value for the driver current, so that my longer steppers get adequate torque without overheating.
11. As a maker, I want stealthChop-quiet standalone operation, so that the bot is as quiet as upstream.
12. As a maker, I want the pen servo driven from the laser TTL output, so that I can use a labelled, PWM-capable connector.
13. As a maker, I want explicit wiring guidance that the servo's power comes from a 5V pin and never from the laser connector's VIN, so that I don't destroy the servo with 12–24 V.
14. As a maker, I want the firmware to compensate if the TTL output turns out to be inverted, so that the servo works whatever the board's output stage.
15. As a maker, I want the onboard TF slot used through 4-bit SDMMC, so that the drawing commands are stored on the card as upstream intends.
16. As a maker, I want card-missing and card-unformatted states reported on the display and serial like upstream, so that I know what to fix.
17. As a maker, I want the 0.91" OLED connected to a documented header on this board, so that I can see status and the IP address like upstream.
18. As a maker, I want the firmware to keep running headless if the OLED is missing or miswired, so that a display problem never blocks drawing.
19. As a maker, I want WiFi setup through the same captive portal as upstream, so that the onboarding experience is identical.
20. As a maker, I want the web UI and SVG pipeline unchanged, so that existing docs at getmural.me still apply.
21. As a maker with heavier, longer steppers, I want the bot mass used by the kinematic model set for my hardware, so that the belt-elongation compensation stays accurate.
22. As a maker, I want a bring-up firmware that beeps the buzzer, wiggles each motor and sweeps the servo in sequence, so that I can verify wiring by watching instead of debugging.
23. As a maker, I want the bring-up firmware to report SD and OLED detection over serial, so that every peripheral is verified before the first drawing.
24. As a maker, I want a short wiring guide with a connector-by-connector table (motors, servo, OLED, power, SD), so that assembly needs no guesswork.
25. As a maker, I want to power the board from the BOM's USB-C PD trigger through the DC barrel jack at 12 V or 20 V, so that I keep the upstream power setup.
26. As a maker, I want a printed bottom part with bosses on the DLC32 MAX's 92 × 62 mm hole pattern, sized for the BOM's M3 heat-set inserts, so that the board mounts firmly.
27. As a maker, I want a cover that clears the board, the stepstick drivers with heatsinks and all plugged connectors, so that the bot closes fully.
28. As a maker, I want openings in the enclosure for the DC jack, USB, TF card and motor/servo cables, so that I can power, flash and swap cards without disassembly.
29. As a maker, I want the motor carriers checked against my longer steppers, so that the motors fit without colliding with other parts.
30. As a maker, I want the modified parts as print-ready STL/3MF files next to the originals, so that I can slice and print them directly.
31. As a maker, I want the enclosure generated by a script from the upstream STEP, so that dimension corrections (for example, measured hole spacing) are a one-number change.
32. As a maker, I want preview renders of the modified parts, so that I can sanity-check them before spending filament.
33. As a maker, I want the shifted centre of mass from the heavier board and motors reflected in the kinematic constants, so that drawings aren't distorted.
34. As a maker, I want an automated build for both targets on every push, so that regressions are caught without my involvement.
35. As a future contributor, I want the board-specific facts documented in one place with their sources, so that supporting another board revision is straightforward.

## Implementation Decisions

- **Two targets, one codebase.** Add a DLC32 MAX build environment alongside the existing one. Board selection is a compile-time flag. The existing environment's behaviour, pins and flash layout stay byte-for-byte equivalent in intent.
- **Board profile module.** One module holds the board-specific facts for each target: stepper STEP/DIR pins per side, enable pin and polarity, driver-config mode (UART or standalone), servo pin and inversion, I2C pins, storage backend and pins, and hardware constants that vary with the build (bot mass). The movement, pen, display and storage modules read from it.
- **DLC32 MAX pin map.** Sources: the Makerbase "MKS DLC32 MAX V1.0_002 PIN" sheet, the grblHAL `mks_dlc32_max_1_0_map`, and the MKS DLC32 MAX Manual. These were written for V1.0, so the bring-up firmware verifies them on V1.2.
  - Left motor = X socket: STEP 16, DIR 15
  - Right motor = Y socket: STEP 7, DIR 6. The Y1 and Y2 connectors are in parallel; use Y1.
  - Shared enable: GPIO 8, assumed active-low and verified at bring-up.
  - Servo signal: laser TTL, GPIO 2, on the XH2.54-3P "laser 1" connector. Use only the S and G pins; V is VIN.
  - Servo 5V and GND: taken from any endstop header's 5V/GND pins.
  - SD: SDMMC 4-bit. CLK 13, CMD 14, D0 12, D1 11, D2 47, D3 21, card-detect 10.
  - OLED I2C: the first-choice candidate is UART1 GPIO 17/18, which go to the RJ11 display port and the expansion header. This is confirmed from a board photo or at bring-up. GPIO 35 is avoided because it is reportedly rewired to a driver DIAG signal on V1.2.
  - Buzzer: GPIO 42, used by the bring-up firmware.
- **Standalone drivers.** The DLC32 MAX target doesn't compile the TMC UART configuration. Upstream only uses UART to set current, microsteps and autoscale; homing doesn't use StallGuard, so no behaviour is lost.
  - Microstepping is 1/8, the upstream value, set with the socket DIP switches. For TMC2209, 1/8 is MS1 = MS2 = low.
  - Current is set by Vref. The upstream value of 707 mA RMS is about Vref 1.0 V on 0.11 Ω-sense modules, adjusted for the actual motors.
- **Storage interface.** Abstract "the card" behind one interface that provides the filesystem handle, capacity and usage, and the mount/format/missing result. The phase manager, SVG upload and runner use the interface instead of the SPI-SD global. The SPI implementation keeps upstream's mount, detect and format behaviour. The SDMMC implementation provides mount and missing detection; formatting on the device may degrade to "please format FAT32 on a PC" if SDMMC formatting is impractical.
- **Display robustness.** Display init must not hang or crash when no OLED answers on I2C. Status goes to serial instead.
- **Flash layout.** A separate 8 MB partition table for the DLC32 MAX target gives the app ample room and the rest to the web filesystem. The upstream 4 MB table is unchanged.
- **Cross-platform build script.** Replace the shell-specific `rm`/`cp` calls in the pre-build step with portable equivalents. Behaviour is unchanged on Linux.
- **Bring-up firmware.** A separate build environment with a self-contained test sequence and a serial report. It shares the board profile so that it tests the exact pins the real firmware uses.
- **CI.** A GitHub Actions workflow builds both firmware targets, plus the UI and worker, on push.
- **Enclosure generation.** A Python/CadQuery script reads the printed solids from the upstream STEP, which keeps exact B-rep geometry. It then:
  - removes the original controller standoffs from the bottom part
  - adds four boss standoffs on the 92 × 62 mm pattern for M3 heat-set inserts, with the board's 100 mm side along the bot's width, in the region the original controller occupied
  - regenerates the cover with enlarged width and height and with connector openings
  - exports STL/3MF
  
  The original 71.6 × 75.5 × 13.6 mm controller sat at the bot's centre; the motor carriers sit below that region.
- **Envelope model.** A simple solid model represents:
  - the DLC32 MAX: a 100 × 70 × 1.6 mm PCB with holes 4 mm in from the edges
  - four stepstick drivers with heatsinks, about 25 mm above the PCB
  - the DC jack, USB, RJ11, TF slot and XH connectors, with their plugged cables' keep-out
  - the longer stepper bodies
  
  It is used only for the interference check and renders.
- **Hardware constants.** Bot mass becomes a board-profile value. The DLC32 MAX value is set from the human's weighed or estimated bot mass. The centre-of-mass offset is re-estimated from the CAD assembly.

## Testing Decisions

- **Good tests check external behaviour:** the target compiles; the bring-up report says pass; the CAD check reports zero interference. They don't pin internal structure.
- **Firmware:** CI compiles both targets. That is the main automated gate. Where pure logic changes, host-side unit tests run through PlatformIO's native test environment. Examples are the storage free-space check before an upload and the board-profile consistency checks (no pin used twice, no use of strapping or flash pins). The repo has no existing tests, so this establishes the prior art.
- **Hardware:** the bring-up firmware is the acceptance test for pin mapping. Its serial output is pasted into the relevant ticket as evidence.
- **CAD:** the generation script's own check (boolean intersection volume = 0 and minimum clearance above a threshold) plus rendered PNGs attached to the ticket. The human prints only after the check passes.
- **End-to-end MVP acceptance:** the bot, built with the DLC32 MAX and the modified enclosure, draws a sample SVG from the web UI on a wall without distortion visibly worse than upstream.

## Out of Scope

- TMC2209 UART features (StallGuard homing, software current control) and the UART bodge.
- The MKS 3.5" touch screen and RJ11 display features beyond possibly reusing its UART1 pins.
- The A and Z axes, laser, spindle, air assist, flame/tilt/door safety inputs.
- Multi-colour drawing (an upstream roadmap item).
- Supporting the classic MKS DLC32 (V2.x, I2S shift-register steppers). It is a different board.
- Upstreaming the port to nikivanov/mural. Possible later, but not a goal.
- Redesigning the bot's mechanics beyond the enclosure and boss changes needed to fit the board and motors.

## Further Notes

- **Open hardware facts to confirm at bring-up:**
  - whether the V1.2 pin map matches V1.0
  - enable polarity
  - laser TTL polarity and voltage level
  - which header exposes GPIO 17/18
  - 5V rail capacity for the MG90S, which stalls at about 0.7 A
  - actual hole spacing and diameter (caliper)
- **Human inputs still needed:**
  - a photo of both sides of the V1.2 board
  - the stepper model, or its body length and mass
  - the flashing session and observations from the bring-up run
  - printing and assembly
- **Power:** the board's USB port does not power it. 12–24 V must be present on the DC barrel jack (DC-007B, 2.1 mm) even for flashing.
- **Remotes:** `origin` is the user's fork and `upstream` is nikivanov/mural. Keep DLC32-specific changes isolated so that `git merge upstream/main` stays easy.
