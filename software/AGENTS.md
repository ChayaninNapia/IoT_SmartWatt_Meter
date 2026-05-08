# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Overview

**Smart Watt-Meter** (FRA503 — Technopreneurship in IoT Industry, KMUTT)
An IoT bicycle power measurement system. A crank-arm add-on device (ESP32 + strain gauge + IMU) measures pedaling power and transmits telemetry via BLE to a Flutter Android app, which records ride data locally and uploads to Firebase Cloud Firestore after each ride.

**Scope for current phase**: Build a minimal Flutter app that connects to ESP32 via BLE and uploads ride data to Firebase. The ESP32 is mocked — no real strain gauge or IMU yet.

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
- **Sensors (real hardware)**: HX711 (strain gauge ADC), MPU6050 (gyroscope via I2C)
- **Full-system phase**: ESP32 sends real device telemetry from HX711 + MPU6050 over BLE GATT notify. Do not treat mock BLE `speed` as real bicycle speed.

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

## Firestore Schema (v2 full-system telemetry)

See `cloud_detail.md` for full rules. Schema summary:

```
users/{uid}
  createdAt, lastActiveAt

users/{uid}/rides/{rideId}
  title, startTime, endTime, durationSec, distanceKm,
  avgPower, maxPower, avgCadence, maxCadence, avgSpeed,
  maxSpeed, avgForce, maxForce, avgTorque, maxTorque,
  avgOmega, maxOmega, calories, sampleCount, deviceSampleCount,
  gpsSampleCount, droppedPacketCount, calibrationOk, hxTared,
  imuOk, hxOk, schemaVersion, uploadMode, syncStatus, createdAt

users/{uid}/rides/{rideId}/samples/{sampleId}
  timestamp, elapsedSec, deviceTimeMs, seq, powerW, cadenceRpm,
  forceN, torqueNm, omegaRadS, hxRaw, hxTared, imuOk, hxOk,
  calOk, gpsSpeed, lat, lng
```

**Rules**: 1 ride = 1 document. 1 timestep sample = 1 document. Upload only after ride ends. New documents should set `schemaVersion: 2`. Keep backward compatibility for legacy v1 fields where practical, but new full-system samples should prefer unit-explicit fields: `powerW`, `cadenceRpm`, and `gpsSpeed`.

---

## BLE Protocol

**Device name**: `ESP32_PowerMeter_01`

**Custom Service UUID**: `12345678-1234-1234-1234-123456789012`

**Telemetry Characteristic UUID**: `12345678-1234-1234-1234-123456789013`
- Properties: READ + NOTIFY
- Interval: 1 Hz (every 1000 ms)
- Format: JSON string

**App Status / Command Characteristic UUID**: `12345678-1234-1234-1234-123456789014`
- Properties: WRITE + WRITE_NR
- Purpose: phone app writes status/commands back to ESP32 so OLED can show `GPS` and `REC`, and so app-driven tare/calibration commands are possible.
- Status write example:

```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

- Command write examples:

```json
{"type":"command","cmd":"tare"}
{"type":"command","cmd":"gyroBias"}
{"type":"command","cmd":"axisCal"}
```

**Target full-system packet format**:
```json
{
  "v": 1,
  "type": "telemetry",
  "seq": 128,
  "tMs": 128420,
  "powerW": 245.3,
  "cadenceRpm": 82.1,
  "forceN": 34.2,
  "torqueNm": 5.9,
  "omegaRadS": 8.6,
  "hxRaw": 29120,
  "hxTared": true,
  "imuOk": true,
  "hxOk": true,
  "calOk": true
}
```

The app subscribes to GATT notifications and buffers telemetry locally during recording.

Packet field notes:
- `v`: packet schema version.
- `type`: normally `telemetry`; future event packets may use `event`.
- `seq`: monotonically increasing packet sequence number for detecting dropped BLE packets.
- `tMs`: ESP32 `millis()` timestamp.
- `powerW`: device-computed cycling power in watts.
- `cadenceRpm`: crank cadence estimated from MPU6050 gyroscope data.
- `forceN`: calibrated force from HX711 in newtons.
- `torqueNm`: crank torque, derived from force and configured crank-arm length.
- `omegaRadS`: crank angular velocity in rad/s.
- `hxRaw`: HX711 raw or delta raw value for debug visibility.
- `hxTared`: whether HX711 tare has been completed.
- `imuOk`: whether MPU6050 readings are valid.
- `hxOk`: whether HX711 readings are valid.
- `calOk`: whether calibration state is good enough to trust computed `forceN`, `torqueNm`, and `powerW`.

Do not include fields the ESP32 cannot measure directly in the full-system BLE packet. In particular, do not send `speed`, `lat`, `lng`, `distanceKm`, `batteryV`, or `uploadStatus` from ESP32. Phone GPS owns speed, route, and distance. The mobile app/Firebase layer owns upload status.

Legacy mock firmware may still send:
```json
{"power":245,"cadence":85,"speed":28.5,"elapsedSec":42}
```

The Flutter parser should remain backward-compatible with this mock packet during transition, but the live dashboard should treat mock `speed` as deprecated/test-only and should not present it as real GPS speed.

---

## Project Structure

```
Smart Watt-meter/
├── AGENTS.md
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
2. **Power (Live Dashboard)** — Real-time power (large), cadence, force, torque, GPS speed, avg/peak stats, 60s chart, Start/Stop session
3. **Map (Route Tracking)** — GPS map with live power overlay, distance, elevation
4. **History** — Ride list with filter/sort, detail view with heatmap route + power-vs-time chart

---

## Key Implementation Notes

- **GPS source**: Phone GPS only (Flutter Geolocator), not a hardware GPS module on ESP32
- **GPS Speed display**: Label speed as `GPS Speed`. Show `-- km/h` before recording, during bench tests, or whenever phone GPS has no valid fix. Do not show BLE mock `speed` as real speed.
- **Full-system telemetry display**: The live dashboard should prioritize `powerW`, `cadenceRpm`, `forceN`, `torqueNm`, and `omegaRadS` from ESP32. Show placeholders such as `--` when optional fields are missing or `calOk` is false.
- **Sensor status display**: Surface `imuOk`, `hxOk`, `hxTared`, and `calOk` clearly enough for field testing. Suggested labels: `IMU OK`, `HX OK`, `TARE OK`, `CAL OK`; use warning states when false.
- **Power zones**: 5 zones displayed in UI (Recovery <100W, Endurance <160W, Tempo <220W, Threshold <280W, VO₂Max ≥280W) — display-only, not user-configurable
- **Offline-first**: If Firebase upload fails after ride ends, persist locally and retry on next app open
- **Anonymous auth**: Call `FirebaseAuth.instance.signInAnonymously()` on first launch, reuse UID across sessions
- **Route + power merge**: GPS samples (from phone, ~1 Hz) and BLE power samples (from ESP32, 1 Hz) must be merged by timestamp after ride ends
- **Firebase lazy init**: `FirestoreService` uses lazy getters for `FirebaseFirestore.instance` and `FirebaseAuth.instance` — safe to construct before `Firebase.initializeApp()` completes
- **Firebase configured (Apr 2026)**: `flutterfire configure` done. `lib/firebase_options.dart` and `android/app/google-services.json` generated. `main.dart` uses `DefaultFirebaseOptions.currentPlatform`.
- **Maps**: switched from `google_maps_flutter` to `flutter_map` + OpenStreetMap — no API key needed. Tile URL: `https://tile.openstreetmap.org/{z}/{x}/{y}.png`
- **Live telemetry always on**: `RideSessionService.initLiveUpdates()` subscribes to BLE permanently — stats update even before Start is pressed. Samples are only saved to `_samples` while `_recording == true`.
- **Route cleared after stop**: `_samples.clear()` called inside `stopRecording()` after upload — map polyline resets automatically.
- **lastSessionSamples**: `AppState` saves a copy of samples before `stopRecording()` so `RideDetailScreen` can show the GPS heatmap of the most recent ride.
- **syncStatus fix**: `RideSummary.fromFirestore` always returns `syncStatus: 'synced'` — any ride fetched from Firestore is by definition already uploaded.

## Mobile Display Design For Full-System Testing

The phone app is the main ride dashboard, route recorder, and cloud-sync surface. The OLED is the immediate device-side health/control display. Avoid duplicating every debug detail on the main phone screen; expose the essentials for riding first, then put engineering details in a debug/calibration panel.

### Power / Live Dashboard

Primary display:
- Large `powerW` in watts.
- Power zone badge based on `powerW`.
- Recording state: `REC`, paused/stopped, and save/upload status after stop.

Core metric cards:
- `Cadence`: `cadenceRpm` from ESP32, unit `rpm`.
- `Force`: `forceN` from HX711, unit `N`.
- `Torque`: `torqueNm`, unit `Nm`.
- `GPS Speed`: phone GPS speed only, unit `km/h`; show `--` when unavailable.

Secondary metrics:
- `Avg Power`, `Peak Power`.
- `Avg Cadence`, `Max Cadence`.
- Optional `omegaRadS` in a compact engineering/debug area, not as a top-level ride metric unless needed.

Status strip:
- BLE connection state.
- GPS fix state.
- IMU state from `imuOk`.
- HX711 state from `hxOk`.
- Tare/calibration state from `hxTared` and `calOk`.

### Device / Debug Screen

Show connection and hardware readiness:
- BLE device name, MAC/id, service UUID, notification state.
- Packet `seq` and last update age to catch stale telemetry.
- `tMs` from ESP32.
- `hxRaw`, `forceN`, `torqueNm`, `omegaRadS`.
- Status flags: `imuOk`, `hxOk`, `hxTared`, `calOk`.

Use this screen for bench/full-system validation where engineering visibility matters more than ride readability.

### Map Screen

Map and route data must come from the phone:
- Phone GPS route polyline.
- Phone GPS speed and distance.
- Power heatmap overlay using ESP32 `powerW` merged by timestamp with GPS samples.
- If GPS is unavailable, show a clear no-fix/bench-mode state instead of showing mock speed.

### History / Ride Detail

Ride summaries should present:
- Duration, distance from phone GPS, avg/max GPS speed.
- Avg/max power from ESP32.
- Avg/max cadence from ESP32.
- Optional avg/max force and torque if added to stored samples later.

The Firestore schema has been intentionally updated for full-system telemetry. Implementations should store `forceN`, `torqueNm`, `omegaRadS`, `hxRaw`, packet sequence/time fields, and status flags in ride samples when available. Continue to avoid schema churn beyond this v2 update.

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

---

## Mobile App Move Check Report (2026-05-06)

The Flutter app was moved under:

```text
C:\Users\Acer\Desktop\IoT\iot_bicycle_wattMeter_project\software\smart_watt_meter
```

Verification was run after the move. Use this section as the handoff note for the next check-up.

### Verified Commands

Run from `software/smart_watt_meter/`:

```powershell
flutter pub get --enforce-lockfile
flutter analyze
flutter test
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:Path="$env:JAVA_HOME\bin;$env:Path"
flutter build apk --debug
```

### Results

- `flutter pub get --enforce-lockfile` passed. `pubspec.lock` still resolves correctly after the move.
- `flutter test` passed. The widget test logs OpenStreetMap tile `ClientException` 400 messages during test rendering, but the test still completes successfully.
- `flutter build apk --debug` passed and produced `build\app\outputs\flutter-apk\app-debug.apk`.
- `flutter analyze` found no compile errors, but returned 1 lint info:
  - `lib\screens\history_screen.dart:153:43` - `unnecessary_underscores`

### Environment Notes

- The default `java` on PATH points to Oracle Java 8:

```text
C:\Program Files (x86)\Common Files\Oracle\Java\java8path\java.exe
```

- Android Gradle should be run with the Android Studio JBR instead:

```text
C:\Program Files\Android\Android Studio\jbr
```

- If `flutter build apk` or Gradle fails with Java/AGP-related errors, set `JAVA_HOME` to the Android Studio JBR before building.
- `android/local.properties` still points to the correct local SDK paths and keeps `flutter.minSdkVersion=21`.

### Known Follow-up Items

- Update stale run instructions in `smart_watt_meter/INSTRUCTIONS.md`; lines 24 and 71 still point to the old path:

```text
C:\Users\Acer\Desktop\IoT\Smart Watt-meter\smart_watt_meter
```

- Replace that old path with:

```text
C:\Users\Acer\Desktop\IoT\iot_bicycle_wattMeter_project\software\smart_watt_meter
```

- Optional cleanup: fix the analyzer lint at `lib\screens\history_screen.dart:153`.

---

## Mobile App Follow-up Check (2026-05-06)

Re-verification run after previous check-up. All follow-up items from the move report are now resolved.

### Commands Run

```powershell
flutter pub get --enforce-lockfile       # PASS
flutter analyze                          # PASS — No issues found
flutter test                             # PASS — 1 test passed (OSM tile 400 logs expected)
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:Path="$env:JAVA_HOME\bin;$env:Path"
flutter build apk --debug               # PASS — app-debug.apk produced in 14.8s
```

### Fixes Applied

- `lib\screens\history_screen.dart:153` — changed `(_, __)` → `(_, _)` to fix `unnecessary_underscores` lint. `flutter analyze` now returns **No issues found**.
- `smart_watt_meter/INSTRUCTIONS.md` — updated both stale paths (lines 24 and 71) from `C:\Users\Acer\Desktop\IoT\Smart Watt-meter\smart_watt_meter` to `C:\Users\Acer\Desktop\IoT\iot_bicycle_wattMeter_project\software\smart_watt_meter`.

### Remaining Known Items

- 31 packages have newer versions incompatible with current constraints (expected — lockfile pinned). Run `flutter pub outdated` if you want to review upgrade candidates.
- OSM tile `ClientException` 400 logs during `flutter test` are expected (no network in test env) and do not affect test outcome.
