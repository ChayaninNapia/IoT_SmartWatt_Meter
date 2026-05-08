// Smart Watt Meter full-system integration firmware.
//
// Board: ESP32 Dev Module / NodeMCU-32S
// OLED: SH1106 128x64 I2C at 0x3C
// Sensors: MPU6050 on I2C, HX711 on GPIO32/33
// Controls: 4 active-high buttons with INPUT_PULLDOWN, passive buzzer GPIO13
// BLE: telemetry notify packet v2 for the Flutter app.
//
// Button map:
//   BTN1 GPIO25 short: previous OLED screen
//   BTN2 GPIO26 short: next OLED screen
//   BTN3 GPIO27 short: no-op / reserved
//   BTN3 GPIO27 long on SENSOR/SYS: gyro bias calibration, keep crank still
//   BTN4 GPIO14 long on HX/SENSOR/SYS: HX711 tare, remove load
//   BTN4 GPIO14 long on LIVE: crank axis calibration, spin crank steadily
//
// App status write characteristic accepts compact JSON such as:
//   {"type":"appStatus","gpsOk":true,"recording":true}
// or commands:
//   {"type":"command","cmd":"tare"}
//   {"type":"command","cmd":"gyroBias"}
//   {"type":"command","cmd":"axisCal"}

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HX711.h"
#include <math.h>
#include <string>

struct ButtonInput {
  const char *name;
  uint8_t pin;
  uint16_t toneHz;
  bool stablePressed;
  bool lastRawPressed;
  unsigned long lastRawChangeMs;
  unsigned long pressedAtMs;
  bool longPressHandled;
};

struct GyroDps {
  float x;
  float y;
  float z;
};

enum DisplayScreen {
  SCREEN_LIVE = 0,
  SCREEN_SENSOR = 1,
  SCREEN_HX = 2,
  SCREEN_BLE = 3,
  SCREEN_SYS = 4,
};

const char *DEVICE_NAME = "ESP32_PowerMeter_01";
const char *SERVICE_UUID = "12345678-1234-1234-1234-123456789012";
const char *TELEMETRY_CHAR_UUID = "12345678-1234-1234-1234-123456789013";
const char *APP_STATUS_CHAR_UUID = "12345678-1234-1234-1234-123456789014";

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
const unsigned long LONG_PRESS_MS = 1500;
const unsigned long SAMPLE_INTERVAL_MS = 20;       // 50 Hz IMU
const unsigned long HX_INTERVAL_MS = 100;          // 10 Hz HX711
const unsigned long DISPLAY_INTERVAL_MS = 200;     // 5 Hz OLED
const unsigned long TELEMETRY_INTERVAL_MS = 1000;  // 1 Hz BLE
const unsigned long APP_STATUS_TIMEOUT_MS = 3500;
const unsigned long HX_READY_TIMEOUT_MS = 2500;
const int HX_TARE_SAMPLES = 20;
const int GYRO_BIAS_SAMPLES = 250;                 // about 5 s at 20 ms
const int AXIS_CAL_SAMPLES = 500;                  // about 10 s at 20 ms
const float AXIS_MIN_OMEGA_DPS = 30.0;
const uint16_t AXIS_MIN_VALID_SAMPLES = 250;

const float DPS_TO_RAD_S = 0.01745329252;
const float GRAVITY_MPS2 = 9.80665;
const float CRANK_ARM_LENGTH_M = 0.170;            // adjust if final crank length differs
const float HX711_COUNTS_PER_NEWTON_DEFAULT = 851.6;
const float DEFAULT_CRANK_AXIS_X = -0.074;         // measured in experiment 3.2
const float DEFAULT_CRANK_AXIS_Y = 0.995;
const float DEFAULT_CRANK_AXIS_Z = 0.065;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
HX711 hx711;

BLEServer *pServer = nullptr;
BLECharacteristic *pTelemetryChar = nullptr;
BLECharacteristic *pAppStatusChar = nullptr;
BLE2902 *pTelemetry2902 = nullptr;

ButtonInput buttons[] = {
  {"B1", 25, 659, false, false, 0, 0, false},
  {"B2", 26, 784, false, false, 0, 0, false},
  {"B3", 27, 988, false, false, 0, 0, false},
  {"B4", 14, 1319, false, false, 0, 0, false},
};
const uint8_t BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

DisplayScreen screen = SCREEN_LIVE;
uint8_t mpuAddress = 0;
bool oledOk = false;
bool mpuOk = false;
bool hxOk = false;
bool hxTared = false;
bool biasReady = false;
bool axisReady = false;
bool appGpsOk = false;
bool appRecording = false;

unsigned long lastSampleMs = 0;
unsigned long lastHxMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastTelemetryMs = 0;
unsigned long lastAppStatusMs = 0;
unsigned long buzzerOffAtMs = 0;
unsigned long statusUntilMs = 0;
const char *statusLine = "READY";

GyroDps gyro = {0.0, 0.0, 0.0};
GyroDps bias = {0.0, 0.0, 0.0};
GyroDps uCrank = {DEFAULT_CRANK_AXIS_X, DEFAULT_CRANK_AXIS_Y, DEFAULT_CRANK_AXIS_Z};

long hxRaw = 0;
long hxZeroRaw = 0;
long hxDeltaRaw = 0;
uint32_t hxSampleCount = 0;
unsigned long lastHxReadMs = 0;
float hxCountsPerNewton = HX711_COUNTS_PER_NEWTON_DEFAULT;
float forceN = 0.0;
float forceKg = 0.0;
float torqueNm = 0.0;
float omegaRadS = 0.0;
float cadenceRpm = 0.0;
float powerW = 0.0;

uint32_t packetSeq = 0;
uint32_t lastNotifiedSeq = 0;

bool hasActiveClient() {
  return pServer != nullptr && pServer->getConnectedCount() > 0;
}

bool notificationsEnabled() {
  return pTelemetry2902 != nullptr && pTelemetry2902->getNotifications();
}

void setStatus(const char *text, unsigned long durationMs = 1500) {
  statusLine = text;
  statusUntilMs = millis() + durationMs;
}

void playTone(uint16_t frequency, unsigned long durationMs) {
  tone(BUZZER_PIN, frequency, durationMs);
  buzzerOffAtMs = millis() + durationMs + 5;
}

void playSuccessTone() {
  tone(BUZZER_PIN, 988, 80);
  delay(95);
  tone(BUZZER_PIN, 1319, 110);
  buzzerOffAtMs = millis() + 120;
}

void playErrorTone() {
  tone(BUZZER_PIN, 220, 180);
  buzzerOffAtMs = millis() + 190;
}

void playStartupTone() {
  tone(BUZZER_PIN, 784, 70);
  delay(85);
  tone(BUZZER_PIN, 988, 70);
  delay(85);
  tone(BUZZER_PIN, 1319, 100);
  buzzerOffAtMs = millis() + 110;
}

void updateBuzzer() {
  if (buzzerOffAtMs != 0 && millis() >= buzzerOffAtMs) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOffAtMs = 0;
  }
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
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
    return false;
  }

  uint8_t whoAmI = 0;
  if (!readRegister(mpuAddress, REG_WHO_AM_I, whoAmI)) {
    Serial.println("Failed to read MPU6050 WHO_AM_I");
    return false;
  }

  if (!writeRegister(mpuAddress, REG_PWR_MGMT_1, 0x01)) {
    return false;
  }
  delay(100);

  writeRegister(mpuAddress, REG_SMPLRT_DIV, 0x13);   // 50 Hz
  writeRegister(mpuAddress, REG_CONFIG, 0x03);
  writeRegister(mpuAddress, REG_GYRO_CONFIG, 0x18);  // +/-2000 dps
  writeRegister(mpuAddress, REG_ACCEL_CONFIG, 0x00);

  Serial.print("MPU6050 initialized at 0x");
  Serial.println(mpuAddress, HEX);
  Serial.print("WHO_AM_I=0x");
  Serial.println(whoAmI, HEX);
  return true;
}

bool readGyroDps(GyroDps &out) {
  if (mpuAddress == 0) {
    return false;
  }

  uint8_t data[14];
  if (!readRegisters(mpuAddress, REG_ACCEL_XOUT_H, data, sizeof(data))) {
    return false;
  }

  out.x = combineInt16(data[8], data[9]) / 16.4;
  out.y = combineInt16(data[10], data[11]) / 16.4;
  out.z = combineInt16(data[12], data[13]) / 16.4;
  return true;
}

float magnitude3(float x, float y, float z) {
  return sqrt((x * x) + (y * y) + (z * z));
}

void drawStatusScreen(const char *title, const char *line1, const char *line2) {
  if (!oledOk) {
    return;
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);
  display.setCursor(0, 20);
  display.print(line1);
  display.setCursor(0, 34);
  display.print(line2);
  display.display();
}

void calibrateGyroBias() {
  if (!mpuOk) {
    setStatus("GY NO");
    playErrorTone();
    return;
  }

  drawStatusScreen("GYRO BIAS", "Keep crank still", "Calibrating...");
  double sx = 0.0;
  double sy = 0.0;
  double sz = 0.0;
  uint16_t okCount = 0;

  for (int i = 0; i < GYRO_BIAS_SAMPLES; i++) {
    GyroDps sample;
    if (readGyroDps(sample)) {
      sx += sample.x;
      sy += sample.y;
      sz += sample.z;
      okCount++;
    }
    delay(20);
  }

  if (okCount == 0) {
    biasReady = false;
    setStatus("BIAS FAIL");
    playErrorTone();
    Serial.println("GYRO_BIAS_FAIL");
    return;
  }

  bias.x = sx / okCount;
  bias.y = sy / okCount;
  bias.z = sz / okCount;
  biasReady = true;
  setStatus("BIAS OK");
  playSuccessTone();

  Serial.print("GYRO_BIAS_OK,x:");
  Serial.print(bias.x, 4);
  Serial.print(",y:");
  Serial.print(bias.y, 4);
  Serial.print(",z:");
  Serial.println(bias.z, 4);
}

void calibrateCrankAxis() {
  if (!mpuOk || !biasReady) {
    setStatus("BIAS 1ST");
    playErrorTone();
    return;
  }

  drawStatusScreen("AXIS CAL", "Spin crank steady", "10 seconds...");
  double sx = 0.0;
  double sy = 0.0;
  double sz = 0.0;
  uint16_t validCount = 0;

  for (int i = 0; i < AXIS_CAL_SAMPLES; i++) {
    GyroDps sample;
    if (readGyroDps(sample)) {
      const float gx = sample.x - bias.x;
      const float gy = sample.y - bias.y;
      const float gz = sample.z - bias.z;
      const float mag = magnitude3(gx, gy, gz);
      if (mag >= AXIS_MIN_OMEGA_DPS) {
        sx += gx;
        sy += gy;
        sz += gz;
        validCount++;
      }
    }
    delay(20);
  }

  if (validCount < AXIS_MIN_VALID_SAMPLES) {
    axisReady = false;
    setStatus("AXIS FAIL");
    playErrorTone();
    Serial.print("AXIS_CAL_FAIL,valid:");
    Serial.println(validCount);
    return;
  }

  const float mx = sx / validCount;
  const float my = sy / validCount;
  const float mz = sz / validCount;
  const float norm = magnitude3(mx, my, mz);
  if (norm < AXIS_MIN_OMEGA_DPS) {
    axisReady = false;
    setStatus("AXIS FAIL");
    playErrorTone();
    Serial.println("AXIS_CAL_FAIL,norm_low");
    return;
  }

  uCrank.x = mx / norm;
  uCrank.y = my / norm;
  uCrank.z = mz / norm;
  axisReady = true;
  setStatus("AXIS OK");
  playSuccessTone();

  Serial.print("AXIS_CAL_OK,n:");
  Serial.print(validCount);
  Serial.print(",ux:");
  Serial.print(uCrank.x, 5);
  Serial.print(",uy:");
  Serial.print(uCrank.y, 5);
  Serial.print(",uz:");
  Serial.println(uCrank.z, 5);
}

bool waitForHxReady(unsigned long timeoutMs) {
  const unsigned long startMs = millis();
  while (!hx711.is_ready()) {
    if (millis() - startMs >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool readHxRaw(long &rawValue) {
  if (!waitForHxReady(HX_READY_TIMEOUT_MS)) {
    return false;
  }
  rawValue = hx711.read();
  return true;
}

bool readHxAverage(int count, long &averageRaw) {
  long sum = 0;
  for (int i = 0; i < count; i++) {
    long raw = 0;
    if (!readHxRaw(raw)) {
      return false;
    }
    sum += raw;
  }
  averageRaw = sum / count;
  return true;
}

void tareHx711() {
  drawStatusScreen("HX TARE", "Remove load", "Keep still...");
  long averageRaw = 0;
  if (!readHxAverage(HX_TARE_SAMPLES, averageRaw)) {
    hxOk = false;
    hxTared = false;
    setStatus("TARE FAIL");
    playErrorTone();
    Serial.println("HX_TARE_FAIL");
    return;
  }

  hxZeroRaw = averageRaw;
  hxRaw = averageRaw;
  hxDeltaRaw = 0;
  hxSampleCount++;
  lastHxReadMs = millis();
  forceN = 0.0;
  forceKg = 0.0;
  torqueNm = 0.0;
  powerW = 0.0;
  hxOk = true;
  hxTared = true;
  setStatus("TARE OK");
  playSuccessTone();

  Serial.print("HX_TARE_OK,zero_raw:");
  Serial.println(hxZeroRaw);
}

void readHxSample() {
  if (!hx711.is_ready()) {
    hxOk = false;
    return;
  }

  hxRaw = hx711.read();
  hxDeltaRaw = hxTared ? hxRaw - hxZeroRaw : hxRaw;
  forceN = hxTared ? hxDeltaRaw / hxCountsPerNewton : 0.0;
  if (forceN < 0.0) {
    forceN = -forceN;
  }
  forceKg = forceN / GRAVITY_MPS2;
  hxOk = true;
  hxSampleCount++;
  lastHxReadMs = millis();
}

void updateDerivedMetrics() {
  GyroDps sample;
  if (!readGyroDps(sample)) {
    mpuOk = false;
    cadenceRpm = 0.0;
    omegaRadS = 0.0;
    powerW = 0.0;
    return;
  }

  mpuOk = true;
  gyro = sample;
  const float gx = sample.x - (biasReady ? bias.x : 0.0);
  const float gy = sample.y - (biasReady ? bias.y : 0.0);
  const float gz = sample.z - (biasReady ? bias.z : 0.0);

  float omegaDps = 0.0;
  omegaDps = fabs((gx * uCrank.x) + (gy * uCrank.y) + (gz * uCrank.z));

  cadenceRpm = omegaDps * 60.0 / 360.0;
  omegaRadS = omegaDps * DPS_TO_RAD_S;
  torqueNm = fabs(forceN) * CRANK_ARM_LENGTH_M;
  powerW = torqueNm * omegaRadS;
}

bool calibrationOk() {
  return mpuOk && hxOk && hxTared && biasReady;
}

void notifyPacket(const char *packet) {
  if (!notificationsEnabled() || pTelemetryChar == nullptr) {
    return;
  }
  pTelemetryChar->setValue((uint8_t *)packet, strlen(packet));
  pTelemetryChar->notify();
}

void sendEvent(const char *eventName) {
  char packet[160];
  const uint32_t seq = ++packetSeq;
  snprintf(
    packet,
    sizeof(packet),
    "{\"v\":1,\"type\":\"event\",\"seq\":%lu,\"tMs\":%lu,\"event\":\"%s\"}",
    (unsigned long)seq,
    (unsigned long)millis(),
    eventName
  );
  notifyPacket(packet);
  Serial.print("[EV] ");
  Serial.println(packet);
}

void sendTelemetry() {
  const uint32_t seq = ++packetSeq;
  char packet[240];
  snprintf(
    packet,
    sizeof(packet),
    "{\"v\":1,\"type\":\"telemetry\",\"seq\":%lu,\"tMs\":%lu,"
    "\"powerW\":%.1f,\"cadenceRpm\":%.1f,\"forceN\":%.2f,"
    "\"torqueNm\":%.2f,\"omegaRadS\":%.3f,\"hxRaw\":%ld,"
    "\"hxTared\":%s,\"imuOk\":%s,\"hxOk\":%s,\"calOk\":%s}",
    (unsigned long)seq,
    (unsigned long)millis(),
    powerW,
    cadenceRpm,
    forceN,
    torqueNm,
    omegaRadS,
    hxRaw,
    hxTared ? "true" : "false",
    mpuOk ? "true" : "false",
    hxOk ? "true" : "false",
    calibrationOk() ? "true" : "false"
  );

  notifyPacket(packet);
  lastNotifiedSeq = seq;
  Serial.print("[TX] ");
  Serial.println(packet);
}

void setBoolFromJsonLike(String payload, const char *key, bool &target) {
  const String trueNeedle = String("\"") + key + "\":true";
  const String falseNeedle = String("\"") + key + "\":false";
  if (payload.indexOf(trueNeedle) >= 0) {
    target = true;
  } else if (payload.indexOf(falseNeedle) >= 0) {
    target = false;
  }
}

void handleAppPayload(String payload) {
  payload.trim();
  if (payload.length() == 0) {
    return;
  }

  lastAppStatusMs = millis();
  setBoolFromJsonLike(payload, "gpsOk", appGpsOk);
  setBoolFromJsonLike(payload, "recording", appRecording);
  setBoolFromJsonLike(payload, "rec", appRecording);

  if (payload.indexOf("tare") >= 0) {
    tareHx711();
    sendEvent("tare_ok");
  } else if (payload.indexOf("gyroBias") >= 0 || payload.indexOf("bias") >= 0) {
    calibrateGyroBias();
    sendEvent(biasReady ? "gyro_bias_ok" : "gyro_bias_fail");
  } else if (payload.indexOf("axisCal") >= 0 || payload.indexOf("axis") >= 0) {
    calibrateCrankAxis();
    sendEvent(axisReady ? "axis_cal_ok" : "axis_cal_fail");
  }

  Serial.print("[APP] ");
  Serial.println(payload);
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    Serial.println("[BLE] Client connected");
    setStatus("APP CONN");
    playTone(1568, 80);
  }

  void onDisconnect(BLEServer *server) override {
    Serial.println("[BLE] Client disconnected");
    appGpsOk = false;
    appRecording = false;
    setStatus("APP LOST");
    playTone(220, 160);
    delay(300);
    server->startAdvertising();
  }
};

class AppStatusCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();
    if (value.length() == 0) {
      return;
    }
    handleAppPayload(value);
  }
};

void initializeBle() {
  BLEDevice::init(DEVICE_NAME);
  BLEDevice::setMTU(247);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *service = pServer->createService(SERVICE_UUID);

  pTelemetryChar = service->createCharacteristic(
    TELEMETRY_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTelemetry2902 = new BLE2902();
  pTelemetryChar->addDescriptor(pTelemetry2902);

  pAppStatusChar = service->createCharacteristic(
    APP_STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pAppStatusChar->setCallbacks(new AppStatusCallbacks());

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.print("[BLE] Advertising as ");
  Serial.println(DEVICE_NAME);
  Serial.print("[BLE] Telemetry characteristic: ");
  Serial.println(TELEMETRY_CHAR_UUID);
  Serial.print("[BLE] App status write characteristic: ");
  Serial.println(APP_STATUS_CHAR_UUID);
}

void drawBluetoothIcon(uint8_t x, uint8_t y) {
  if (!hasActiveClient() && ((millis() / 500) % 2 == 0)) {
    return;
  }

  display.drawLine(x + 3, y, x + 3, y + 10, SH110X_WHITE);
  display.drawLine(x + 3, y, x + 7, y + 3, SH110X_WHITE);
  display.drawLine(x + 7, y + 3, x + 3, y + 5, SH110X_WHITE);
  display.drawLine(x + 3, y + 5, x + 7, y + 8, SH110X_WHITE);
  display.drawLine(x + 7, y + 8, x + 3, y + 10, SH110X_WHITE);
  display.drawLine(x, y + 2, x + 3, y + 5, SH110X_WHITE);
  display.drawLine(x, y + 8, x + 3, y + 5, SH110X_WHITE);

  if (hasActiveClient() && !notificationsEnabled()) {
    display.drawPixel(x + 9, y + 2, SH110X_WHITE);
    display.drawPixel(x + 9, y + 3, SH110X_WHITE);
    display.drawPixel(x + 9, y + 5, SH110X_WHITE);
  }
}

void drawHeader(const char *title) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);

  drawBluetoothIcon(72, 0);

  display.setCursor(86, 0);
  display.print(appGpsOk ? "GPS" : "---");

  display.setCursor(108, 0);
  if (appRecording) {
    display.print("REC");
  } else {
    display.print("---");
  }

  display.drawLine(0, 11, 127, 11, SH110X_WHITE);
}

void printCompactFloat(float value, uint8_t decimals) {
  if (fabs(value) >= 999.0) {
    display.print((int)value);
  } else {
    display.print(value, decimals);
  }
}

void drawLiveScreen() {
  drawHeader("LIVE");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print((int)round(powerW));
  display.print(" W");

  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print((int)round(cadenceRpm));
  display.print("rpm ");
  if (hxTared) {
    display.print((int)round(forceN));
    display.print("N");
  } else {
    display.print("R");
    display.print(hxRaw);
  }

  display.setCursor(0, 49);
  display.print("TQ ");
  printCompactFloat(torqueNm, 1);
  display.print("Nm");

  display.setCursor(0, 58);
  if (millis() < statusUntilMs) {
    display.print(statusLine);
  } else {
    if (hxOk && hxTared) {
      display.print("HX OK");
    } else if (hxOk) {
      display.print("HX RAW");
    } else {
      display.print("HX WAIT");
    }
    if (!biasReady) {
      display.print(" RAW");
    } else if (axisReady) {
      display.print(" CAL");
    } else {
      display.print(" AX DEF");
    }
  }
  display.display();
}

void drawSensorScreen() {
  drawHeader("SENSOR");
  display.setCursor(0, 15);
  display.print("IMU ");
  display.print(mpuOk ? "OK " : "NO ");
  display.print((int)round(cadenceRpm));
  display.print("rpm");

  display.setCursor(0, 27);
  display.print("HX  ");
  display.print(hxOk ? "OK " : "NO ");
  if (hxTared) {
    printCompactFloat(forceN, 1);
    display.print("N");
  } else {
    display.print("raw");
  }

  display.setCursor(0, 39);
  display.print("RAW ");
  display.print(hxRaw);

  display.setCursor(0, 51);
  display.print("B3 bias B4 tare");
  display.display();
}

void drawHxScreen() {
  drawHeader("HX");
  display.setCursor(0, 14);
  display.print(hxTared ? "TARE OK" : "NO TARE");
  display.print("  c/N ");
  display.print(hxCountsPerNewton, 0);

  display.setCursor(0, 26);
  if (hxTared) {
    display.print("F ");
    printCompactFloat(forceN, 2);
    display.print("N");
  } else {
    display.print("RAW ");
    display.print(hxRaw);
  }

  display.setCursor(0, 38);
  if (hxTared) {
    display.print("M ");
    printCompactFloat(forceKg, 3);
    display.print("kg");
  } else {
    display.print("TARE NEEDED");
  }

  display.setCursor(0, 50);
  if (hxTared) {
    display.print("dR ");
    display.print(hxDeltaRaw);
  } else {
    display.print("Hold B4 tare");
  }
  display.display();
}

void drawBleScreen() {
  drawHeader("BLE");
  display.setCursor(0, 14);
  display.print(hasActiveClient() ? "APP CONN" : "ADV ON");

  display.setCursor(0, 26);
  display.print("NOTIFY ");
  display.print(notificationsEnabled() ? "ON" : "--");

  display.setCursor(0, 38);
  display.print("SEQ ");
  display.print(lastNotifiedSeq);

  display.setCursor(0, 50);
  display.print("CMD ");
  display.print(APP_STATUS_CHAR_UUID + 31);
  display.display();
}

void drawSysScreen() {
  drawHeader("SYS");
  display.setCursor(0, 14);
  display.print("up ");
  display.print(millis() / 1000);
  display.print("s");

  display.setCursor(0, 26);
  display.print("heap ");
  display.print(ESP.getFreeHeap());

  display.setCursor(0, 38);
  display.print("bias ");
  display.print(biasReady ? "OK" : "--");
  display.print(" axis ");
  display.print(axisReady ? "OK" : "DEF");

  display.setCursor(0, 50);
  display.print("B3 bias B4 axis");
  display.display();
}

void updateDisplay() {
  if (!oledOk) {
    return;
  }

  switch (screen) {
    case SCREEN_LIVE:
      drawLiveScreen();
      break;
    case SCREEN_SENSOR:
      drawSensorScreen();
      break;
    case SCREEN_HX:
      drawHxScreen();
      break;
    case SCREEN_BLE:
      drawBleScreen();
      break;
    case SCREEN_SYS:
      drawSysScreen();
      break;
  }
}

bool readPressed(uint8_t pin) {
  return digitalRead(pin) == BUTTON_PRESSED_LEVEL;
}

void nextScreen() {
  screen = (screen == SCREEN_SYS) ? SCREEN_LIVE : (DisplayScreen)(screen + 1);
}

void previousScreen() {
  screen = (screen == SCREEN_LIVE) ? SCREEN_SYS : (DisplayScreen)(screen - 1);
}

void handleShortPress(uint8_t index) {
  if (index == 0) {
    previousScreen();
  } else if (index == 1) {
    nextScreen();
  }
}

void handleLongPress(uint8_t index) {
  if (index == 2 && (screen == SCREEN_SENSOR || screen == SCREEN_SYS)) {
    calibrateGyroBias();
    sendEvent(biasReady ? "gyro_bias_ok" : "gyro_bias_fail");
  } else if (index == 3) {
    if (screen == SCREEN_LIVE || screen == SCREEN_SYS) {
      calibrateCrankAxis();
      sendEvent(axisReady ? "axis_cal_ok" : "axis_cal_fail");
    } else {
      tareHx711();
      sendEvent(hxTared ? "tare_ok" : "tare_fail");
    }
  }
}

void updateButtons() {
  const unsigned long now = millis();
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
        playTone(button.toneHz, 70);
      } else {
        if (!button.longPressHandled) {
          handleShortPress(i);
        }
        button.pressedAtMs = 0;
      }
    }

    if (button.stablePressed &&
        !button.longPressHandled &&
        (now - button.pressedAtMs) >= LONG_PRESS_MS) {
      button.longPressHandled = true;
      handleLongPress(i);
    }
  }
}

void updateAppStatusTimeout() {
  if (lastAppStatusMs != 0 && (millis() - lastAppStatusMs) > APP_STATUS_TIMEOUT_MS) {
    appGpsOk = false;
    appRecording = false;
  }
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

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  oledOk = display.begin(OLED_I2C_ADDRESS, true);
  if (oledOk) {
    display.setRotation(0);
    drawStatusScreen("WATTMETER", "Booting...", "Full system");
  }

  hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN);
  mpuOk = initializeMpu6050();
  initializeBle();
  playStartupTone();

  Serial.println();
  Serial.println("Smart Watt Meter full-system firmware started");
  Serial.println("GPIO: HX711 DOUT32 SCK33, I2C SDA21 SCL22, buzzer13, buttons 25/26/27/14");
  Serial.print("HX711 gain: ");
  Serial.println(HX711_GAIN);
  Serial.print("counts_per_N: ");
  Serial.println(hxCountsPerNewton, 3);
  Serial.print("crank_length_m: ");
  Serial.println(CRANK_ARM_LENGTH_M, 3);

  if (oledOk) {
    drawStatusScreen("SELF CHECK", mpuOk ? "IMU OK" : "IMU NO", "BLE ADV ON");
  }
  delay(800);
  setStatus("READY");
}

void loop() {
  const unsigned long now = millis();

  updateBuzzer();
  updateButtons();
  updateAppStatusTimeout();

  if ((now - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    updateDerivedMetrics();
  }

  if ((now - lastHxMs) >= HX_INTERVAL_MS) {
    lastHxMs = now;
    readHxSample();
  }

  if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryMs = now;
    if (hasActiveClient() && notificationsEnabled()) {
      sendTelemetry();
    }
  }

  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }
}
