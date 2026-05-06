// OLED display unit test for ESP32-S2 using Adafruit SH110X.
// Use this for 1.3 inch I2C OLED modules that use the SH1106 controller.
//
// Wiring from project GPIO Mapping:
//   OLED SDA -> GPIO21
//   OLED SCL/SCK -> GPIO22
//   OLED VCC -> 3.3V or 5V according to the OLED module
//   OLED GND -> GND

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const uint8_t OLED_SDA_PIN = 21;
const uint8_t OLED_SCL_PIN = 22;
const uint8_t OLED_I2C_ADDRESS = 0x3C;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = -1;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

unsigned long frameCounter = 0;

void printI2CScan() {
  Serial.println();
  Serial.println("Scanning I2C bus...");

  uint8_t deviceCount = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.print("I2C device count: ");
    Serial.println(deviceCount);
  }
  Serial.println();
}

void drawStaticTestPattern() {
  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED UNIT TEST");
  display.println("Adafruit SH1106");
  display.println("I2C: SDA21 SCL22");

  display.drawLine(0, 28, 127, 28, SH110X_WHITE);
  display.drawRect(0, 34, 128, 30, SH110X_WHITE);
  display.fillRect(4, 38, 28, 10, SH110X_WHITE);
  display.drawCircle(52, 44, 7, SH110X_WHITE);
  display.drawTriangle(78, 51, 91, 37, 104, 51, SH110X_WHITE);

  display.setCursor(4, 53);
  display.print("ADDR 0x");
  display.print(OLED_I2C_ADDRESS, HEX);

  display.display();
}

void drawAnimatedStatus() {
  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED SH1106 TEST");
  display.println("If readable: PASS");

  display.setCursor(0, 24);
  display.print("Frame: ");
  display.println(frameCounter);

  const int barWidth = frameCounter % 121;
  display.drawRect(0, 42, 122, 12, SH110X_WHITE);
  display.fillRect(1, 43, barWidth, 10, SH110X_WHITE);

  display.setCursor(0, 56);
  display.print("GPIO21/22 0x");
  display.print(OLED_I2C_ADDRESS, HEX);

  display.display();
  frameCounter++;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("OLED display unit test started");
  Serial.println("Library: Adafruit SH110X / SH1106G");
  Serial.println("SDA: GPIO21, SCL: GPIO22, address: 0x3C");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);

  printI2CScan();

  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    Serial.println("SH1106 init failed");
    Serial.println("Check wiring, I2C address, and OLED controller type.");
    while (true) {
      delay(1000);
    }
  }

  display.setRotation(0);
  Serial.println("OLED initialized successfully");
  drawStaticTestPattern();
  delay(3000);
}

void loop() {
  drawAnimatedStatus();
  delay(250);
}
