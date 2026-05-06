// IMU + OLED + buzzer + 4-button integration test for ESP32 Dev Module / NodeMCU-32S.
//
// Wiring:
//   Shared I2C bus:
//     ESP32 SDA -> GPIO21
//     ESP32 SCL -> GPIO22
//
//   MPU6050:
//     VCC -> 3.3V
//     GND -> GND
//     SDA -> GPIO21
//     SCL -> GPIO22
//     AD0 -> optional on common GY-521 modules; LOW/GND gives address 0x68
//     INT -> not used
//
//   OLED SH1106 1.3 inch I2C:
//     VCC -> 3.3V
//     GND -> GND
//     SDA -> GPIO21
//     SCL/SCK -> GPIO22
//     I2C address -> 0x3C
//
//   Passive buzzer:
//     Signal -> GPIO13
//     GND -> GND
//
//   Active-high buttons:
//     Button 1 -> GPIO25
//     Button 2 -> GPIO26
//     Button 3 -> GPIO27
//     Button 4 -> GPIO14
//
// Button wiring uses INPUT_PULLDOWN: released = LOW, pressed = HIGH.
// Button 5 / GPIO16 is intentionally not used in the current system.
//
// This sketch plays a short startup sound, reads MPU6050 gyro values in rad/s,
// updates the SH1106 OLED, and plays a distinct response tone for each button.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

struct ButtonInput {
  const char *name;
  uint8_t pin;
  unsigned int toneHz;
  bool stablePressed;
  bool lastRawPressed;
  unsigned long lastRawChangeMs;
  uint32_t pressCount;
};

const uint8_t I2C_SDA_PIN = 21;
const uint8_t I2C_SCL_PIN = 22;
const uint8_t BUZZER_PIN = 13;

const uint8_t OLED_I2C_ADDRESS = 0x3C;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = -1;

const uint8_t MPU6050_ADDR_LOW = 0x68;
const uint8_t MPU6050_ADDR_HIGH = 0x69;

const uint8_t REG_SMPLRT_DIV = 0x19;
const uint8_t REG_CONFIG = 0x1A;
const uint8_t REG_GYRO_CONFIG = 0x1B;
const uint8_t REG_ACCEL_CONFIG = 0x1C;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1 = 0x6B;
const uint8_t REG_WHO_AM_I = 0x75;

const uint8_t BUTTON_PIN_MODE = INPUT_PULLDOWN;
const uint8_t BUTTON_PRESSED_LEVEL = HIGH;
const unsigned long DEBOUNCE_MS = 35;
const unsigned long BUTTON_TONE_MS = 110;
const unsigned long SAMPLE_INTERVAL_MS = 50;
const unsigned long DISPLAY_INTERVAL_MS = 250;
const float DEGREES_TO_RADIANS = 0.01745329252;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

ButtonInput buttons[] = {
  {"BTN1", 25, 659, false, false, 0, 0},
  {"BTN2", 26, 784, false, false, 0, 0},
  {"BTN3", 27, 988, false, false, 0, 0},
  {"BTN4", 14, 1319, false, false, 0, 0},
};
const uint8_t BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

uint8_t mpuAddress = 0;
unsigned long sampleCount = 0;
unsigned long lastSampleMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long buzzerOffAtMs = 0;

float gxRadS = 0.0;
float gyRadS = 0.0;
float gzRadS = 0.0;
bool latestReadOk = false;
const char *lastButtonName = "none";
uint32_t lastButtonPressCount = 0;

void playToneBlocking(unsigned int frequency, unsigned long durationMs, unsigned long gapMs) {
  tone(BUZZER_PIN, frequency, durationMs);
  delay(durationMs + gapMs);
  noTone(BUZZER_PIN);
}

void playStartupSound() {
  playToneBlocking(784, 90, 30);
  playToneBlocking(988, 90, 30);
  playToneBlocking(1319, 140, 0);
  digitalWrite(BUZZER_PIN, LOW);
}

void playButtonResponse(const ButtonInput &button, unsigned long now) {
  tone(BUZZER_PIN, button.toneHz, BUTTON_TONE_MS);
  buzzerOffAtMs = now + BUTTON_TONE_MS + 10;
}

void updateBuzzer(unsigned long now) {
  if (buzzerOffAtMs != 0 && now >= buzzerOffAtMs) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOffAtMs = 0;
  }
}

bool readPressed(uint8_t pin) {
  return digitalRead(pin) == BUTTON_PRESSED_LEVEL;
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void scanI2CBus() {
  Serial.println();
  Serial.println("Scanning I2C bus...");

  uint8_t deviceCount = 0;
  for (uint8_t address = 1; address < 127; address++) {
    if (i2cDevicePresent(address)) {
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

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t address, uint8_t startReg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(address);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t received = Wire.requestFrom(address, length);
  if (received != length) {
    return false;
  }

  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

bool readRegister(uint8_t address, uint8_t reg, uint8_t &value) {
  return readRegisters(address, reg, &value, 1);
}

int16_t combineInt16(uint8_t highByte, uint8_t lowByte) {
  return (int16_t)((highByte << 8) | lowByte);
}

uint8_t detectMpuAddress() {
  if (i2cDevicePresent(MPU6050_ADDR_LOW)) {
    return MPU6050_ADDR_LOW;
  }

  if (i2cDevicePresent(MPU6050_ADDR_HIGH)) {
    return MPU6050_ADDR_HIGH;
  }

  return 0;
}

bool initializeMpu6050() {
  mpuAddress = detectMpuAddress();
  if (mpuAddress == 0) {
    Serial.println("MPU6050 not found at 0x68 or 0x69");
    Serial.println("Check SDA/SCL wiring, power, GND, and AD0 pin.");
    return false;
  }

  Serial.print("MPU6050 found at 0x");
  Serial.println(mpuAddress, HEX);

  uint8_t whoAmI = 0;
  if (!readRegister(mpuAddress, REG_WHO_AM_I, whoAmI)) {
    Serial.println("Failed to read WHO_AM_I register");
    return false;
  }

  Serial.print("WHO_AM_I: 0x");
  if (whoAmI < 16) {
    Serial.print("0");
  }
  Serial.println(whoAmI, HEX);

  if ((whoAmI & 0x7E) != 0x68) {
    Serial.println("Warning: WHO_AM_I is not the usual MPU6050 value 0x68");
  }

  // Wake sensor, use X gyro PLL clock, set accel to +/-2g and gyro to +/-250 dps.
  if (!writeRegister(mpuAddress, REG_PWR_MGMT_1, 0x01)) {
    return false;
  }
  delay(100);

  writeRegister(mpuAddress, REG_SMPLRT_DIV, 0x07);
  writeRegister(mpuAddress, REG_CONFIG, 0x03);
  writeRegister(mpuAddress, REG_GYRO_CONFIG, 0x00);
  writeRegister(mpuAddress, REG_ACCEL_CONFIG, 0x00);

  Serial.println("MPU6050 initialized");
  return true;
}

bool readGyroRadS() {
  uint8_t data[14];
  if (!readRegisters(mpuAddress, REG_ACCEL_XOUT_H, data, sizeof(data))) {
    latestReadOk = false;
    return false;
  }

  const int16_t rawGx = combineInt16(data[8], data[9]);
  const int16_t rawGy = combineInt16(data[10], data[11]);
  const int16_t rawGz = combineInt16(data[12], data[13]);

  gxRadS = (rawGx / 131.0) * DEGREES_TO_RADIANS;
  gyRadS = (rawGy / 131.0) * DEGREES_TO_RADIANS;
  gzRadS = (rawGz / 131.0) * DEGREES_TO_RADIANS;

  sampleCount++;
  latestReadOk = true;
  return true;
}

void printGyroSample() {
  Serial.print("sample:");
  Serial.print(sampleCount);
  Serial.print("\t");
  Serial.print("gx_rads:");
  Serial.print(gxRadS, 4);
  Serial.print("\t");
  Serial.print("gy_rads:");
  Serial.print(gyRadS, 4);
  Serial.print("\t");
  Serial.print("gz_rads:");
  Serial.println(gzRadS, 4);
}

void handleButtons(unsigned long now) {
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    ButtonInput &button = buttons[i];
    const bool rawPressed = readPressed(button.pin);

    if (rawPressed != button.lastRawPressed) {
      button.lastRawPressed = rawPressed;
      button.lastRawChangeMs = now;
    }

    if ((now - button.lastRawChangeMs) >= DEBOUNCE_MS &&
        rawPressed != button.stablePressed) {
      button.stablePressed = rawPressed;

      if (button.stablePressed) {
        button.pressCount++;
        lastButtonName = button.name;
        lastButtonPressCount = button.pressCount;
        playButtonResponse(button, now);

        Serial.print(button.name);
        Serial.print(" GPIO");
        Serial.print(button.pin);
        Serial.print(" pressed tone=");
        Serial.print(button.toneHz);
        Serial.print("Hz count=");
        Serial.println(button.pressCount);
      }
    }
  }
}

void drawStatusScreen(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IMU OLED BTN BUZ");
  display.println(line1);
  display.println(line2);
  display.display();
}

void drawGyroScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("IMU OLED BTN BUZ");

  display.setCursor(0, 12);
  display.print("MPU:0x");
  if (mpuAddress < 16) {
    display.print("0");
  }
  display.print(mpuAddress, HEX);
  display.print(" ");
  display.println(latestReadOk ? "OK" : "ERR");

  display.setCursor(0, 24);
  display.print("gx ");
  display.print(gxRadS, 3);
  display.println(" rad/s");

  display.setCursor(0, 36);
  display.print("gy ");
  display.print(gyRadS, 3);
  display.println(" rad/s");

  display.setCursor(0, 48);
  display.print("gz ");
  display.print(gzRadS, 3);
  display.println(" rad/s");

  display.setCursor(0, 56);
  display.print(lastButtonName);
  display.print(" #");
  display.print(lastButtonPressCount);

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  const unsigned long now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttons[i].pin, BUTTON_PIN_MODE);
    buttons[i].stablePressed = readPressed(buttons[i].pin);
    buttons[i].lastRawPressed = buttons[i].stablePressed;
    buttons[i].lastRawChangeMs = now;
  }

  Serial.println("IMU + OLED + buzzer + 4-button integration test started");
  Serial.println("Board target: ESP32 Dev Module");
  Serial.println("I2C wiring: SDA GPIO21, SCL GPIO22");
  Serial.println("Buzzer signal: GPIO13");
  Serial.println("Buttons: GPIO25, GPIO26, GPIO27, GPIO14");
  Serial.println("Button 5 / GPIO16: not used");
  Serial.println("OLED SH1106 address: 0x3C");

  playStartupSound();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  scanI2CBus();

  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    Serial.println("SH1106 init failed");
    while (true) {
      delay(1000);
    }
  }
  display.setRotation(0);
  drawStatusScreen("OLED initialized", "Starting MPU...");

  if (!initializeMpu6050()) {
    drawStatusScreen("MPU init failed", "Retrying...");
  } else {
    drawStatusScreen("4 buttons ready", "Rotate / press");
  }
}

void loop() {
  const unsigned long now = millis();

  updateBuzzer(now);
  handleButtons(now);

  if (mpuAddress == 0) {
    if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
      lastDisplayMs = now;
      scanI2CBus();
      if (!initializeMpu6050()) {
        drawStatusScreen("MPU not found", "Check wiring");
      }
    }
    return;
  }

  if ((now - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    if (readGyroRadS()) {
      printGyroSample();
    } else {
      Serial.println("Gyro read failed");
    }
  }

  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    drawGyroScreen();
  }
}
