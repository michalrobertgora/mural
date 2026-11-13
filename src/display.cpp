#include "display.h"
#include <Adafruit_SSD1306.h>
#include "board.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define TEXT_HEIGHT 8

Display::Display() {
    display = nullptr;
    if (BOARD.i2cSda == NO_PIN || BOARD.i2cScl == NO_PIN) {
        Serial.println("Display: no I2C pins on this board, running headless");
        return;
    }
    auto i2c_valid = Wire.begin(BOARD.i2cSda, BOARD.i2cScl);
    Serial.printf("I2C on SDA=%d SCL=%d: %d\n", BOARD.i2cSda, BOARD.i2cScl, i2c_valid);
    // Probe first so a missing/miswired OLED doesn't stop the bot from booting.
    Wire.beginTransmission(0x3C);
    if (!i2c_valid || Wire.endTransmission() != 0) {
        Serial.println("Display: no SSD1306 answering at 0x3C, running headless");
        return;
    }
    auto oled = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    if (!oled->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed, running headless"));
        delete oled;
        return;
    }
    display = oled;
    display->setRotation(0);
    display->clearDisplay();
    display->setTextColor(WHITE);
    display->setTextSize(1);
    display->display();
}

void Display::drawLines(String lines[], int n) {
    if (!display) {
        String joined;
        for (int i = 0; i < n; i++) {
            joined += (i ? " | " : "") + lines[i];
        }
        Serial.println("[display] " + joined);
        return;
    }
    display->clearDisplay();
    int16_t x1, y1;
    uint16_t w, h;
    int zoneH = SCREEN_HEIGHT / n;
    for (int i = 0; i < n; i++) {
        int y = i * zoneH + (zoneH - TEXT_HEIGHT) / 2;
        display->getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);
        display->setCursor((SCREEN_WIDTH - w) / 2, y);
        display->print(lines[i]);
    }
    display->display();
}

void Display::showStarting() {
    String lines[] = {"Starting up..."};
    drawLines(lines, 1);
}

void Display::showConnecting() {
    String lines[] = {"Connecting", "to WiFi..."};
    drawLines(lines, 2);
}

void Display::showHotspot() {
    String lines[] = {"Connect to hotspot", "SSID: Mural"};
    drawLines(lines, 2);
}

void Display::showHotspotReady(String ipAddress) {
    ip = ipAddress;
    String lines[] = {"SSID: Mural", "http://" + ip};
    drawLines(lines, 2);
}

void Display::showConnected(String ipAddress) {
    ip = ipAddress;
    String lines[] = {ip, "http://mural.local"};
    drawLines(lines, 2);
}

void Display::showCalibration(double x, double y) {
    String lines[] = {"X:" + String(x, 1) + " Y:" + String(y, 1)};
    drawLines(lines, 1);
}

void Display::showNoCard() {
    String lines[] = {"NO CARD"};
    drawLines(lines, 1);
}

void Display::showFormatCountdown(int secondsLeft) {
    String lines[] = {"Formatting SD in", String(secondsLeft) + "s..."};
    drawLines(lines, 2);
}

void Display::showFormatting() {
    String lines[] = {"Formatting", "SD Card..."};
    drawLines(lines, 2);
}

void Display::showDrawing(int progress) {
    if (!display) {
        return;  // headless: progress is visible in the web UI
    }
    display->clearDisplay();

    const int x = 8;
    const int y = 8;
    const int width = SCREEN_WIDTH - 16;
    const int height = SCREEN_HEIGHT - 16;

    display->drawRect(x, y, width, height, WHITE);

    int fillWidth = (width - 2) * progress / 100;
    if (fillWidth > 0) {
        display->fillRect(x + 1, y + 1, fillWidth, height - 2, WHITE);
    }

    String label = String(progress) + "%";
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    display->setCursor(x + (width - w) / 2, y + (height - h) / 2 - y1);
    display->setTextColor(INVERSE);
    display->print(label);
    display->setTextColor(WHITE);

    display->display();
}
