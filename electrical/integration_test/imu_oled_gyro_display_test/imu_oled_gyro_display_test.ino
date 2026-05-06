// IMU + OLED integration test for ESP32 Dev Module / NodeMCU-32S.
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
//     AD0 -> GND or LOW for address 0x68
//     INT -> not used
//
//   OLED SH1106 1.3 inch I2C:
//     VCC -> 3.3V
//     GND -> GND
//     SDA -> GPIO21
//     SCL/SCK -> GPIO22
//     I2C address -> 0x3C
//
// This sketch reads MPU6050 gyro values using direct I2C registers,
// converts gx/gy/gz to rad/s, and updates the SH1106 OLED display.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const uint8_t I2C_SDA_PIN = 21;
const uint8_t I2C_SCL_PIN = 22;

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

const unsigned long SAMPLE_INTERVAL_MS = 50;
const unsigned long DISPLAY_INTERVAL_MS = 250;
const float DEGREES_TO_RADIANS = 0.01745329252;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

uint8_t mpuAddress = 0;
unsigned long sampleCount = 0;
unsigned long lastSampleMs = 0;
unsigned long lastDisplayMs = 0;

float gxRadS = 0.0;
float gyRadS = 0.0;
float gzRadS = 0.0;
bool latestReadOk = false;

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

void drawStatusScreen(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IMU + OLED TEST");
  display.println(line1);
  display.println(line2);
  display.display();
}

void drawGyroScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("IMU + OLED TEST");

  display.setCursor(0, 12);
  display.print("MPU: 0x");
  if (mpuAddress < 16) {
    display.print("0");
  }
  display.print(mpuAddress, HEX);
  display.print(" ");
  display.println(latestReadOk ? "OK" : "READ ERR");

  display.setCursor(0, 24);
  display.print("gx ");
  display.print(gxRadS, 4);
  display.println(" rad/s");

  display.setCursor(0, 36);
  display.print("gy ");
  display.print(gyRadS, 4);
  display.println(" rad/s");

  display.setCursor(0, 48);
  display.print("gz ");
  display.print(gzRadS, 4);
  display.println(" rad/s");

  display.setCursor(84, 56);
  display.print("#");
  display.print(sampleCount);

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("IMU + OLED integration test started");
  Serial.println("Board target: ESP32 Dev Module");
  Serial.println("I2C wiring: SDA GPIO21, SCL GPIO22");
  Serial.println("OLED SH1106 address: 0x3C");

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
    drawStatusScreen("MPU initialized", "Rotate sensor");
  }
}

void loop() {
  const unsigned long now = millis();

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
