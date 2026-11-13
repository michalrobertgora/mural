#ifndef Board_h
#define Board_h

// Selects the active board profile at compile time. Set the build flag in platformio.ini:
//   (none)                  -> upstream FluidNC Pen/Laser TMC2209 board
//   -D BOARD_MKS_DLC32_MAX  -> Makerbase MKS DLC32 MAX

#include "boards/fluidnc_pen_laser.h"

#if defined(BOARD_MKS_DLC32_MAX)
#include "boards/mks_dlc32_max.h"
inline constexpr const BoardProfile &BOARD = MKS_DLC32_MAX;
#else
inline constexpr const BoardProfile &BOARD = FLUIDNC_PEN_LASER;
#endif

#endif
