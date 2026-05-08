// Experiment 3.2 firmware: IMU gyroscope cadence validation.
//
// Purpose:
//   Standalone OLED-guided workflow for measuring crank cadence from an
//   MPU6050 gyroscope without keeping USB serial connected while the crank
//   is spinning.
//
// Hardware:
//   ESP32 Dev Module / NodeMCU-32S
//   Shared I2C: SDA GPIO21, SCL GPIO22
//   OLED SH1106 1.3 inch I2C: address 0x3C
//   MPU6050: address 0x68 or 0x69
//   Passive buzzer: GPIO13
//   Active-high buttons with INPUT_PULLDOWN:
//     BTN1 GPIO25: START / RETRY
//     BTN2 GPIO26: NEXT
//     BTN3 GPIO27: previous trial
//     BTN4 GPIO14: next trial

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <math.h>

enum AppState {
  STATE_BIAS_IDLE,
  STATE_BIAS_RUNNING,
  STATE_BIAS_RESULT,
  STATE_AXIS_IDLE,
  STATE_AXIS_RUNNING,
  STATE_AXIS_RESULT,
  STATE_TRIAL_IDLE,
  STATE_TRIAL_RUNNING,
  STATE_TRIAL_RESULT
};

struct GyroDps {
  float x;
  float y;
  float z;
};

struct ButtonInput {
  const char *name;
  uint8_t pin;
  unsigned int toneHz;
  bool stablePressed;
  bool lastRawPressed;
  unsigned long lastRawChangeMs;
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
const unsigned long DISPLAY_INTERVAL_MS = 200;
const unsigned long SAMPLE_INTERVAL_MS = 20;     // 50 Hz
const unsigned long BIAS_DURATION_MS = 5000;     // 5 s
const unsigned long AXIS_DURATION_MS = 10000;    // 10 s
const unsigned long TRIAL_DURATION_MS = 10000;   // 10 s
const float AXIS_MIN_OMEGA_DPS = 30.0;
const uint16_t AXIS_MIN_VALID_SAMPLES = 350;
const float GYRO_LSB_PER_DPS = 16.4;             // MPU6050 +/-2000 dps

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

ButtonInput buttons[] = {
  {"BTN1", 25, 659, false, false, 0},
  {"BTN2", 26, 784, false, false, 0},
  {"BTN3", 27, 988, false, false, 0},
  {"BTN4", 14, 1319, false, false, 0},
};
const uint8_t BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

AppState state = STATE_BIAS_IDLE;
uint8_t mpuAddress = 0;
bool oledOk = false;
bool mpuOk = false;
bool biasReady = false;
bool axisReady = false;

GyroDps gyro = {0.0, 0.0, 0.0};
GyroDps bias = {0.0, 0.0, 0.0};
GyroDps uCrank = {0.0, 0.0, 0.0};

unsigned long stateStartMs = 0;
unsigned long lastSampleMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long buzzerOffAtMs = 0;

double sumX = 0.0;
double sumY = 0.0;
double sumZ = 0.0;
uint32_t sampleCount = 0;
uint32_t validSampleCount = 0;
float axisNorm = 0.0;
bool axisQualityGood = false;

uint16_t trialNumber = 1;
double rpmSum = 0.0;
float rpmAvg = 0.0;
float rpmLive = 0.0;
float rpmMin = 99999.0;
float rpmMax = 0.0;
uint32_t trialSampleCount = 0;

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
    Serial.println("Failed to read WHO_AM_I");
    return false;
  }

  if (!writeRegister(mpuAddress, REG_PWR_MGMT_1, 0x01)) {
    return false;
  }
  delay(100);

  writeRegister(mpuAddress, REG_SMPLRT_DIV, 0x13);   // 1 kHz / (19 + 1) = 50 Hz
  writeRegister(mpuAddress, REG_CONFIG, 0x03);       // DLPF around 44 Hz
  writeRegister(mpuAddress, REG_GYRO_CONFIG, 0x18);  // +/-2000 deg/s
  writeRegister(mpuAddress, REG_ACCEL_CONFIG, 0x00);

  Serial.print("MPU6050 initialized at 0x");
  Serial.println(mpuAddress, HEX);
  Serial.print("WHO_AM_I=0x");
  Serial.println(whoAmI, HEX);
  return true;
}

bool readGyroDps(GyroDps &out) {
  uint8_t data[14];
  if (!readRegisters(mpuAddress, REG_ACCEL_XOUT_H, data, sizeof(data))) {
    return false;
  }

  const int16_t rawGx = combineInt16(data[8], data[9]);
  const int16_t rawGy = combineInt16(data[10], data[11]);
  const int16_t rawGz = combineInt16(data[12], data[13]);

  out.x = rawGx / GYRO_LSB_PER_DPS;
  out.y = rawGy / GYRO_LSB_PER_DPS;
  out.z = rawGz / GYRO_LSB_PER_DPS;
  return true;
}

float magnitude3(float x, float y, float z) {
  return sqrt((x * x) + (y * y) + (z * z));
}

void resetAccumulator() {
  sumX = 0.0;
  sumY = 0.0;
  sumZ = 0.0;
  sampleCount = 0;
  validSampleCount = 0;
}

void resetTrial() {
  rpmSum = 0.0;
  rpmAvg = 0.0;
  rpmLive = 0.0;
  rpmMin = 99999.0;
  rpmMax = 0.0;
  trialSampleCount = 0;
}

void enterState(AppState newState) {
  state = newState;
  stateStartMs = millis();
  lastSampleMs = 0;

  if (newState == STATE_BIAS_RUNNING || newState == STATE_AXIS_RUNNING) {
    resetAccumulator();
  }
  if (newState == STATE_TRIAL_RUNNING) {
    resetTrial();
  }
}

void playTone(unsigned int frequency, unsigned long durationMs) {
  tone(BUZZER_PIN, frequency, durationMs);
  buzzerOffAtMs = millis() + durationMs + 5;
}

void playSuccessTone() {
  tone(BUZZER_PIN, 988, 80);
  delay(100);
  tone(BUZZER_PIN, 1319, 120);
  buzzerOffAtMs = millis() + 130;
}

void playErrorTone() {
  tone(BUZZER_PIN, 220, 180);
  buzzerOffAtMs = millis() + 190;
}

void updateBuzzer() {
  if (buzzerOffAtMs != 0 && millis() >= buzzerOffAtMs) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOffAtMs = 0;
  }
}

bool readPressed(uint8_t pin) {
  return digitalRead(pin) == BUTTON_PRESSED_LEVEL;
}

void handleButtonPress(uint8_t index) {
  playTone(buttons[index].toneHz, 70);

  if (index == 0) {  // BTN1
    if (state == STATE_BIAS_IDLE || state == STATE_BIAS_RESULT) {
      enterState(STATE_BIAS_RUNNING);
    } else if (state == STATE_AXIS_IDLE || state == STATE_AXIS_RESULT) {
      enterState(STATE_AXIS_RUNNING);
    } else if (state == STATE_TRIAL_IDLE || state == STATE_TRIAL_RESULT) {
      if (biasReady && axisReady) {
        enterState(STATE_TRIAL_RUNNING);
      } else {
        playErrorTone();
      }
    }
  } else if (index == 1) {  // BTN2
    if (state == STATE_BIAS_RESULT && biasReady) {
      enterState(STATE_AXIS_IDLE);
    } else if (state == STATE_AXIS_RESULT && axisReady) {
      enterState(STATE_TRIAL_IDLE);
    } else if (state == STATE_TRIAL_RESULT) {
      enterState(STATE_TRIAL_IDLE);
    }
  } else if (index == 2) {  // BTN3
    if ((state == STATE_TRIAL_IDLE || state == STATE_TRIAL_RESULT) && trialNumber > 1) {
      trialNumber--;
    }
  } else if (index == 3) {  // BTN4
    if (state == STATE_TRIAL_IDLE || state == STATE_TRIAL_RESULT) {
      trialNumber++;
      enterState(STATE_TRIAL_IDLE);
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
        handleButtonPress(i);
      }
    }
  }
}

void drawLineText(uint8_t line, const char *text) {
  display.setCursor(0, line * 10);
  display.print(text);
}

void drawHeader(const char *title) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);
}

void drawBiasIdle() {
  drawHeader("STEP 1/3 BIAS");
  drawLineText(2, "Keep crank still");
  drawLineText(3, "BTN1 START");
  drawLineText(5, "BTN2 after done");
  display.display();
}

void drawBiasRunning() {
  const float elapsedS = (millis() - stateStartMs) / 1000.0;
  drawHeader("BIAS CAL...");
  drawLineText(2, "Do not move");
  display.setCursor(0, 30);
  display.print("t ");
  display.print(elapsedS, 1);
  display.print("/5.0s");
  display.setCursor(0, 42);
  display.print("samples ");
  display.print(sampleCount);
  display.display();
}

void drawBiasResult() {
  drawHeader("BIAS DONE");
  display.setCursor(0, 16);
  display.print("x ");
  display.print(bias.x, 2);
  display.print(" dps");
  display.setCursor(0, 28);
  display.print("y ");
  display.print(bias.y, 2);
  display.print(" dps");
  display.setCursor(0, 40);
  display.print("z ");
  display.print(bias.z, 2);
  display.print(" dps");
  drawLineText(5, "BTN2 NEXT");
  display.display();
}

void drawAxisIdle() {
  drawHeader("STEP 2/3 AXIS");
  drawLineText(2, "Spin one direction");
  drawLineText(3, "Steady for 10s");
  drawLineText(5, "BTN1 START");
  display.display();
}

void drawAxisRunning() {
  const float elapsedS = (millis() - stateStartMs) / 1000.0;
  drawHeader("AXIS CAL...");
  drawLineText(2, "Spin steady");
  display.setCursor(0, 30);
  display.print("t ");
  display.print(elapsedS, 1);
  display.print("/10.0s");
  display.setCursor(0, 42);
  display.print("valid ");
  display.print(validSampleCount);
  display.print("/");
  display.print(sampleCount);
  display.display();
}

void drawAxisResult() {
  drawHeader(axisQualityGood ? "AXIS DONE GOOD" : "AXIS RETRY");
  display.setCursor(0, 14);
  display.print("ux: ");
  display.print(uCrank.x, 3);
  display.setCursor(0, 26);
  display.print("uy: ");
  display.print(uCrank.y, 3);
  display.setCursor(0, 38);
  display.print("uz: ");
  display.print(uCrank.z, 3);
  display.setCursor(0, 52);
  display.print("n:");
  display.print(validSampleCount);
  display.print(axisQualityGood ? " BTN2 NEXT" : " BTN1 RETRY");
  display.display();
}

void drawTrialIdle() {
  drawHeader("STEP 3/3 TRIAL");
  display.setCursor(0, 16);
  display.print("Trial ");
  display.print(trialNumber);
  display.setCursor(0, 28);
  display.print("Measure 10 sec");
  drawLineText(4, "BTN1 START");
  drawLineText(5, "BTN3/4 trial -/+");
  display.display();
}

void drawTrialRunning() {
  const float elapsedS = (millis() - stateStartMs) / 1000.0;
  drawHeader("TRIAL RUNNING");
  display.setCursor(0, 14);
  display.print("T");
  display.print(trialNumber);
  display.print(" t ");
  display.print(elapsedS, 1);
  display.print("/10s");
  display.setCursor(0, 28);
  display.print("RPM ");
  display.print(rpmLive, 1);
  display.setCursor(0, 40);
  display.print("AVG ");
  display.print(rpmAvg, 1);
  display.setCursor(0, 52);
  display.print("samples ");
  display.print(trialSampleCount);
  display.display();
}

void drawTrialResult() {
  drawHeader("TRIAL RESULT");
  display.setCursor(0, 14);
  display.print("T");
  display.print(trialNumber);
  display.print(" AVG ");
  display.print(rpmAvg, 1);
  display.setCursor(0, 26);
  display.print("MIN ");
  display.print(rpmMin, 1);
  display.print(" MAX ");
  display.print(rpmMax, 1);
  display.setCursor(0, 38);
  display.print("samples ");
  display.print(trialSampleCount);
  drawLineText(5, "B1 retry B4 next");
  display.display();
}

void drawStartupError() {
  drawHeader("INIT ERROR");
  display.setCursor(0, 16);
  display.print("OLED ");
  display.print(oledOk ? "OK" : "FAIL");
  display.setCursor(0, 28);
  display.print("MPU ");
  display.print(mpuOk ? "OK" : "FAIL");
  display.setCursor(0, 44);
  display.print("Check wiring/I2C");
  display.display();
}

void updateDisplay() {
  if (!oledOk) {
    return;
  }

  switch (state) {
    case STATE_BIAS_IDLE:
      drawBiasIdle();
      break;
    case STATE_BIAS_RUNNING:
      drawBiasRunning();
      break;
    case STATE_BIAS_RESULT:
      drawBiasResult();
      break;
    case STATE_AXIS_IDLE:
      drawAxisIdle();
      break;
    case STATE_AXIS_RUNNING:
      drawAxisRunning();
      break;
    case STATE_AXIS_RESULT:
      drawAxisResult();
      break;
    case STATE_TRIAL_IDLE:
      drawTrialIdle();
      break;
    case STATE_TRIAL_RUNNING:
      drawTrialRunning();
      break;
    case STATE_TRIAL_RESULT:
      drawTrialResult();
      break;
  }
}

void sampleBias() {
  if (!readGyroDps(gyro)) {
    return;
  }
  sumX += gyro.x;
  sumY += gyro.y;
  sumZ += gyro.z;
  sampleCount++;
}

void finishBias() {
  if (sampleCount == 0) {
    playErrorTone();
    enterState(STATE_BIAS_IDLE);
    return;
  }

  bias.x = sumX / sampleCount;
  bias.y = sumY / sampleCount;
  bias.z = sumZ / sampleCount;
  biasReady = true;
  playSuccessTone();

  Serial.println("BIAS_DONE");
  Serial.print("bias_x=");
  Serial.print(bias.x, 4);
  Serial.print(",bias_y=");
  Serial.print(bias.y, 4);
  Serial.print(",bias_z=");
  Serial.println(bias.z, 4);

  enterState(STATE_BIAS_RESULT);
}

void sampleAxis() {
  if (!readGyroDps(gyro)) {
    return;
  }

  const float gx = gyro.x - bias.x;
  const float gy = gyro.y - bias.y;
  const float gz = gyro.z - bias.z;
  const float mag = magnitude3(gx, gy, gz);

  sampleCount++;
  if (mag < AXIS_MIN_OMEGA_DPS) {
    return;
  }

  sumX += gx;
  sumY += gy;
  sumZ += gz;
  validSampleCount++;
}

void finishAxis() {
  if (validSampleCount == 0) {
    axisQualityGood = false;
    axisReady = false;
    playErrorTone();
    enterState(STATE_AXIS_RESULT);
    return;
  }

  const float meanX = sumX / validSampleCount;
  const float meanY = sumY / validSampleCount;
  const float meanZ = sumZ / validSampleCount;
  axisNorm = magnitude3(meanX, meanY, meanZ);

  axisQualityGood = validSampleCount >= AXIS_MIN_VALID_SAMPLES &&
                    axisNorm >= AXIS_MIN_OMEGA_DPS;
  axisReady = axisQualityGood;

  if (axisQualityGood) {
    uCrank.x = meanX / axisNorm;
    uCrank.y = meanY / axisNorm;
    uCrank.z = meanZ / axisNorm;
    playSuccessTone();
  } else {
    uCrank.x = 0.0;
    uCrank.y = 0.0;
    uCrank.z = 0.0;
    playErrorTone();
  }

  Serial.println("AXIS_DONE");
  Serial.print("valid_samples=");
  Serial.print(validSampleCount);
  Serial.print(",norm=");
  Serial.print(axisNorm, 4);
  Serial.print(",u_x=");
  Serial.print(uCrank.x, 5);
  Serial.print(",u_y=");
  Serial.print(uCrank.y, 5);
  Serial.print(",u_z=");
  Serial.println(uCrank.z, 5);

  enterState(STATE_AXIS_RESULT);
}

void sampleTrial() {
  if (!readGyroDps(gyro)) {
    return;
  }

  const float gx = gyro.x - bias.x;
  const float gy = gyro.y - bias.y;
  const float gz = gyro.z - bias.z;
  const float omegaCrankDps = (gx * uCrank.x) + (gy * uCrank.y) + (gz * uCrank.z);

  rpmLive = fabs(omegaCrankDps) * 60.0 / 360.0;
  rpmSum += rpmLive;
  trialSampleCount++;
  rpmAvg = rpmSum / trialSampleCount;

  if (rpmLive < rpmMin) {
    rpmMin = rpmLive;
  }
  if (rpmLive > rpmMax) {
    rpmMax = rpmLive;
  }
}

void finishTrial() {
  if (trialSampleCount == 0) {
    rpmMin = 0.0;
    rpmMax = 0.0;
    rpmAvg = 0.0;
    playErrorTone();
  } else {
    playSuccessTone();
  }

  Serial.println("TRIAL_DONE");
  Serial.print("trial=");
  Serial.print(trialNumber);
  Serial.print(",samples=");
  Serial.print(trialSampleCount);
  Serial.print(",rpm_avg=");
  Serial.print(rpmAvg, 3);
  Serial.print(",rpm_min=");
  Serial.print(rpmMin, 3);
  Serial.print(",rpm_max=");
  Serial.println(rpmMax, 3);

  enterState(STATE_TRIAL_RESULT);
}

void updateSampling() {
  const unsigned long now = millis();
  if ((now - lastSampleMs) < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs = now;

  if (state == STATE_BIAS_RUNNING) {
    sampleBias();
    if ((now - stateStartMs) >= BIAS_DURATION_MS) {
      finishBias();
    }
  } else if (state == STATE_AXIS_RUNNING) {
    sampleAxis();
    if ((now - stateStartMs) >= AXIS_DURATION_MS) {
      finishAxis();
    }
  } else if (state == STATE_TRIAL_RUNNING) {
    sampleTrial();
    if ((now - stateStartMs) >= TRIAL_DURATION_MS) {
      finishTrial();
    }
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
  }

  mpuOk = initializeMpu6050();

  Serial.println("Experiment 3.2 IMU cadence validation firmware");
  Serial.println("Gyro unit: deg/s, range: +/-2000 deg/s, sample rate: 50 Hz");
  Serial.println("BTN1 START/RETRY, BTN2 NEXT, BTN3 trial-, BTN4 trial+");

  if (oledOk && mpuOk) {
    playSuccessTone();
    enterState(STATE_BIAS_IDLE);
    updateDisplay();
  } else {
    playErrorTone();
    if (oledOk) {
      drawStartupError();
    }
  }
}

void loop() {
  updateBuzzer();
  updateButtons();

  if (!mpuOk) {
    delay(250);
    return;
  }

  updateSampling();

  const unsigned long now = millis();
  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }
}
