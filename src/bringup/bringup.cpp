// Hardware bring-up firmware for the active board profile (build env: *_bringup).
//
// Runs a fixed sequence of hardware tests and prints a numbered report over serial (115200 baud).
// Steps marked OBSERVE need a human to watch/listen; paste the whole serial log into the
// bring-up ticket. Afterwards a menu lets you re-run single steps.
//
// It deliberately uses raw GPIO / LEDC / SD_MMC / Wire instead of the Mural modules, so it tests
// the *pins in the board profile* and nothing else.

#include <Arduino.h>
#include <SD_MMC.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "../board.h"

namespace {

constexpr int STEPS_PER_REV = 200 * 8;  // firmware expects 1/8 microstepping (DIP MS1=MS2=low)
constexpr int SERVO_LEDC_CHANNEL = 4;
constexpr int SERVO_LEDC_BITS = 14;
constexpr int SERVO_FREQ = 50;

int stepNo = 0;

void header(const char *title) {
    stepNo++;
    Serial.printf("\n==== STEP %d: %s ====\n", stepNo, title);
}

void observe(const char *what) { Serial.printf("  OBSERVE: %s\n", what); }

void result(bool ok, const char *what) { Serial.printf("  %s: %s\n", ok ? "PASS" : "FAIL", what); }

void setEnable(bool enabled) {
    bool level = enabled ? !BOARD.motorEnableActiveLow : BOARD.motorEnableActiveLow;
    digitalWrite(BOARD.motorEnable, level ? HIGH : LOW);
}

void beep(int ms) {
    if (BOARD.buzzer == NO_PIN) return;
    digitalWrite(BOARD.buzzer, HIGH);
    delay(ms);
    digitalWrite(BOARD.buzzer, LOW);
}

void stepMotor(int stepPin, int dirPin, bool dirLevel, int steps, int periodUs) {
    digitalWrite(dirPin, dirLevel ? HIGH : LOW);
    delayMicroseconds(50);
    for (int i = 0; i < steps; i++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(periodUs - 5);
    }
}

// --------------------------------------------------------------------------------------------
void testProfile() {
    header("Board profile");
    Serial.printf("  board: %s\n", BOARD.name);
    Serial.printf("  left  STEP=%d DIR=%d   right STEP=%d DIR=%d   EN=%d (active %s)\n", BOARD.leftStep,
                  BOARD.leftDir, BOARD.rightStep, BOARD.rightDir, BOARD.motorEnable,
                  BOARD.motorEnableActiveLow ? "LOW" : "HIGH");
    Serial.printf("  servo=%d (inverted=%d)  I2C SDA=%d SCL=%d  buzzer=%d  SD detect=%d\n", BOARD.servo,
                  BOARD.servoInverted, BOARD.i2cSda, BOARD.i2cScl, BOARD.buzzer, BOARD.sdDetect);
    Serial.printf("  chip: %s, flash %u MB, free heap %u\n", ESP.getChipModel(),
                  (unsigned)(ESP.getFlashChipSize() / (1024 * 1024)), (unsigned)ESP.getFreeHeap());
    result(true, "profile printed");
}

void testBuzzer() {
    header("Buzzer");
    if (BOARD.buzzer == NO_PIN) {
        Serial.println("  SKIP: no buzzer in profile");
        return;
    }
    for (int i = 0; i < 3; i++) {
        beep(120);
        delay(150);
    }
    observe("three short beeps (only if a buzzer is plugged into BEEPER)");
}

void testEnable() {
    header("Stepper enable polarity");
    observe("try turning each motor shaft by hand during the next 8 s");
    setEnable(true);
    Serial.println("  4 s: motors should be ENABLED (shaft hard to turn, holds position)");
    delay(4000);
    setEnable(false);
    Serial.println("  4 s: motors should be DISABLED (shaft turns freely)");
    delay(4000);
    setEnable(true);
    observe("if it was the other way round, flip motorEnableActiveLow in the board profile");
}

void testMotor(const char *name, int stepPin, int dirPin, bool dirInverted) {
    char title[64];
    snprintf(title, sizeof(title), "%s motor", name);
    header(title);
    setEnable(true);
    Serial.printf("  1 rev 'extend' direction (DIR=%d), pause, 1 rev back\n", dirInverted ? 1 : 0);
    stepMotor(stepPin, dirPin, dirInverted, STEPS_PER_REV, 1250);
    delay(500);
    stepMotor(stepPin, dirPin, !dirInverted, STEPS_PER_REV, 1250);
    char what[160];
    snprintf(what, sizeof(what),
             "the %s motor turns exactly ONE full turn one way, then back (half a turn => DIP is 1/16;"
             " two turns => 1/4). Note which way it turned first.", name);
    observe(what);
}

void servoWrite(int angle, bool inverted) {
    // Same mapping as ESP32Servo defaults: 0..180 deg -> 544..2400 us
    int us = 544 + (2400 - 544) * angle / 180;
    uint32_t maxDuty = (1u << SERVO_LEDC_BITS) - 1;
    uint32_t duty = (uint32_t)((uint64_t)us * maxDuty / 20000);
    ledcWrite(SERVO_LEDC_CHANNEL, inverted ? maxDuty - duty : duty);
}

void servoSweep(bool inverted) {
    const int seq[] = {70, 90, 40, 90, 70};
    for (int a : seq) {
        servoWrite(a, inverted);
        delay(700);
    }
}

void testServo() {
    header("Pen servo");
    ledcSetup(SERVO_LEDC_CHANNEL, SERVO_FREQ, SERVO_LEDC_BITS);
    ledcAttachPin(BOARD.servo, SERVO_LEDC_CHANNEL);
    Serial.println("  A) normal signal: 70 -> 90 -> 40 -> 90 -> 70 deg");
    servoSweep(false);
    delay(800);
    Serial.println("  B) inverted signal: same sweep");
    servoSweep(true);
    servoWrite(70, BOARD.servoInverted);
    observe("which pass (A or B) made the servo sweep smoothly? B only => set servoInverted=true");
}

void testSd() {
    header("SD card");
    if (BOARD.sdDetect != NO_PIN) {
        pinMode(BOARD.sdDetect, INPUT_PULLUP);
        delay(2);
        int level = digitalRead(BOARD.sdDetect);
        Serial.printf("  card-detect GPIO%d reads %s (profile expects %s when a card is inserted)\n",
                      BOARD.sdDetect, level ? "HIGH" : "LOW", BOARD.sdDetectActiveLow ? "LOW" : "HIGH");
    }
    bool mounted = false;
    fs::FS *fs = nullptr;
    if (BOARD.storage == StorageBackend::SdMmc4Bit) {
        SD_MMC.setPins(BOARD.sdClk, BOARD.sdCmd, BOARD.sdD0, BOARD.sdD1, BOARD.sdD2, BOARD.sdD3);
        mounted = SD_MMC.begin("/sd", false, false);
        Serial.printf("  SDMMC 4-bit mount: %s\n", mounted ? "ok" : "failed");
        if (!mounted) {
            SD_MMC.end();
            mounted = SD_MMC.begin("/sd", true, false);
            Serial.printf("  SDMMC 1-bit mount: %s%s\n", mounted ? "ok" : "failed",
                          mounted ? "  (=> D1..D3 wiring differs from profile)" : "");
        }
        if (mounted) {
            Serial.printf("  card: %llu MB, type %d\n", SD_MMC.cardSize() / (1024 * 1024), SD_MMC.cardType());
            fs = &SD_MMC;
        }
    } else {
        SPI.begin(BOARD.sdSck, BOARD.sdMiso, BOARD.sdMosi, BOARD.sdCs);
        mounted = SD.begin(BOARD.sdCs, SPI, 20000000, "/sd");
        if (mounted) fs = &SD;
    }
    if (fs) {
        File f = fs->open("/bringup.txt", "w");
        bool wrote = f && f.print("mural bring-up\n") > 0;
        if (f) f.close();
        f = fs->open("/bringup.txt", "r");
        bool read = f && f.readString().startsWith("mural");
        if (f) f.close();
        fs->remove("/bringup.txt");
        result(wrote && read, "write + read back a test file");
    } else {
        result(false, "mount (card inserted? FAT32? 4-16 GB recommended)");
    }
}

bool scanI2c(int sda, int scl, uint8_t *found) {
    Wire.end();
    if (!Wire.begin(sda, scl, 100000)) return false;
    bool any = false;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("    device at 0x%02X\n", addr);
            if (!any) *found = addr;
            any = true;
        }
    }
    return any;
}

void testOled() {
    header("OLED (I2C scan)");
    if (BOARD.i2cSda == NO_PIN || BOARD.i2cScl == NO_PIN) {
        Serial.println("  SKIP: no I2C pins in profile (headless)");
        return;
    }
    const int pairs[][2] = {{BOARD.i2cSda, BOARD.i2cScl}, {BOARD.i2cScl, BOARD.i2cSda}};
    for (auto &p : pairs) {
        uint8_t addr = 0;
        Serial.printf("  scan SDA=%d SCL=%d\n", p[0], p[1]);
        if (scanI2c(p[0], p[1], &addr)) {
            bool swapped = p[0] != BOARD.i2cSda;
            result(!swapped, swapped ? "OLED found only with SDA/SCL swapped => swap i2cSda/i2cScl in profile"
                                     : "OLED found on profile pins");
            Adafruit_SSD1306 d(128, 32, &Wire, -1);
            if (d.begin(SSD1306_SWITCHCAPVCC, addr)) {
                d.clearDisplay();
                d.setTextColor(WHITE);
                d.setTextSize(1);
                d.setCursor(0, 0);
                d.print("Mural bring-up OK");
                d.setCursor(0, 16);
                d.print(BOARD.name);
                d.display();
                observe("OLED shows 'Mural bring-up OK'");
            }
            return;
        }
    }
    result(false, "no I2C device found (check OLED wiring: VCC 3.3V, GND, SDA, SCL)");
}

void runAll() {
    stepNo = 0;
    testProfile();
    testBuzzer();
    testEnable();
    testMotor("LEFT", BOARD.leftStep, BOARD.leftDir, BOARD.leftDirInverted);
    testMotor("RIGHT", BOARD.rightStep, BOARD.rightDir, BOARD.rightDirInverted);
    testServo();
    testSd();
    testOled();
    Serial.println("\n==== DONE. Paste this whole log into the bring-up ticket. ====");
}

void menu() {
    Serial.println("\nMenu: a=all  b=buzzer  e=enable  l=left motor  r=right motor  s=servo  d=SD  o=OLED");
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);
    pinMode(BOARD.leftStep, OUTPUT);
    pinMode(BOARD.leftDir, OUTPUT);
    pinMode(BOARD.rightStep, OUTPUT);
    pinMode(BOARD.rightDir, OUTPUT);
    pinMode(BOARD.motorEnable, OUTPUT);
    setEnable(false);
    if (BOARD.buzzer != NO_PIN) {
        pinMode(BOARD.buzzer, OUTPUT);
        digitalWrite(BOARD.buzzer, LOW);
    }
    Serial.println("\n\nMural hardware bring-up");
    runAll();
    menu();
}

void loop() {
    if (!Serial.available()) {
        delay(20);
        return;
    }
    char c = Serial.read();
    stepNo = 0;
    switch (c) {
        case 'a': runAll(); break;
        case 'b': testBuzzer(); break;
        case 'e': testEnable(); break;
        case 'l': testMotor("LEFT", BOARD.leftStep, BOARD.leftDir, BOARD.leftDirInverted); break;
        case 'r': testMotor("RIGHT", BOARD.rightStep, BOARD.rightDir, BOARD.rightDirInverted); break;
        case 's': testServo(); break;
        case 'd': testSd(); break;
        case 'o': testOled(); break;
        default: return;
    }
    menu();
}
