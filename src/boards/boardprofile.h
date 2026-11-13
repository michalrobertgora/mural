#ifndef BoardProfile_h
#define BoardProfile_h

#include <stdint.h>

// Every hardware fact that differs between controller boards lives in a BoardProfile.
// Firmware modules read the active profile via board.h (BOARD) and never name a GPIO directly.

enum class Chip { Esp32, Esp32S3 };

// How the TMC2209 drivers get their current/microstep settings.
enum class DriverConfig {
    Uart,        // configured by firmware over a shared UART (upstream FluidNC Pen/Laser board)
    Standalone,  // configured in hardware: microsteps via MS1/MS2 (DIP), current via Vref pot
};

enum class StorageBackend { SpiSd, SdMmc4Bit };

constexpr int NO_PIN = -1;

struct BoardProfile {
    const char *name;
    Chip chip;

    // Steppers (AccelStepper DRIVER mode). Left/right as seen facing the wall.
    int leftStep, leftDir;
    bool leftDirInverted;
    int rightStep, rightDir;
    bool rightDirInverted;
    int motorEnable;              // shared enable for both drivers
    bool motorEnableActiveLow;

    DriverConfig driverConfig;
    int tmcUartRx, tmcUartTx;     // only for DriverConfig::Uart
    uint8_t leftUartAddr, rightUartAddr;

    // Pen-lift servo (50 Hz hobby servo).
    int servo;
    bool servoInverted;           // true if the board's output stage inverts the signal

    // SSD1306 OLED on I2C. NO_PIN on either means "no display header", run headless.
    int i2cSda, i2cScl;

    // SD card holding the drawing commands.
    StorageBackend storage;
    int sdSck, sdMiso, sdMosi, sdCs;                      // SpiSd
    int sdClk, sdCmd, sdD0, sdD1, sdD2, sdD3;             // SdMmc4Bit
    int sdDetect;                                         // NO_PIN if not wired
    bool sdDetectActiveLow;

    int buzzer;                   // NO_PIN if absent

    // Mechanics that depend on the build (controller + motors change the bot's weight).
    double botMassKg;
    // Distance from the line through both pulley axes down to the centre of mass, projected onto
    // the wall [mm]. Negative = centre of mass above the axes. (Kinematic model: d_m = this + q.)
    double comBelowAxesMm;
};

// ---- Compile-time consistency checks (run in every firmware build) ----
namespace boardcheck {

constexpr bool isForbidden(Chip chip, int pin) {
    if (pin == NO_PIN) return false;
    if (chip == Chip::Esp32) {
        // GPIO6-11: SPI flash. GPIO34-39: input only (checked separately for outputs).
        return (pin >= 6 && pin <= 11) || pin < 0 || pin > 39;
    }
    // ESP32-S3: GPIO26-32 SPI flash/PSRAM; GPIO22-25 don't exist; valid range 0..48.
    return (pin >= 22 && pin <= 32) || pin < 0 || pin > 48;
}

constexpr bool isInputOnly(Chip chip, int pin) {
    return chip == Chip::Esp32 && pin >= 34 && pin <= 39;
}

constexpr int MAX_PINS = 24;

struct PinList {
    int pins[MAX_PINS];
    bool output[MAX_PINS];
    int count;
};

constexpr PinList usedPins(const BoardProfile &b) {
    PinList l{};
    auto add = [&l](int pin, bool out) {
        if (pin != NO_PIN) { l.pins[l.count] = pin; l.output[l.count] = out; l.count++; }
    };
    add(b.leftStep, true); add(b.leftDir, true);
    add(b.rightStep, true); add(b.rightDir, true);
    add(b.motorEnable, true);
    if (b.driverConfig == DriverConfig::Uart) { add(b.tmcUartRx, false); add(b.tmcUartTx, true); }
    add(b.servo, true);
    add(b.i2cSda, true); add(b.i2cScl, true);
    if (b.storage == StorageBackend::SpiSd) {
        add(b.sdSck, true); add(b.sdMiso, false); add(b.sdMosi, true); add(b.sdCs, true);
    } else {
        add(b.sdClk, true); add(b.sdCmd, true);
        add(b.sdD0, true); add(b.sdD1, true); add(b.sdD2, true); add(b.sdD3, true);
    }
    add(b.sdDetect, false);
    add(b.buzzer, true);
    return l;
}

constexpr bool noDuplicatePins(const BoardProfile &b) {
    PinList l = usedPins(b);
    for (int i = 0; i < l.count; i++)
        for (int j = i + 1; j < l.count; j++)
            if (l.pins[i] == l.pins[j]) return false;
    return true;
}

constexpr bool noForbiddenPins(const BoardProfile &b) {
    PinList l = usedPins(b);
    for (int i = 0; i < l.count; i++) {
        if (isForbidden(b.chip, l.pins[i])) return false;
        if (l.output[i] && isInputOnly(b.chip, l.pins[i])) return false;
    }
    return true;
}

constexpr bool requiredPinsPresent(const BoardProfile &b) {
    return b.leftStep != NO_PIN && b.leftDir != NO_PIN && b.rightStep != NO_PIN &&
           b.rightDir != NO_PIN && b.motorEnable != NO_PIN && b.servo != NO_PIN &&
           b.botMassKg > 0;
}

}  // namespace boardcheck

#define BOARD_PROFILE_CHECKS(profile)                                                   \
    static_assert(boardcheck::noDuplicatePins(profile), "board profile: pin used twice"); \
    static_assert(boardcheck::noForbiddenPins(profile),                                  \
                  "board profile: flash/nonexistent pin, or output on input-only pin");  \
    static_assert(boardcheck::requiredPinsPresent(profile), "board profile: missing required pin/value")

#endif
