/*
 * smart_watt_meter_mock.ino
 * ESP32 Mock Firmware — Smart Watt-Meter (FRA503)
 *
 * Simulates strain gauge + MPU6050 readings and sends telemetry
 * to the Flutter app via BLE GATT notifications at 1 Hz.
 *
 * BLE Service UUID  : 12345678-1234-1234-1234-123456789012
 * Telemetry Char UUID: 12345678-1234-1234-1234-123456789013
 * Packet format (JSON): {"power":245,"cadence":85,"speed":28.5,"elapsedSec":42}
 *
 * Flash target: ESP32 (any variant)
 * Arduino IDE board: "ESP32 Dev Module"
 * Baud: 115200
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// ── BLE identifiers ───────────────────────────────────────────
#define DEVICE_NAME          "ESP32_PowerMeter_01"
#define SERVICE_UUID         "12345678-1234-1234-1234-123456789012"
#define TELEMETRY_CHAR_UUID  "12345678-1234-1234-1234-123456789013"

// ── Timing ───────────────────────────────────────────────────
#define NOTIFY_INTERVAL_MS   1000   // 1 Hz telemetry

// ── Globals ──────────────────────────────────────────────────
BLEServer*          pServer        = nullptr;
BLECharacteristic*  pTelemetryChar = nullptr;
BLE2902*            pTelemetry2902 = nullptr;
bool deviceConnected    = false;
bool oldDeviceConnected = false;
uint32_t elapsedSec     = 0;
unsigned long lastNotifyMs = 0;
unsigned long lastConnectionDebugMs = 0;
unsigned long notifyDisabledSinceMs = 0;

bool hasActiveClient() {
  return pServer != nullptr && pServer->getConnectedCount() > 0;
}

bool notificationsEnabled() {
  return pTelemetry2902 != nullptr && pTelemetry2902->getNotifications();
}

// ── BLE connection callbacks ──────────────────────────────────
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    elapsedSec = 0;
    Serial.println("[BLE] Client connected — session timer reset");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
  }
};

// ── Mock sensor data ──────────────────────────────────────────
// Mimics a real cycling session: sinusoidal power base + random noise.
// Formula mirrors actual firmware: P = torque × angular_velocity
//   torque      → strain gauge (HX711) reading
//   ang_velocity→ MPU6050 gyroscope Z-axis
// For mock: both are approximated by a sine wave.

int mockPower() {
  // 180 W base, ±60 W sine (30 s period), ±20 W noise
  float base  = 180.0f + 60.0f * sinf((float)elapsedSec * 2.0f * M_PI / 30.0f);
  float noise = (float)(random(-20, 21));
  return (int)max(30.0f, base + noise);
}

int mockCadence() {
  // 82 RPM base ± 5 RPM
  return 82 + random(-5, 6);
}

float mockSpeed() {
  // 28.0 km/h ± 2.0 km/h
  return 28.0f + (float)random(-20, 21) / 10.0f;
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== Smart Watt-Meter Mock Firmware ===");

  randomSeed(analogRead(0));

  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pTelemetryChar = pService->createCharacteristic(
    TELEMETRY_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTelemetry2902 = new BLE2902();
  pTelemetryChar->addDescriptor(pTelemetry2902);

  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.printf("[BLE] Advertising as \"%s\"\n", DEVICE_NAME);
  Serial.println("[BLE] Waiting for Flutter app to connect...");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();
  deviceConnected = hasActiveClient();
  bool notifyEnabled = notificationsEnabled();

  if (deviceConnected && !notifyEnabled) {
    if (notifyDisabledSinceMs == 0) {
      notifyDisabledSinceMs = now;
      Serial.printf(
        "[BLE] Notifications disabled while connected; connId=%u count=%lu\n",
        pServer->getConnId(),
        (unsigned long)pServer->getConnectedCount()
      );
    } else if (now - notifyDisabledSinceMs >= 2000) {
      Serial.printf(
        "[BLE] Forcing disconnect of stale client; connId=%u count=%lu\n",
        pServer->getConnId(),
        (unsigned long)pServer->getConnectedCount()
      );
      pServer->disconnect(pServer->getConnId());
      notifyDisabledSinceMs = now;
    }
  } else {
    notifyDisabledSinceMs = 0;
  }

  if (deviceConnected && (now - lastConnectionDebugMs >= 5000)) {
    lastConnectionDebugMs = now;
    Serial.printf(
      "[DBG] still connected, connectedCount=%lu notify=%s elapsedSec=%lu\n",
      (unsigned long)pServer->getConnectedCount(),
      notifyEnabled ? "on" : "off",
      (unsigned long)elapsedSec
    );
  }

  if (deviceConnected && notifyEnabled && (now - lastNotifyMs >= NOTIFY_INTERVAL_MS)) {
    lastNotifyMs = now;

    int   power   = mockPower();
    int   cadence = mockCadence();
    float speed   = mockSpeed();

    // JSON packet — must match the Flutter parser exactly
    char packet[128];
    snprintf(packet, sizeof(packet),
      "{\"power\":%d,\"cadence\":%d,\"speed\":%.1f,\"elapsedSec\":%lu}",
      power, cadence, speed, (unsigned long)elapsedSec
    );

    pTelemetryChar->setValue((uint8_t*)packet, strlen(packet));
    pTelemetryChar->notify();

    Serial.printf("[TX] %s\n", packet);
    elapsedSec++;
  }

  // Handle disconnect → restart advertising
  if (!deviceConnected && oldDeviceConnected) {
    Serial.printf("[BLE] Connected count = %lu\n", (unsigned long)pServer->getConnectedCount());
    delay(500);
    pServer->startAdvertising();
    Serial.println("[BLE] Restarting advertising...");
    oldDeviceConnected = false;
    lastConnectionDebugMs = 0;
  }

  if (deviceConnected && !oldDeviceConnected) {
    Serial.printf("[BLE] Connected count = %lu\n", (unsigned long)pServer->getConnectedCount());
    oldDeviceConnected = true;
    lastConnectionDebugMs = 0;
  }
}
