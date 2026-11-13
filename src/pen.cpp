#include "pen.h"

bool shouldStop(int currentDegree, int targetDegree, bool positive) {
    if (positive) {
        return currentDegree > targetDegree;
    } else {
        return currentDegree < targetDegree;
    }
}

void doSlowMove(Pen* pen, int startDegree, int targetDegree, int speedDegPerSec) {
    if (startDegree == targetDegree) {
        return;
    }

    auto startTime = millis();

    bool positive;
    if (targetDegree > startDegree) {
        positive = true;
    } else {
        positive = false;
    }

    auto currentDegree = startDegree;

    while (!(shouldStop(currentDegree, targetDegree, positive))) {
        pen->setRawValue(currentDegree);
        delay(10);

        auto currentTime = millis();
        auto deltaTime = currentTime - startTime;
        auto progressDegrees = int(double(deltaTime) / 1000 * speedDegPerSec);

        if (!positive) {
            progressDegrees = progressDegrees * -1;
        }

        currentDegree = startDegree + progressDegrees;
    }
    pen->setRawValue(targetDegree);
    delay(200);
}


namespace {
// Used only when the board's servo output stage inverts the signal (BOARD.servoInverted):
// drive LEDC directly and emit the complementary duty so the servo sees normal pulses.
constexpr int INVERTED_SERVO_LEDC_CHANNEL = 4;
constexpr int SERVO_LEDC_BITS = 14;
constexpr int SERVO_PERIOD_US = 20000;          // 50 Hz
constexpr int SERVO_MIN_US = 544, SERVO_MAX_US = 2400;  // ESP32Servo's default angle mapping

void writeInvertedServo(int angle) {
    angle = constrain(angle, 0, 180);
    int us = SERVO_MIN_US + (SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
    uint32_t maxDuty = (1u << SERVO_LEDC_BITS) - 1;
    uint32_t duty = (uint32_t)((uint64_t)us * maxDuty / SERVO_PERIOD_US);
    ledcWrite(INVERTED_SERVO_LEDC_CHANNEL, maxDuty - duty);
}
}  // namespace

Pen::Pen()
{
    if constexpr (BOARD.servoInverted) {
        servo = nullptr;
        ledcSetup(INVERTED_SERVO_LEDC_CHANNEL, 1000000 / SERVO_PERIOD_US, SERVO_LEDC_BITS);
        ledcAttachPin(BOARD.servo, INVERTED_SERVO_LEDC_CHANNEL);
    } else {
        servo = new Servo();
        servo->attach(BOARD.servo);
    }
    setRawValue(70);
}

void Pen::setRawValue(int rawValue) {
    if constexpr (BOARD.servoInverted) {
        writeInvertedServo(rawValue);
    } else {
        this->servo->write(rawValue);
    }
    currentPosition = rawValue;
}

void Pen::setPenDistance(int value) {
    Serial.println("Pen distance angle set to " + String(value));
    this->penDistance = value;
}

void Pen::slowUp() {
    if (penDistance == -1) {
        throw std::invalid_argument("not ready");
    }

    doSlowMove(this, currentPosition, 90, slowSpeedDegPerSec);
    currentPosition = 90;
}

void Pen::slowDown() {
    if (penDistance == -1) {
        throw std::invalid_argument("not ready");
    }

    doSlowMove(this, currentPosition, penDistance, slowSpeedDegPerSec);
    currentPosition = penDistance;
}

bool Pen::isDown() {
    return currentPosition == penDistance;
}