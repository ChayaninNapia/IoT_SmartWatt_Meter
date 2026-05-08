# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Smart Watt-Meter** (FRA503 — Technopreneurship in IoT Industry, KMUTT)
An IoT bicycle power measurement system. A crank-arm add-on device (ESP32 + strain gauge + IMU) measures pedaling power and transmits telemetry via BLE to a Flutter Android app, which records ride data locally and uploads to Firebase Cloud Firestore after each ride.

**Current phase**: Full-system hardware integration. ESP32 + HX711 + MPU6050 + OLED + 4 buttons + buzzer are assembled. The Flutter app now targets real full-system BLE telemetry (schema v2). Legacy mock firmware may still be used during transition but mock `speed` is deprecated.

---

## System Architecture (5 Layers)

```
[Device Layer]          Strain Gauge (×4) + HX711 ADC + MPU6050 (IMU/gyroscope)
                        → ESP32 (edge power estimation: torque × angular velocity)
        ↓
[Communication Layer]   BLE GAP + GATT (BLE Notifications, 1 Hz)
        ↓
[Application Layer]     Flutter Android App
                        ├── Phone GPS sensor → Flutter Geolocator Plugin
                        ├── BLE telemetry → power + cadence + timestamp
                        ├── Merge route + power by timestamp (Route Data Service)
                        ├── Ride Session Data (local buffer during ride)
                        └── Cloud Sync Service → Firebase Flutter SDK
        ↓
[Cloud Layer]           Firebase Cloud Firestore (via HTTPS / Firebase API, 4G/5G)
```

---

## Tech Stack

### Mobile App
- **Framework**: Flutter (Android target, SDK to be decided — use latest stable)
- **Language**: Dart
- **BLE**: `flutter_blue_plus` — scan, connect, subscribe to GATT notifications
- **GPS**: `geolocator` (Flutter Geolocator Plugin) — reads phone GPS, not hardware
- **Maps**: `flutter_map` + `latlong2` (OpenStreetMap, no API key required) — route rendering + power heatmap overlay
- **Firebase**: `firebase_core`, `cloud_firestore`, `firebase_auth`
- **Local storage**: buffer ride samples in memory during ride; persist to `shared_preferences` or `hive` if upload fails

### Firmware (ESP32)
- **Platform**: Arduino framework (`.ino`)
- **BLE**: Espressif BLE libraries (`BLEDevice`, `BLEServer`, `BLE2902`)
- **Sensors**: HX711 (strain gauge ADC), MPU6050 (gyroscope via I2C), OLED SH1106, 4 buttons (GPIO25/26/27/14), passive buzzer (GPIO13)
- **Full-system BLE packet** (v2): `powerW`, `cadenceRpm`, `forceN`, `torqueNm`, `omegaRadS`, `hxRaw`, `hxTared`, `imuOk`, `hxOk`, `calOk`, `seq`, `tMs`
- **Legacy mock packet** (backward-compat only): `{"power":245,"cadence":85,"speed":28.5,"elapsedSec":42}` — mock `speed` is deprecated and must not be displayed as real GPS speed

### Cloud
- **Database**: Firebase Cloud Firestore (security rules active — users can only access their own data)
- **Auth**: Firebase Anonymous Auth (configured and working)
- **SDK on app**: Firebase Flutter SDK
- **Project ID**: `iot-smart-watt-meter-83be9`
- **firebase_options.dart**: generated via `flutterfire configure` — do not edit manually

---

## Hardware Components

| Component | Role |
|-----------|------|
| ESP32 | Main MCU, BLE server, edge computation |
| Strain Gauge ×4 | Measure torque on crank arm (Wheatstone bridge) |
| HX711 | 24-bit ADC front-end for strain gauge |
| MPU6050 | Gyroscope + accelerometer (angular velocity for cadence) |
| Li-Po 7.4V + step-down 5V | Power supply |

---

## Firestore Schema (v2 — do not change without discussion)

See `cloud_detail.md` for full rules. Schema summary:

```
users/{uid}
  createdAt, lastActiveAt

users/{uid}/rides/{rideId}
  schemaVersion(=2), title, startTime, endTime, durationSec, distanceKm,
  avgPower, maxPower, avgCadence, maxCadence, avgSpeed, maxSpeed,
  avgForce, maxForce, avgTorque, maxTorque, avgOmega, maxOmega,
  calories, sampleCount, deviceSampleCount, gpsSampleCount, droppedPacketCount,
  calibrationOk, hxTared, imuOk, hxOk,
  uploadMode, syncStatus, createdAt

users/{uid}/rides/{rideId}/samples/{sampleId}
  timestamp, elapsedSec, powerW, cadenceRpm, gpsSpeed, lat, lng,
  seq, deviceTimeMs, forceN, torqueNm, omegaRadS,
  hxRaw, hxTared, imuOk, hxOk, calOk
```

**Rules**: 1 ride = 1 document. 1 timestep = 1 document. Upload only after ride ends.
New documents use `schemaVersion: 2`. v1 legacy fields (`power`, `cadence`, `speed`) may exist in old Firestore documents — `fromFirestore` handles them gracefully.

---

## BLE Protocol

**Device name**: `ESP32_PowerMeter_01`

**Custom Service UUID**: `12345678-1234-1234-1234-123456789012`

**Telemetry Characteristic UUID**: `12345678-1234-1234-1234-123456789013`
- Properties: READ + NOTIFY
- Interval: 1 Hz (every 1000 ms)
- Format: JSON string

**Full-system packet (v2 — current target)**:
```json
{
  "v": 1, "type": "telemetry",
  "seq": 128, "tMs": 128420,
  "powerW": 245.3, "cadenceRpm": 82.1,
  "forceN": 34.2, "torqueNm": 5.9, "omegaRadS": 8.6,
  "hxRaw": 29120, "hxTared": true, "imuOk": true, "hxOk": true, "calOk": true
}
```

**Legacy mock packet (backward-compat, deprecated)**:
```json
{"power":245,"cadence":85,"speed":28.5,"elapsedSec":42}
```
Mock `speed` must not be stored as `gpsSpeed` or displayed as real bicycle speed.
`BlePacket.isFullSystem` is `true` for v2 packets and `false` for legacy.

The app subscribes to GATT notifications and buffers each packet locally during recording.

---

## Project Structure

```
Smart Watt-meter/
├── CLAUDE.md
├── cloud_detail.md          ← Firestore schema & rules (authoritative)
├── firmware/
│   └── smart_watt_meter_mock/
│       └── smart_watt_meter_mock.ino   ← ESP32 mock firmware (Arduino)
├── ui design/
│   ├── SmartWattmeter.html  ← Interactive React prototype (open in browser)
│   └── android-frame.jsx    ← Material Design 3 device frame component
├── docs/
│   └── FRA503 Project Proposal Report 65009.pdf
└── smart_watt_meter/        ← Flutter app (created Apr 2026)
    ├── INSTRUCTIONS.md      ← How to run the app
    ├── pubspec.yaml
    ├── android/
    │   ├── app/build.gradle.kts     (minSdk = flutter.minSdkVersion)
    │   ├── app/src/main/AndroidManifest.xml  (BLE + GPS permissions)
    │   └── local.properties         (flutter.minSdkVersion=21 ← ห้ามลบ)
    └── lib/
        ├── main.dart                ← Entry point + bottom nav shell
        ├── models/
        │   ├── ride_sample.dart     ← 1 Hz telemetry data point
        │   └── ride_summary.dart    ← Ride metadata + Firestore serialization
        ├── services/
        │   ├── ble_service.dart     ← BLE scan/connect/GATT notify
        │   ├── gps_service.dart     ← Phone GPS + distance accumulation
        │   ├── firestore_service.dart  ← Firebase upload + ride fetch (lazy init)
        │   └── ride_session_service.dart ← Merges BLE+GPS, manages recording
        ├── providers/
        │   └── app_state.dart       ← Global state via Provider
        ├── screens/
        │   ├── device_screen.dart
        │   ├── power_dashboard_screen.dart
        │   ├── map_screen.dart      ← OSM live map, polyline route, heatmap toggle
        │   ├── history_screen.dart  ← Ride list with sort (date/duration/distance asc/desc)
        │   └── ride_detail_screen.dart ← Full-screen ride detail with heatmap map + stats grid
        └── widgets/
            └── power_chart.dart    ← 60-second line chart with Y-axis labels (CustomPainter)
```

---

## Development Setup & Commands

### Flutter App

```bash
# Project already created at smart_watt_meter/
cd smart_watt_meter

# Install dependencies
flutter pub get

# Launch emulator (Pixel 4a)
flutter emulators --launch Pixel_4a

# Run on emulator
flutter run -d emulator-5554

# Hot reload (while flutter run is active in the SAME terminal)
# press: r = hot reload, R = hot restart, q = quit

# List available devices
flutter devices

# Build APK
flutter build apk
```

**Important — minSdk:** `flutter.minSdkVersion=21` is set in `android/local.properties`.
Do NOT remove this line. Flutter's build system rewrites `build.gradle.kts` on every run
and would reset minSdk to 16, breaking `flutter_blue_plus`.

### ESP32 Firmware

Flash `firmware/smart_watt_meter_mock/smart_watt_meter_mock.ino` using Arduino IDE or `arduino-cli`:

```bash
# With arduino-cli (install ESP32 board package first)
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/smart_watt_meter_mock
arduino-cli upload  --fqbn esp32:esp32:esp32 -p <COM_PORT> firmware/smart_watt_meter_mock

# Monitor serial output (115200 baud)
arduino-cli monitor -p <COM_PORT> --config baudrate=115200
```

Required Arduino libraries (install via Library Manager or `arduino-cli lib install`):
- `ESP32 BLE Arduino` (bundled with ESP32 board package)
- No external libraries needed for mock firmware

---

## UI Screens (from prototype)

Open `ui design/SmartWattmeter.html` in a browser to see the full interactive prototype.

1. **Device** — BLE scan/connect, shows MAC, protocol, service UUID, ping status
2. **Power (Live Dashboard)** — Real-time power (large), cadence/speed/avg/peak stats, 60s chart, Start/Stop session
3. **Map (Route Tracking)** — GPS map with live power overlay, distance, elevation
4. **History** — Ride list with filter/sort, detail view with heatmap route + power-vs-time chart

---

## Key Implementation Notes

- **GPS source**: Phone GPS only (Flutter Geolocator), not a hardware GPS module on ESP32. GPS speed is stored as `gpsSpeed` (km/h), never from ESP32.
- **ESP32 must not send speed/lat/lng**: Phone GPS owns speed, route, and distance. Do not add `speed`, `lat`, `lng`, `distanceKm`, or `batteryV` to ESP32 BLE packets.
- **Power zones**: 5 zones displayed in UI (Recovery <100W, Endurance <160W, Tempo <220W, Threshold <280W, VO₂Max ≥280W) — display-only, not user-configurable
- **Schema v2 field names**: Use `powerW`, `cadenceRpm`, `gpsSpeed` throughout the Dart codebase (not ambiguous `power`, `cadence`, `speed`). `BlePacket` maps legacy mock fields on parse.
- **Dropped packet tracking**: `RideSessionService` detects gaps in `seq` from ESP32 and accumulates `droppedPacketCount` for the ride summary.
- **Device vs GPS sample counts**: `deviceSampleCount` counts full-system BLE packets received; `gpsSampleCount` counts samples that had a valid GPS fix. Both stored in ride summary.
- **Sensor status**: Final `calibrationOk`, `hxTared`, `imuOk`, `hxOk` from the last BLE packet are stored in the ride summary.
- **Offline-first**: If Firebase upload fails after ride ends, ride is returned with `syncStatus: 'pending'` but samples are not persisted locally yet.
- **Anonymous auth**: Call `FirebaseAuth.instance.signInAnonymously()` on first launch, reuse UID across sessions
- **Firebase lazy init**: `FirestoreService` uses lazy getters for `FirebaseFirestore.instance` and `FirebaseAuth.instance` — safe to construct before `Firebase.initializeApp()` completes
- **Firebase configured (Apr 2026)**: `flutterfire configure` done. `lib/firebase_options.dart` and `android/app/google-services.json` generated. `main.dart` uses `DefaultFirebaseOptions.currentPlatform`.
- **Maps**: `flutter_map` + OpenStreetMap — no API key needed. Tile URL: `https://tile.openstreetmap.org/{z}/{x}/{y}.png`
- **Live GPS always on**: `AppState.init()` calls `gpsService.startTracking()` so GPS speed is live even before a session starts. `startRecording()` resets the distance accumulator only.
- **Live telemetry always on**: `RideSessionService.initLiveUpdates()` subscribes to BLE and GPS permanently — stats update even before Start is pressed. Samples are only saved to `_samples` while `_recording == true`.
- **Route cleared after stop**: `_samples.clear()` called inside `stopRecording()` after upload — map polyline resets automatically.
- **lastSessionSamples**: `AppState` saves a copy of samples before `stopRecording()` so `RideDetailScreen` can show the GPS heatmap of the most recent ride.
- **syncStatus fix**: `RideSummary.fromFirestore` always returns `syncStatus: 'synced'` — any ride fetched from Firestore is by definition already uploaded.

## BLE Debugging Insight (Apr 2026)

- The Flutter app's BLE disconnect flow was verified to work: app logs showed `setNotifyValue(false)`, platform `disconnect`, and `connection state changed: disconnected`.
- The remaining bug was on the ESP32 BLE stack side: after app disconnect, the server sometimes still reported `connectedCount=1` even though notifications were already disabled.
- Important observation: `connectedCount=1` does **not** necessarily mean telemetry is still reaching the phone. Once `BLE2902` showed `notify=off`, real notifications had stopped even if the server still thought a client was connected.
- Current firmware workaround in `firmware/smart_watt_meter_mock/smart_watt_meter_mock.ino`:
  - track `BLE2902` directly via `getNotifications()`
  - only emit telemetry when notifications are enabled
  - if notifications are disabled but `connectedCount` stays stuck for more than ~2 seconds, force-disconnect the stale client with `pServer->disconnect(pServer->getConnId())`
- This workaround fixed the practical issue: pressing Disconnect in the app now stops ESP32 telemetry and lets advertising recover without manually resetting the board.
- If BLE reconnect problems return or become more complex, the next improvement path should be migrating firmware BLE code from `BLEDevice/BLEServer` to `NimBLE-Arduino`, which is generally considered more stable and lightweight than the legacy Arduino ESP32 BLE stack.

## Pending / Known Limitations

| Item | Detail |
|------|--------|
| Offline fallback | If Firebase upload fails, ride is returned with `syncStatus: 'pending'` but samples are not persisted locally — user loses GPS data if app is closed before retry. Fix: serialize to `shared_preferences`. |
| History GPS data | `RideDetailScreen` shows GPS heatmap only for the most recent session (from `lastSessionSamples` in memory). Older rides show "No GPS data" because samples are not fetched back from Firestore yet. Fix: add `fetchSamples(rideId)` to `FirestoreService`. |

---

## Timeline (from proposal)

| Task | Apr W3 | Apr W4 | May W1 | May W2 |
|------|--------|--------|--------|--------|
| Order parts | ✓ | | | |
| **Mobile app dev** | **✓** | | | |
| Test design | ✓ | | | |
| Circuit design + test | | ✓ | | |
| Control box design | | ✓ | | |
| Strain gauge install + calibrate | | | ✓ | |
| System integration | | | ✓ | |
| Testing + report | | | | ✓ |

Current phase (Apr W3): mobile app + mock firmware.
