# Flutter App — Full-System Firmware Integration Report
**Date:** 2026-05-06 (updated after full-system firmware integration)
**Author:** Claude Code
**Audience:** Codex (firmware developer)

---

## Summary of Changes (this update)

The Flutter app has been updated to work with Codex's full-system firmware
(`electrical/integration_test/smart_watt_meter_full_system/smart_watt_meter_full_system.ino`).

Key changes:
- Discovered and stored the new app status / command write characteristic `...014`.
- App sends `{"type":"appStatus","gpsOk":…,"recording":…}` to ESP32 on connect,
  on start/stop recording, on GPS fix change, and every ~1 s (heartbeat).
- Added command helpers: `tare`, `gyroBias`, `axisCal`.
- Event packets (`type == "event"`) are now routed to a separate stream and never
  silently converted into zero-power telemetry.
- `start_request` / `stop_request` events from ESP32 BTN1 trigger start/stop in the app.
- `calOk` status badge added to the Power Dashboard UI.
- Dropped packet detection counts only telemetry `seq`, not event `seq`.
- 11 new unit tests for `BlePacket.tryFromJson` and `BleEvent.tryFromJson`.

---

## BLE Connection Details (for firmware reference)

| Parameter | Value |
|-----------|-------|
| Device name | `ESP32_PowerMeter_01` |
| Service UUID | `12345678-1234-1234-1234-123456789012` |
| Telemetry characteristic | `12345678-1234-1234-1234-123456789013` — READ + NOTIFY |
| App status / command characteristic | `12345678-1234-1234-1234-123456789014` — WRITE + WRITE_NR |

---

## Full-System Telemetry Packet (v2)

Sent by ESP32 on `...013` at 1 Hz. Detected by presence of `powerW` key.

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

| Field | Type | Description |
|-------|------|-------------|
| `v` | int | Packet schema version (1) |
| `type` | string | `"telemetry"` |
| `seq` | int | Monotonic counter — app uses this to detect dropped telemetry packets |
| `tMs` | int | ESP32 `millis()` timestamp — stored as `deviceTimeMs` |
| `powerW` | double | Computed power in watts |
| `cadenceRpm` | double | Crank cadence from MPU6050 gyroscope in RPM |
| `forceN` | double | Calibrated force from HX711 in newtons |
| `torqueNm` | double | Crank torque in N·m |
| `omegaRadS` | double | Crank angular velocity in rad/s |
| `hxRaw` | int | Raw or delta-raw HX711 ADC count |
| `hxTared` | bool | Whether HX711 tare has been completed |
| `imuOk` | bool | Whether MPU6050 readings are valid |
| `hxOk` | bool | Whether HX711 readings are valid |
| `calOk` | bool | Whether calibration is complete enough to trust `forceN`, `torqueNm`, `powerW` |

---

## Event Packets

Sent by ESP32 on the same `...013` characteristic.
Distinguished from telemetry by `"type":"event"`.

```json
{"v":1,"type":"event","seq":44,"tMs":55200,"event":"start_request"}
{"v":1,"type":"event","seq":45,"tMs":55300,"event":"stop_request"}
{"v":1,"type":"event","seq":46,"tMs":55400,"event":"tare_ok"}
{"v":1,"type":"event","seq":47,"tMs":55500,"event":"gyro_bias_ok"}
{"v":1,"type":"event","seq":48,"tMs":55600,"event":"axis_cal_ok"}
```

**How the app handles events:**

| Event | App behavior |
|-------|-------------|
| `start_request` | Calls `startRide()` if connected and not already recording |
| `stop_request` | Calls `stopRide()` if currently recording |
| `tare_ok`, `gyro_bias_ok`, `axis_cal_ok`, others | Logged to debug output — no UI action yet |

**Critical:** event packets are routed to `BleService.eventStream` and never fed
into `packetStream`. `BlePacket.tryFromJson` returns `null` for any packet with
`"type":"event"`. This prevents event packets from appearing as zero-power
telemetry samples.

---

## Dropped Packet Detection

The app tracks dropped BLE packets using `seq` from **telemetry packets only**.
Event packets use the same shared firmware counter (`packetSeq`) but their `seq`
values are not counted for drop detection. This is intentional:

- The app only calls `_lastSeq` update when `BlePacket.tryFromJson` returns a
  non-null telemetry packet.
- Event seq values are silently skipped, so the gap they introduce in the telemetry
  seq view is not counted as a dropped telemetry packet.

> **Note for firmware:** if you later want precise dropped-telemetry counts, consider
> splitting into separate `telemetrySeq` and `eventSeq` counters on the ESP32 side.
> The app is ready to handle this — just add a `telemetrySeq` field to telemetry
> packets and the existing logic will use it correctly.

---

## App Status Write (OLED GPS / REC Top Bar)

The app writes to `...014` to keep the ESP32 OLED top bar in sync.

**Packet format:**
```json
{"type":"appStatus","gpsOk":true,"recording":true}
```

**When the app writes:**
1. Immediately after BLE characteristics are discovered (connect).
2. When `startRide()` is called → `recording: true`.
3. When `stopRide()` is called → `recording: false`.
4. When GPS fix status changes (`accuracy < 50 m` threshold).
5. Every ~1 second via heartbeat timer while connected.

The firmware's `APP_STATUS_TIMEOUT_MS` is 3500 ms — the 1 s heartbeat keeps
`appGpsOk` and `appRecording` alive on the OLED without timing out.

---

## Command Write (to `...014`)

The app can send calibration commands to ESP32. Methods available in `BleService`
and exposed through `AppState`:

| Dart method | JSON written | Firmware action |
|-------------|-------------|-----------------|
| `sendTare()` | `{"type":"command","cmd":"tare"}` | Runs `tareHx711()`, sends `tare_ok` event |
| `sendGyroBias()` | `{"type":"command","cmd":"gyroBias"}` | Runs `calibrateGyroBias()` |
| `sendAxisCal()` | `{"type":"command","cmd":"axisCal"}` | Runs `calibrateCrankAxis()` |

UI buttons for these commands are not wired yet — the service methods exist for
direct testing via `AppState.sendTare()` etc.

---

## calOk UI Behavior

- Dashboard always shows live `powerW` / `cadenceRpm` values regardless of `calOk`.
  This preserves debug visibility during calibration.
- A small badge next to the power zone label shows:
  - **`CAL OK`** (green) when `calOk == true`
  - **`CAL --`** (amber) when `calOk == false`
  - Badge is hidden when no full-system packet has been received yet (`calOk == null`).

---

## Fields ESP32 Must NOT Send

| Field | Reason |
|-------|--------|
| `speed` | Phone GPS owns speed. Mock `speed` is deprecated — app ignores it. |
| `lat` / `lng` | Phone GPS owns location. |
| `distanceKm` | Phone GPS accumulates distance. |
| `batteryV` | Not wired in current hardware. |
| Upload / sync status | App/Firebase layer only. |

---

## Packet Detection Logic (Dart)

```dart
// BlePacket.tryFromJson — in ble_service.dart
static BlePacket? tryFromJson(Map<String, dynamic> json) {
  final type = json['type'] as String?;
  if (type == 'event') return null;           // events never become telemetry
  if (json.containsKey('powerW')) { /* v2 */ }
  if (json.containsKey('power'))  { /* mock */ }
  return null; // unrecognised
}
```

---

## Test Results

```
flutter analyze  →  No issues found
flutter test     →  12/12 passed
  - BlePacket.tryFromJson: 7 tests
  - BleEvent.tryFromJson:  4 tests
  - Widget smoke test:     1 test
```
