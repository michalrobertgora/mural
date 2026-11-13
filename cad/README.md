# Enclosure for the MKS DLC32 MAX

`enclosure.py` generates the printed parts that change for the DLC32 MAX build. It reads them from the upstream `STLs/Mural v2.step`, so every upstream feature is kept.

```sh
uv run cad/enclosure.py     # first run parses the 40 MB STEP (~2 min), then it's cached in cad/.cache
```

Outputs go to `cad/out/`:

| File | What |
|---|---|
| `bottom_dlc32max.stl` | Replaces upstream `bottom.stl`. The board bay is 30 mm longer, the old controller standoffs are gone, and it has 4 new M3 heat-set-insert bosses on the 92 × 62 mm pattern (same boss size as upstream) |
| `cover_dlc32max.stl` | Replaces upstream `cover.stl`. It's 30 mm longer to match, with a notch in the right wall for the DC barrel plug and USB |
| `dlc32max_envelope.stl` | The board envelope model used for the interference check (not for printing) |
| `preview_*.png` | Renders |
| `report.txt` | Dimensions, board hole positions, and the PASS/FAIL interference report |

All other printed parts (motor carriers, pen, belt loops, gear, screen holder, PD clamp, spacer) are unchanged. Print them from the upstream 3MF files.

## What it does

1. **Stretch.** Cut the bottom and cover at the featureless plane z = −25 mm and move everything above it up 30 mm: the top corner posts, the PD-trigger clamp, the top wall. The gap is filled with the extruded cross-section.
2. **Bottom.** Clear the old controller's standoffs and edge supports within the new board footprint, then add the new bosses.
3. **Cover.** Trim the ledge beside the motor connectors, then cut the DC/USB notch (open at the bottom edge, so the cover drops over plugged cables).
4. **Check.** Build an envelope of the board, the stepstick drivers with heatsinks, the connectors and cable keep-outs. Intersect it with the new parts and every other upstream component (servo, OLED and its wiring, screen holder, PD board). It must find no overlap and at least 0.5 mm clearance.

## Board placement

The board lies flat with its 100 mm side running up the wall:
- the motor-connector edge faces the left wall, and the DC/USB/SD/RJ11 edge faces the right wall;
- the drivers go in the **Y and Z sockets**. The X socket sits under the OLED's wiring, and the A socket uses the ESP32-S3 USB pins.

## Adjusting

Every dimension is a constant at the top of the script:
- `BOARD_X0` and `BOARD_Z0` set the board position;
- `STRETCH_DZ` sets how much longer the bay is;
- `ENVELOPE_BLOCKS` and `STEPSTICK` hold the component sizes;
- `POWER_USB_OPENING_*` sets the notch.

Component positions were read off Makerbase's dimension drawing (±1 mm). After measuring the real V1.2 board with calipers, correct them and re-run.
