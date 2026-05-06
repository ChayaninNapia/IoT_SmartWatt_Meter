// MPU6050 unit test for ESP32 Dev Module / NodeMCU-32S.
//
// Wiring from project GPIO Mapping:
//   MPU6050 SDA -> GPIO21
//   MPU6050 SCL -> GPIO22
//   MPU6050 VCC -> 3.3V or 5V according to the module
//   MPU6050 GND -> GND
//
// This sketch uses only Wire and direct MPU6050 registers, so no external
// sensor library is required for the first hardware bring-up.

#include <Wire.h>

const uint8_t I2C_SDA_PIN = 21;
const uint8_t I2C_SCL_PIN = 22;

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
const float STANDARD_GRAVITY_MS2 = 9.80665;
const float DEGREES_TO_RADIANS = 0.01745329252;

uint8_t mpuAddress = 0;
unsigned long sampleCount = 0;
unsigned long lastSampleMs = 0;

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
  Serial.println("Move/rotate the module and watch accel/gyro values change.");
  Serial.println();
  Serial.println("Serial Plotter format:");
  Serial.println("ax_ms2:value ay_ms2:value az_ms2:value temp_degC:value gx_rads:value gy_rads:value gz_rads:value");
  return true;
}

void printMpuSample() {
  uint8_t data[14];
  if (!readRegisters(mpuAddress, REG_ACCEL_XOUT_H, data, sizeof(data))) {
    Serial.println("Read failed");
    return;
  }

  const int16_t rawAx = combineInt16(data[0], data[1]);
  const int16_t rawAy = combineInt16(data[2], data[3]);
  const int16_t rawAz = combineInt16(data[4], data[5]);
  const int16_t rawTemp = combineInt16(data[6], data[7]);
  const int16_t rawGx = combineInt16(data[8], data[9]);
  const int16_t rawGy = combineInt16(data[10], data[11]);
  const int16_t rawGz = combineInt16(data[12], data[13]);

  const float axG = rawAx / 16384.0;
  const float ayG = rawAy / 16384.0;
  const float azG = rawAz / 16384.0;
  const float tempC = (rawTemp / 340.0) + 36.53;
  const float gxDps = rawGx / 131.0;
  const float gyDps = rawGy / 131.0;
  const float gzDps = rawGz / 131.0;
  const float axMs2 = axG * STANDARD_GRAVITY_MS2;
  const float ayMs2 = ayG * STANDARD_GRAVITY_MS2;
  const float azMs2 = azG * STANDARD_GRAVITY_MS2;
  const float gxRadS = gxDps * DEGREES_TO_RADIANS;
  const float gyRadS = gyDps * DEGREES_TO_RADIANS;
  const float gzRadS = gzDps * DEGREES_TO_RADIANS;

  sampleCount++;

  Serial.print("ax_ms2:");
  Serial.print(axMs2, 3);
  Serial.print("\t");
  Serial.print("ay_ms2:");
  Serial.print(ayMs2, 3);
  Serial.print("\t");
  Serial.print("az_ms2:");
  Serial.print(azMs2, 3);
  Serial.print("\t");
  Serial.print("temp_degC:");
  Serial.print(tempC, 2);
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

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("MPU6050 unit test started");
  Serial.println("Board target: ESP32 Dev Module");
  Serial.println("I2C wiring: SDA GPIO21, SCL GPIO22");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  scanI2CBus();

  if (!initializeMpu6050()) {
    Serial.println("MPU6050 init failed. Retrying every 2 seconds...");
  }
}

void loop() {
  if (mpuAddress == 0) {
    delay(2000);
    scanI2CBus();
    initializeMpu6050();
    return;
  }

  const unsigned long now = millis();
  if ((now - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    printMpuSample();
  }
}
