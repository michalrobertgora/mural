# 06 — Bring-up firmware

**What to build:** A separate build environment that uses the same DLC32 MAX board profile to run a self-contained hardware test. In order, it:
1. beeps the buzzer (GPIO 42)
2. toggles enable
3. wiggles the left motor, then the right, in both directions
4. sweeps the servo
5. mounts the SD card and reports its size
6. scans I2C for the OLED on the candidate pins

It prints a numbered PASS/OBSERVE report over serial, so the human only has to watch and paste the output.

**Blocked by:** 04

**Status:** done

- [x] The bring-up environment builds green
- [x] It shares the board profile, so it tests the exact pins the real firmware uses
- [x] The serial report lists each step with what the human should observe
- [x] It includes an I2C scan across candidate header pin pairs

## Comments

Done in c588ec6 (src/bringup/bringup.cpp, env mks_dlc32_max_bringup, 115200 baud). Steps: profile dump, buzzer, enable polarity, left/right motor 1 rev each way (detects wrong DIP microstepping), servo normal vs inverted signal, SD 4-bit then 1-bit mount + write/read, I2C scan on profile pins (and swapped). Menu to re-run single steps.
