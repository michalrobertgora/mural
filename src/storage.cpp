#include "storage.h"
#include <SPI.h>
#include <SD.h>
#include <SD_MMC.h>
#include <sd_diskio.h>
#include "board.h"

namespace {
constexpr const char *MOUNT_POINT = "/sd";
constexpr uint32_t SPI_SD_FREQUENCY = 20000000;
constexpr int FORMAT_COUNTDOWN_SECONDS = 10;
constexpr bool IS_SPI = BOARD.storage == StorageBackend::SpiSd;

void haltWithNoCard(Display *display) {
    Serial.println("SD card: none detected (or unusable), halting");
    display->showNoCard();
    while (true) {
        delay(1000);
    }
}

void formatCountdown(Display *display) {
    for (int secondsLeft = FORMAT_COUNTDOWN_SECONDS; secondsLeft > 0; secondsLeft--) {
        display->showFormatCountdown(secondsLeft);
        delay(1000);
    }
    display->showFormatting();
}

// ---- SPI SD (upstream board) ----
bool mountSpi(bool formatIfNeeded) {
    return SD.begin(BOARD.sdCs, SPI, SPI_SD_FREQUENCY, MOUNT_POINT, 5, formatIfNeeded);
}

void initSpiOrHalt(Display *display) {
    SPI.begin(BOARD.sdSck, BOARD.sdMiso, BOARD.sdMosi, BOARD.sdCs);

    if (mountSpi(false)) {
        return;
    }

    // SD.begin() doesn't say whether the card is missing or just needs
    // formatting, since it resets its driver state on any mount failure.
    // Probe the hardware directly to tell those two cases apart.
    uint8_t pdrv = sdcard_init(BOARD.sdCs, &SPI, SPI_SD_FREQUENCY);
    if (pdrv == 0xFF) {
        haltWithNoCard(display);
    }
    sdcard_uninit(pdrv);

    formatCountdown(display);
    if (!mountSpi(true)) {
        haltWithNoCard(display);
    }
}

// ---- 4-bit SDMMC (MKS DLC32 MAX onboard TF slot) ----
bool cardDetected() {
    if (BOARD.sdDetect == NO_PIN) {
        return true;  // can't tell; treat a failed mount as "needs formatting"
    }
    pinMode(BOARD.sdDetect, INPUT_PULLUP);
    delay(2);
    return digitalRead(BOARD.sdDetect) == (BOARD.sdDetectActiveLow ? LOW : HIGH);
}

bool mountSdMmc(bool formatIfNeeded) {
    SD_MMC.end();
    if (!SD_MMC.setPins(BOARD.sdClk, BOARD.sdCmd, BOARD.sdD0, BOARD.sdD1, BOARD.sdD2, BOARD.sdD3)) {
        Serial.println("SD card: SDMMC pin assignment rejected");
        return false;
    }
    return SD_MMC.begin(MOUNT_POINT, false /* 4-bit */, formatIfNeeded);
}

void initSdMmcOrHalt(Display *display) {
    if (!cardDetected()) {
        haltWithNoCard(display);
    }
    if (mountSdMmc(false)) {
        return;
    }
    Serial.println("SD card: present but not mountable, will format");
    formatCountdown(display);
    if (!mountSdMmc(true)) {
        haltWithNoCard(display);
    }
}
}  // namespace

namespace storage {

void initOrHalt(Display *display) {
    if constexpr (IS_SPI) {
        initSpiOrHalt(display);
    } else {
        initSdMmcOrHalt(display);
    }
    Serial.printf("SD card: mounted, %llu bytes total, %llu used\n", totalBytes(), usedBytes());
}

fs::FS &fs() {
    if constexpr (IS_SPI) {
        return SD;
    } else {
        return SD_MMC;
    }
}

uint64_t totalBytes() {
    if constexpr (IS_SPI) {
        return SD.totalBytes();
    } else {
        return SD_MMC.totalBytes();
    }
}

uint64_t usedBytes() {
    if constexpr (IS_SPI) {
        return SD.usedBytes();
    } else {
        return SD_MMC.usedBytes();
    }
}

}  // namespace storage
