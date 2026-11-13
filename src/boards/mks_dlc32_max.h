#ifndef Board_MksDlc32Max_h
#define Board_MksDlc32Max_h

#include "boardprofile.h"

// Makerbase MKS DLC32 MAX (ESP32-S3, 8 MB flash, CH340 USB-serial on UART0 GPIO43/44).
//
// Pin sources (written for board V1.0; to be verified on V1.2 by the bring-up firmware):
//  - Makerbase "MKS DLC32 MAX V1.0_002 PIN.pdf"
//    https://github.com/makerbase-mks/MKS-DLC32/tree/main/MKS--DLC32-MAX-main/firmware
//  - grblHAL board map mks_dlc32_max_1_0_map.h
//    https://github.com/grblHAL/ESP32/blob/master/main/boards/mks_dlc32_max_1_0_map.h
//  - MKS DLC32 MAX Manual (power, connectors, dimensions)
//
// Wiring summary (see docs for details):
//  - Left motor  -> Y1-MOTOR connector, driver in Y socket (Y1/Y2 are paralleled)
//  - Right motor -> Z-MOTOR connector, driver in Z socket
//    (Y+Z, not X+Y: in the enclosure the X socket sits under the OLED wiring; A uses GPIO19/20 =
//    ESP32-S3 native USB pins. See cad/enclosure.py.)
//  - TMC2209 drivers run standalone: 1/8 microstepping via socket DIP (MS1=MS2=low), current via Vref
//  - Servo signal -> "Laser" XH2.54-3P connector, S and G pins only (V is 12-24V VIN, never use it!)
//    Servo +5V/GND -> 5V/GND pins of any endstop header
//  - OLED -> UART1 pins GPIO17/18 (candidate; exposed on the RJ11 screen port / expansion header)
//  - SD -> onboard TF slot, 4-bit SDMMC
//
// Pins NOT used on purpose: GPIO35 (spindle enable on V1.0, reportedly re-routed to a driver DIAG
// on V1.2), GPIO19/20 (A-axis, native USB pins), strapping pins 0/3/45/46.
inline constexpr BoardProfile MKS_DLC32_MAX = {
    /* name */ "MKS DLC32 MAX",
    /* chip */ Chip::Esp32S3,

    /* leftStep, leftDir, leftDirInverted */ 7, 6, true,       // Y socket
    /* rightStep, rightDir, rightDirInverted */ 5, 4, false,   // Z socket
    /* motorEnable, activeLow */ 8, true,                      // shared EN; polarity verify @bring-up

    /* driverConfig */ DriverConfig::Standalone,
    /* tmcUartRx, tmcUartTx */ NO_PIN, NO_PIN,
    /* leftUartAddr, rightUartAddr */ 0, 0,

    /* servo, servoInverted */ 2, false,                       // laser TTL; polarity verify @bring-up

    /* i2cSda, i2cScl */ 17, 18,                               // UART1 TX/RX pins; verify @bring-up

    /* storage */ StorageBackend::SdMmc4Bit,
    /* sdSck, sdMiso, sdMosi, sdCs */ NO_PIN, NO_PIN, NO_PIN, NO_PIN,
    /* sdClk, sdCmd, sdD0..sdD3 */ 13, 14, 12, 11, 47, 21,
    /* sdDetect, activeLow */ 10, true,                        // polarity verify @bring-up

    /* buzzer */ 42,

    // Estimates (see docs/dlc32-max.md, "Bot mass and balance"): upstream 0.55 kg, plus ~0.30 kg
    // for 40 mm-class NEMA17s instead of pancakes, ~0.04 kg for the bigger board + 2 stepsticks,
    // and ~0.02 kg of extra enclosure plastic. Refine by weighing the finished bot.
    /* botMassKg */ 0.90,
    // The bigger board and the 30 mm longer bay sit above the pulley axes, moving the centre of mass
    // up by ~12 mm vs upstream (10 mm below the axes) -> ~2 mm ABOVE the axes. Estimate.
    /* comBelowAxesMm */ -2.0,
};
BOARD_PROFILE_CHECKS(MKS_DLC32_MAX);

#endif
