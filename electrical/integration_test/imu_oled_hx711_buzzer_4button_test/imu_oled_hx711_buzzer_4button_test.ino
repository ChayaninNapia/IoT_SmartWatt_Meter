// IMU + OLED + HX711 + buzzer + 4-button integration test for ESP32 Dev Module / NodeMCU-32S.
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
//   HX711:
//     DOUT -> GPIO32
//     SCK  -> GPIO33
//     VCC  -> 3.3V
//     GND  -> GND
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
// This sketch plays a short startup sound, reads MPU6050 gyro in rad/s, reads
// HX711 force with gain 64, and shows a 3-tab SH1106 OLED UI. BTN1/BTN2 switch
// tabs, long-press BTN3 calibrates gyro offsets, and long-press BTN4 tares HX711.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "HX711.h"

struct ButtonInput {
  const char *name;
  uint8_t pin;
  unsigned int toneHz;
  bool stablePressed;
  bool lastRawPressed;
  unsigned long lastRawChangeMs;
  unsigned long pressedAtMs;
  uint32_t pressCount;
  bool longPressHandled;
};

enum DisplayTab {
  TAB_MAIN = 0,
  TAB_GYRO = 1,
  TAB_HX711 = 2,
  TAB_CAL = 3,
};

const uint8_t I2C_SDA_PIN = 21;
const uint8_t I2C_SCL_PIN = 22;
const uint8_t BUZZER_PIN = 13;
const int HX711_DOUT_PIN = 32;
const int HX711_SCK_PIN = 33;
const byte HX711_GAIN = 64;

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
const unsigned long HX711_SAMPLE_INTERVAL_MS = 100;
const unsigned long HX711_READY_TIMEOUT_MS = 2000;
const unsigned long TARE_LONG_PRESS_MS = 1600;
const unsigned long GYRO_CAL_LONG_PRESS_MS = 1600;
const unsigned long TARE_STATUS_MS = 1500;
const unsigned long GYRO_CAL_STATUS_MS = 1500;
const unsigned long HX711_CAL_STATUS_MS = 1500;
const int HX711_TARE_SAMPLES = 20;
const int HX711_CAL_SAMPLES = 20;
const int GYRO_CAL_SAMPLES = 80;
const float HX711_COUNTS_PER_NEWTON_DEFAULT = 851.6;
const float GRAVITY_MPS2 = 9.80665;
const float DEGREES_TO_RADIANS = 0.01745329252;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
HX711 hx711;

ButtonInput buttons[] = {
  {"BTN1", 25, 659, false, false, 0, 0, 0, false},
  {"BTN2", 26, 784, false, false, 0, 0, 0, false},
  {"BTN3", 27, 988, false, false, 0, 0, 0, false},
  {"BTN4", 14, 1319, false, false, 0, 0, 0, false},
};
const uint8_t BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

uint8_t mpuAddress = 0;
unsigned long sampleCount = 0;
unsigned long lastSampleMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastHx711SampleMs = 0;
unsigned long buzzerOffAtMs = 0;
unsigned long tareStatusUntilMs = 0;
unsigned long gyroCalStatusUntilMs = 0;
unsigned long hx711CalStatusUntilMs = 0;

float gxRadS = 0.0;
float gyRadS = 0.0;
float gzRadS = 0.0;
float gxOffsetRadS = 0.0;
float gyOffsetRadS = 0.0;
float gzOffsetRadS = 0.0;
bool latestReadOk = false;
const char *lastButtonName = "none";
uint32_t lastButtonPressCount = 0;
long hx711ZeroRaw = 0;
long hx711Raw = 0;
long hx711DeltaRaw = 0;
float forceN = 0.0;
float forceKg = 0.0;
float hx711CountsPerNewton = HX711_COUNTS_PER_NEWTON_DEFAULT;
bool hx711Tared = false;
bool hx711LatestReadOk = false;
const char *hx711StatusText = "HX INIT";
const char *gyroStatusText = "GY OK";
const char *hx711CalStatusText = "CAL RDY";
DisplayTab currentTab = TAB_MAIN;
float knownMassOptionsKg[] = {1.0, 2.0, 3.0};
const uint8_t KNOWN_MASS_OPTION_COUNT = sizeof(knownMassOptionsKg) / sizeof(knownMassOptionsKg[0]);
uint8_t knownMassIndex = 1;
long hx711CalDeltaAverage = 0;

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

void playTareSuccessSound(unsigned long now) {
  tone(BUZZER_PIN, 1760, 180);
  buzzerOffAtMs = now + 200;
}

void playGyroCalSuccessSound(unsigned long now) {
  tone(BUZZER_PIN, 1568, 90);
  buzzerOffAtMs = now + 110;
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

bool waitForHx711Ready(unsigned long timeoutMs) {
  const unsigned long startMs = millis();
  while (!hx711.is_ready()) {
    if (millis() - startMs >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool readHx711Raw(long &rawValue) {
  if (!waitForHx711Ready(HX711_READY_TIMEOUT_MS)) {
    return false;
  }

  rawValue = hx711.read();
  return true;
}

bool readHx711Average(int sampleCount, long &averageRaw) {
  long sum = 0;

  for (int i = 0; i < sampleCount; i++) {
    long raw = 0;
    if (!readHx711Raw(raw)) {
      return false;
    }
    sum += raw;
  }

  averageRaw = sum / sampleCount;
  return true;
}

void drawStatusScreen(const char *line1, const char *line2);

void tareHx711(unsigned long now) {
  hx711StatusText = "TARE...";
  drawStatusScreen("HX711 tare", "Keep no load");

  long averageRaw = 0;
  if (!readHx711Average(HX711_TARE_SAMPLES, averageRaw)) {
    hx711LatestReadOk = false;
    hx711StatusText = "TARE FAIL";
    tareStatusUntilMs = now + TARE_STATUS_MS;
    Serial.println("HX711 tare failed: not ready");
    return;
  }

  hx711ZeroRaw = averageRaw;
  hx711Raw = averageRaw;
  hx711DeltaRaw = 0;
  forceN = 0.0;
  forceKg = 0.0;
  hx711Tared = true;
  hx711LatestReadOk = true;
  hx711StatusText = "TARE OK";
  tareStatusUntilMs = millis() + TARE_STATUS_MS;
  playTareSuccessSound(millis());

  Serial.print("HX711_TARE_OK,zero_raw:");
  Serial.println(hx711ZeroRaw);
}

void previousTab() {
  currentTab = (currentTab == TAB_MAIN) ? TAB_CAL : (DisplayTab)(currentTab - 1);
}

void nextTab() {
  currentTab = (currentTab == TAB_CAL) ? TAB_MAIN : (DisplayTab)(currentTab + 1);
}

void nextKnownMass() {
  knownMassIndex = (knownMassIndex + 1) % KNOWN_MASS_OPTION_COUNT;
  hx711CalStatusText = "MASS SET";
  hx711CalStatusUntilMs = millis() + HX711_CAL_STATUS_MS;

  Serial.print("HX711_CAL_KNOWN_MASS,kg:");
  Serial.println(knownMassOptionsKg[knownMassIndex], 2);
}

void calculateHx711CountsPerNewton(unsigned long now) {
  if (!hx711Tared) {
    hx711CalStatusText = "TARE 1ST";
    hx711CalStatusUntilMs = now + HX711_CAL_STATUS_MS;
    Serial.println("HX711_CAL_FAIL,not_tared");
    return;
  }

  hx711CalStatusText = "CALC...";
  drawStatusScreen("HX CAL", "Keep load still");

  long averageRaw = 0;
  if (!readHx711Average(HX711_CAL_SAMPLES, averageRaw)) {
    hx711LatestReadOk = false;
    hx711CalStatusText = "CAL FAIL";
    hx711CalStatusUntilMs = millis() + HX711_CAL_STATUS_MS;
    Serial.println("HX711_CAL_FAIL,not_ready");
    return;
  }

  hx711Raw = averageRaw;
  hx711CalDeltaAverage = averageRaw - hx711ZeroRaw;
  hx711DeltaRaw = hx711CalDeltaAverage;

  const float knownForceN = knownMassOptionsKg[knownMassIndex] * GRAVITY_MPS2;
  if (knownForceN <= 0.0 || hx711CalDeltaAverage == 0) {
    hx711CalStatusText = "BAD LOAD";
    hx711CalStatusUntilMs = millis() + HX711_CAL_STATUS_MS;
    Serial.println("HX711_CAL_FAIL,bad_load");
    return;
  }

  hx711CountsPerNewton = hx711CalDeltaAverage / knownForceN;
  if (hx711CountsPerNewton < 0.0) {
    hx711CountsPerNewton = -hx711CountsPerNewton;
  }

  forceN = hx711DeltaRaw / hx711CountsPerNewton;
  forceKg = forceN / GRAVITY_MPS2;
  hx711LatestReadOk = true;
  hx711CalStatusText = "CAL OK";
  hx711CalStatusUntilMs = millis() + HX711_CAL_STATUS_MS;

  Serial.print("HX711_CAL_OK,known_kg:");
  Serial.print(knownMassOptionsKg[knownMassIndex], 2);
  Serial.print(",delta_avg:");
  Serial.print(hx711CalDeltaAverage);
  Serial.print(",counts_per_N:");
  Serial.println(hx711CountsPerNewton, 3);
}

void handleShortPress(const ButtonInput &button) {
  if (button.pin == 25) {
    previousTab();
  } else if (button.pin == 26) {
    nextTab();
  } else if (button.pin == 27 && currentTab == TAB_CAL) {
    nextKnownMass();
  } else if (button.pin == 14 && currentTab == TAB_CAL) {
    calculateHx711CountsPerNewton(millis());
  }
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

  gxRadS = ((rawGx / 131.0) * DEGREES_TO_RADIANS) - gxOffsetRadS;
  gyRadS = ((rawGy / 131.0) * DEGREES_TO_RADIANS) - gyOffsetRadS;
  gzRadS = ((rawGz / 131.0) * DEGREES_TO_RADIANS) - gzOffsetRadS;

  sampleCount++;
  latestReadOk = true;
  if (millis() > gyroCalStatusUntilMs) {
    gyroStatusText = "GY OK";
  }
  return true;
}

bool readGyroRawRadS(float &gxRawRadS, float &gyRawRadS, float &gzRawRadS) {
  uint8_t data[14];
  if (!readRegisters(mpuAddress, REG_ACCEL_XOUT_H, data, sizeof(data))) {
    return false;
  }

  const int16_t rawGx = combineInt16(data[8], data[9]);
  const int16_t rawGy = combineInt16(data[10], data[11]);
  const int16_t rawGz = combineInt16(data[12], data[13]);

  gxRawRadS = (rawGx / 131.0) * DEGREES_TO_RADIANS;
  gyRawRadS = (rawGy / 131.0) * DEGREES_TO_RADIANS;
  gzRawRadS = (rawGz / 131.0) * DEGREES_TO_RADIANS;
  return true;
}

void calibrateGyro(unsigned long now) {
  if (mpuAddress == 0) {
    gyroStatusText = "GY FAIL";
    gyroCalStatusUntilMs = now + GYRO_CAL_STATUS_MS;
    Serial.println("GYRO_CAL_FAIL,mpu_not_ready");
    return;
  }

  gyroStatusText = "CAL...";
  drawStatusScreen("GYRO cal", "Keep still");

  float gxSum = 0.0;
  float gySum = 0.0;
  float gzSum = 0.0;

  for (int i = 0; i < GYRO_CAL_SAMPLES; i++) {
    float gxRaw = 0.0;
    float gyRaw = 0.0;
    float gzRaw = 0.0;

    if (!readGyroRawRadS(gxRaw, gyRaw, gzRaw)) {
      gyroStatusText = "GY FAIL";
      gyroCalStatusUntilMs = millis() + GYRO_CAL_STATUS_MS;
      Serial.println("GYRO_CAL_FAIL,read_error");
      return;
    }

    gxSum += gxRaw;
    gySum += gyRaw;
    gzSum += gzRaw;
    delay(5);
  }

  gxOffsetRadS = gxSum / GYRO_CAL_SAMPLES;
  gyOffsetRadS = gySum / GYRO_CAL_SAMPLES;
  gzOffsetRadS = gzSum / GYRO_CAL_SAMPLES;
  gxRadS = 0.0;
  gyRadS = 0.0;
  gzRadS = 0.0;
  latestReadOk = true;
  gyroStatusText = "CAL OK";
  gyroCalStatusUntilMs = millis() + GYRO_CAL_STATUS_MS;
  playGyroCalSuccessSound(millis());

  Serial.print("GYRO_CAL_OK,gx_offset:");
  Serial.print(gxOffsetRadS, 5);
  Serial.print(",gy_offset:");
  Serial.print(gyOffsetRadS, 5);
  Serial.print(",gz_offset:");
  Serial.println(gzOffsetRadS, 5);
}

void readHx711Sample() {
  if (!hx711.is_ready()) {
    hx711LatestReadOk = false;
    hx711StatusText = "HX WAIT";
    return;
  }

  hx711Raw = hx711.read();
  hx711DeltaRaw = hx711Tared ? hx711Raw - hx711ZeroRaw : 0;
  forceN = hx711Tared ? hx711DeltaRaw / hx711CountsPerNewton : 0.0;
  forceKg = forceN / GRAVITY_MPS2;
  hx711LatestReadOk = true;

  if (millis() > tareStatusUntilMs) {
    hx711StatusText = hx711Tared ? "HX OK" : "NO TARE";
  }
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
  Serial.print(gzRadS, 4);
  Serial.print("\t");
  Serial.print("hx_raw:");
  Serial.print(hx711Raw);
  Serial.print("\t");
  Serial.print("hx_delta:");
  Serial.print(hx711DeltaRaw);
  Serial.print("\t");
  Serial.print("force_N:");
  Serial.print(forceN, 3);
  Serial.print("\t");
  Serial.print("force_kg:");
  Serial.print(forceKg, 3);
  Serial.print("\t");
  Serial.print("hx_status:");
  Serial.println(hx711StatusText);
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
        button.pressedAtMs = now;
        button.longPressHandled = false;
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
      } else {
        if (!button.longPressHandled) {
          handleShortPress(button);
        }
        button.pressedAtMs = 0;
        button.longPressHandled = false;
      }
    }

    if (button.stablePressed &&
        button.pin == 27 &&
        currentTab == TAB_GYRO &&
        !button.longPressHandled &&
        (now - button.pressedAtMs) >= GYRO_CAL_LONG_PRESS_MS) {
      button.longPressHandled = true;
      lastButtonName = "GYCAL";
      calibrateGyro(now);
    }

    if (button.stablePressed &&
        button.pin == 14 &&
        (currentTab == TAB_HX711 || currentTab == TAB_CAL) &&
        !button.longPressHandled &&
        (now - button.pressedAtMs) >= TARE_LONG_PRESS_MS) {
      button.longPressHandled = true;
      lastButtonName = "TARE";
      tareHx711(now);
    }
  }
}

const char *currentButtonName() {
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (buttons[i].stablePressed) {
      return buttons[i].name;
    }
  }

  return lastButtonName;
}

void drawStatusScreen(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WattMeter UI");
  display.println(line1);
  display.println(line2);
  display.display();
}

void drawButtonFooter(uint8_t y) {
  display.setCursor(0, y);
  display.print("BTN ");
  display.print(currentButtonName());
  display.print(" #");
  display.print(lastButtonPressCount);
}

void drawMainTab() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("[MAIN]        1/4");

  display.setCursor(0, 12);
  display.print("F  ");
  if (hx711Tared && hx711LatestReadOk) {
    display.print(forceN, 2);
  } else {
    display.print("--.--");
  }
  display.println(" N");

  display.setCursor(0, 24);
  display.print("M  ");
  if (hx711Tared && hx711LatestReadOk) {
    display.print(forceKg, 2);
  } else {
    display.print("--.--");
  }
  display.println(" kg");

  display.setCursor(0, 36);
  display.print("Gy ");
  display.print(gyRadS, 3);
  display.println(" r/s");

  display.setCursor(0, 48);
  display.print("HX ");
  display.print(hx711StatusText);
  display.print("   <-B1 B2->");

  display.display();
}

void drawGyroTab() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("[GYRO rad/s] 2/4");

  display.setCursor(0, 12);
  display.print("Gx ");
  display.println(gxRadS, 3);

  display.setCursor(0, 24);
  display.print("Gy ");
  display.println(gyRadS, 3);

  display.setCursor(0, 36);
  display.print("Gz ");
  display.println(gzRadS, 3);

  display.setCursor(0, 48);
  if (millis() <= gyroCalStatusUntilMs) {
    display.print(gyroStatusText);
  } else {
    display.print("HOLD B3 cal");
  }

  display.display();
}

void drawHx711Tab() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("[HX711]      3/4");

  display.setCursor(0, 10);
  if (millis() <= tareStatusUntilMs) {
    display.println(hx711StatusText);
  } else {
    display.print("N");
    if (hx711Tared && hx711LatestReadOk) {
      display.print(forceN, 2);
    } else {
      display.print("--.--");
    }
    display.print(" kg");
    if (hx711Tared && hx711LatestReadOk) {
      display.println(forceKg, 2);
    } else {
      display.println("--.--");
    }
  }

  display.setCursor(0, 20);
  display.print("dR ");
  display.println(hx711DeltaRaw);

  display.setCursor(0, 30);
  display.print("off ");
  display.println(hx711ZeroRaw);

  display.setCursor(0, 40);
  display.print("c/N ");
  display.println(hx711CountsPerNewton, 1);

  display.setCursor(0, 52);
  display.print("HOLD B4 tare");

  display.display();
}

void drawCalTab() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("[CAL]        4/4");

  display.setCursor(0, 12);
  display.print("known ");
  display.print(knownMassOptionsKg[knownMassIndex], 2);
  display.println("kg");

  display.setCursor(0, 24);
  display.print("dRavg ");
  display.println(hx711CalDeltaAverage);

  display.setCursor(0, 36);
  display.print("c/N ");
  display.print(hx711CountsPerNewton, 1);

  display.setCursor(0, 48);
  if (millis() <= hx711CalStatusUntilMs) {
    display.print(hx711CalStatusText);
  } else {
    display.print("B3 mass B4 calc");
  }

  display.display();
}

void drawCurrentTab() {
  if (currentTab == TAB_MAIN) {
    drawMainTab();
  } else if (currentTab == TAB_GYRO) {
    drawGyroTab();
  } else if (currentTab == TAB_HX711) {
    drawHx711Tab();
  } else {
    drawCalTab();
  }
}

void drawGyroScreen() {
  drawCurrentTab();
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
  Serial.println("HX711 DOUT GPIO32, SCK GPIO33, gain 64");
  Serial.print("HX711 calibration counts_per_N:");
  Serial.println(hx711CountsPerNewton, 3);
  Serial.println("Buttons: GPIO25, GPIO26, GPIO27, GPIO14");
  Serial.println("Button 5 / GPIO16: not used");
  Serial.println("BTN1 short press: previous tab");
  Serial.println("BTN2 short press: next tab");
  Serial.println("BTN3 short press on CAL tab: select known mass");
  Serial.println("BTN4 short press on CAL tab: calculate HX711 c/N");
  Serial.println("BTN3 long press on GYRO tab: gyro calibration");
  Serial.println("BTN4 long press on HX711/CAL tab: HX711 tare");
  Serial.println("OLED SH1106 address: 0x3C");

  playStartupSound();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
  hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN);
  hx711CalDeltaAverage = hx711DeltaRaw;

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
    drawStatusScreen("4 buttons ready", "Taring HX711...");
  }

  tareHx711(millis());
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

  if ((now - lastHx711SampleMs) >= HX711_SAMPLE_INTERVAL_MS) {
    lastHx711SampleMs = now;
    readHx711Sample();
  }

  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    drawCurrentTab();
  }
}
