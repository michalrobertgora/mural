# 04 — DLC32 MAX target

**What to build:** A selectable DLC32 MAX build target that compiles and boots:
- ESP32-S3 with 8 MB of flash and its own partition table; CH340 serial on UART0
- left motor on the X socket (STEP 16 / DIR 15), right motor on the Y socket (STEP 7 / DIR 6)
- shared enable on GPIO 8, with the polarity taken from the profile
- TMC2209 drivers in standalone mode; no UART configuration is compiled
- servo on laser TTL GPIO 2, with optional inversion
- OLED on the profile's I2C pins (candidates 17/18); display init never hangs, and falls back to headless operation with status logged to serial

**Blocked by:** 02

**Status:** done

- [x] A new build environment compiles for the ESP32-S3 with an 8 MB partition table; the upstream environment is unchanged
- [x] A DLC32 MAX profile has the pins from the V1.0_002 pin sheet, with sources cited
- [x] TMC UART configuration is excluded on this target
- [x] A missing OLED does not block boot (headless fallback)
- [x] Both targets build green

## Comments

Done in c588ec6. Deviation: drivers in the **Y and Z sockets** (left = Y STEP7/DIR6, right = Z STEP5/DIR4), not X/Y - forced by the enclosure layout (ticket 09): the X socket sits under the OLED's wiring, A uses GPIO19/20 (ESP32-S3 USB). Builds: RAM 14.9%, Flash 35.9% of 3 MB app partition; LittleFS image builds.
