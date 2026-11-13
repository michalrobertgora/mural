# 09 — Enclosure generator

**What to build:** A reproducible CadQuery script that reads the printed solids from the upstream STEP and produces two modified parts:
- a bottom part with the original controller standoffs removed and four M3 heat-set-insert bosses on the 92 × 62 mm pattern
- a cover, widened and made taller, that clears the DLC32 MAX, the stepstick drivers with heatsinks (about 25 mm tall) and the plugged connectors, with openings for the DC jack, USB, TF slot and cables

The stepper body length is a parameter, because the motors are bigger than the pancake motors and the exact model isn't chosen yet. An envelope model drives an automated interference and clearance check and the preview renders. The parts are exported as STL/3MF next to the originals.

**Blocked by:** None — can start immediately

**Status:** ready-for-agent

- [ ] The script regenerates the parts from the upstream STEP, with key dimensions as parameters
- [ ] The interference check finds zero intersection volume and clearance above the threshold
- [ ] Preview renders are produced
- [ ] STL/3MF files are exported
