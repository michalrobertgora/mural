#ifndef Storage_h
#define Storage_h
#include <FS.h>
#include "display.h"

// "The card": the SD card that stores the uploaded drawing commands.
// The backend (SPI SD or 4-bit SDMMC) and its pins come from the board profile (board.h).
namespace storage {

// Mounts the card, blocking on the boot screen until it succeeds. Halts forever (screen shows
// "NO CARD") if no card is detected, or if it still can't be formatted/mounted after the
// on-screen countdown.
void initOrHalt(Display *display);

fs::FS &fs();
uint64_t totalBytes();
uint64_t usedBytes();

}  // namespace storage

#endif
