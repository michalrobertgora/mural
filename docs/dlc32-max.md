# Building Mural with the MKS DLC32 MAX

This fork adds support for the Makerbase **MKS DLC32 MAX** controller board (ESP32-S3, 8 MB flash), in place of the upstream "TMC2209 Pen/Laser FluidNC" controller. Everything else follows the upstream [getmural.me](https://getmural.me) docs and BOM.

> Status: the firmware builds; the pin map comes from Makerbase's V1.0 pin sheet. Items marked **(verify)** are confirmed on a V1.2 board with the bring-up firmware (see below).

## What's different from the upstream BOM

| Upstream | DLC32 MAX build |
|---|---|
| TMC2209 Pen/Laser FluidNC controller | MKS DLC32 MAX + **2× TMC2209 stepstick** drivers (V2.0-style, with heatsinks) |
| Pancake NEMA17 | Any NEMA17 works. Longer bodies stick out of the front, clear of the enclosure, but add weight (see *Bot mass*) |
| `bottom.stl`, `cover.stl` | `cad/out/bottom_dlc32max.stl`, `cad/out/cover_dlc32max.stl`. They are 35 mm longer, with new board bosses and a DC/USB opening. Regenerate with `uv run cad/enclosure.py` |
| — | 2.1 mm DC barrel plug with a short lead, right-angle preferred, from the PD trigger to the board |

## Board setup

### Drivers
Put the TMC2209 modules in the **Y** and **Z** sockets, oriented as printed on the board (EN pin to the "EN" silkscreen). **Never plug or unplug drivers or motors with power on.** The X and A sockets stay empty: in the enclosure, the X socket sits under the OLED's wiring.

### Microstepping DIP switches (both used sockets)
The firmware assumes **1/8 microstepping**, which on a TMC2209 is MS1 = MS2 = low. Set the socket's M0 (MS1) and M1 (MS2) switches to the position that ties them low, and leave M2 unused. The bring-up firmware checks this: one commanded revolution must be exactly one shaft turn.

### Driver current (Vref)
The TMC2209 runs standalone, so the firmware does not set current over UART. Turn the module's potentiometer while measuring between its centre and GND:
- Upstream runs pancake motors at about 0.7 A RMS. On common 0.11 Ω-sense modules, Irms ≈ 0.71 × Vref, so **Vref ≈ 1.0 V**.
- For bigger motors, aim for about 70 % of the motor's rated current: Vref ≈ 0.7 × I_rated / 0.71. Example: a 1.5 A motor needs Vref ≈ 1.5 V.

## Wiring

| What | Where on the DLC32 MAX | Notes |
|---|---|---|
| Left motor | **Y1-MOTOR** (Y socket) | Y1 and Y2 are in parallel; use Y1 |
| Right motor | **Z-MOTOR** (Z socket) | |
| Servo signal (orange/yellow) | **Laser 1** connector (XH2.54-3P), pin **S** | Laser TTL, GPIO 2. Don't connect pin **V**: that's 12–24 V and would destroy the servo |
| Servo GND (brown) | Laser 1 pin **G**, or any endstop G | |
| Servo +5V (red) | the **5V** pin of an unused endstop header (Probe or A-) | **(verify)** that the 5V rail handles the MG90S (≈0.7 A stall) |
| OLED SDA / SCL | GPIO 17 / GPIO 18 (UART1 pins: RJ11 screen port / expansion header) | **(verify)** which header exposes them on V1.2. Use soldered or right-angle leads: there's little room under the OLED |
| OLED VCC / GND | 3.3 V / GND | |
| Power | USB-C PD trigger (12 V or 20 V) → 2.1 mm barrel plug → **DC jack** | The board's USB port does **not** power the board, so 12–24 V must be present even for flashing |
| SD card | onboard TF slot | FAT32, 4–16 GB recommended. The firmware offers to format it if it can't be mounted |

## Flashing (Windows, macOS, Linux)

Prerequisites: [uv](https://docs.astral.sh/uv/), Node.js 20+, and the CH340 USB driver (Windows).

```sh
uv tool install platformio
pio run -e mks_dlc32_max_bringup -t upload -t monitor   # 1) hardware test, see below
pio run -e mks_dlc32_max -t upload                      # 2) Mural firmware
pio run -e mks_dlc32_max -t uploadfs                    # 3) web UI filesystem
```

If upload doesn't start, hold **BOOT**, tap **RESET**, release BOOT, and retry.

## Bring-up firmware

`mks_dlc32_max_bringup` tests every pin in the board profile and prints a numbered report at 115200 baud: buzzer, enable polarity, each motor turning exactly one revolution and back, servo (normal and inverted signal), SD mount with a write/read test, and an OLED I2C scan. Steps marked **OBSERVE** need you to watch or listen. Afterwards, a menu lets you re-run single steps. If anything differs, the fix is a value in `src/boards/mks_dlc32_max.h`.

## Bot mass and balance

The kinematic model uses two build-dependent values from the board profile (`src/boards/mks_dlc32_max.h`):

- `botMassKg` is used for belt-stretch compensation. The default is **0.90 kg**: upstream's 0.55 kg plus about 0.30 kg for 40 mm-class NEMA17s, 0.04 kg for the bigger board and drivers, and 0.02 kg of extra plastic. Weigh your finished bot, including the pen, and enter the real value.
- `comBelowAxesMm` is the distance from the line through both pulley axes down to the centre of mass. Upstream uses +10 mm. The board bay sits *above* the axes, and the heavier board plus the 30 mm longer bay move the centre of mass up by about 12 mm, so the default is **−2 mm**. This comes from CAD centroids and estimated part masses.

**Watch out:** a centre of mass close to, or above, the belt tangent points makes the bot top-heavy, so it tilts more under pen drag. If drawings show tilt-related distortion:

1. Add a counterweight low in the bottom part, below the pulleys. For example, 50–100 g of steel or lead tape in the lower corners.
2. Re-estimate the value. Hang the bot from a single pin at the pen position and see whether the top swings forward or back.
3. Adjust `comBelowAxesMm` to match.

Heavier motors don't change the balance much: they sit on the pulley axis.
