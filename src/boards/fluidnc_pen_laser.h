#ifndef Board_FluidncPenLaser_h
#define Board_FluidncPenLaser_h

#include "boardprofile.h"

// Upstream Mural board: "TMC2209 Pen/Laser FluidNC CNC Controller" (classic ESP32, 4 MB).
// Values are exactly those hard-coded in upstream nikivanov/mural.
inline constexpr BoardProfile FLUIDNC_PEN_LASER = {
    /* name */ "FluidNC Pen/Laser TMC2209",
    /* chip */ Chip::Esp32,

    /* leftStep, leftDir, leftDirInverted */ 14, 12, true,
    /* rightStep, rightDir, rightDirInverted */ 25, 26, false,
    /* motorEnable, activeLow */ 13, true,

    /* driverConfig */ DriverConfig::Uart,
    /* tmcUartRx, tmcUartTx */ 16, 17,
    /* leftUartAddr, rightUartAddr */ 0, 1,

    /* servo, servoInverted */ 21, false,

    /* i2cSda, i2cScl */ 15, 4,

    /* storage */ StorageBackend::SpiSd,
    /* sdSck, sdMiso, sdMosi, sdCs */ 18, 19, 23, 5,
    /* sdClk, sdCmd, sdD0..sdD3 */ NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN,
    /* sdDetect, activeLow */ NO_PIN, true,

    /* buzzer */ NO_PIN,

    /* botMassKg */ 0.55,
    /* comBelowAxesMm */ 10.0,
};
BOARD_PROFILE_CHECKS(FLUIDNC_PEN_LASER);

#endif
