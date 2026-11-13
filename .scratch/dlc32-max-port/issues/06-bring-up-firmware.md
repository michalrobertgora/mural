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

**Status:** ready-for-agent

- [ ] The bring-up environment builds green
- [ ] It shares the board profile, so it tests the exact pins the real firmware uses
- [ ] The serial report lists each step with what the human should observe
- [ ] It includes an I2C scan across candidate header pin pairs
