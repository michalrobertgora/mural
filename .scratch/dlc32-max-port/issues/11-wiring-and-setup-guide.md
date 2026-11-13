# 11 — Wiring and setup guide

**What to build:** A short guide to building Mural with the DLC32 MAX, covering:
- a connector-by-connector table:
  - motors go to the X and Y1 connectors
  - the servo signal goes to the laser TTL connector's S and G pins
  - the servo's 5 V comes from an endstop header, never from the laser connector's VIN
  - which header the OLED uses
  - power comes from the USB-C PD trigger at 12 or 20 V into the DC barrel jack
- DIP settings for 1/8 microstepping on TMC2209
- the Vref value for the chosen steppers
- flashing steps on Windows

**Blocked by:** 08

**Status:** ready-for-agent

- [ ] The guide reflects the V1.2 facts verified at bring-up
- [ ] The flashing instructions have been tested on Windows
